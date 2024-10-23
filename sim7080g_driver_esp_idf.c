#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <driver/uart.h>

#include "sim7080g_driver_esp_idf.h"
#include "sim7080g_at_commands.h"

#define AT_CMD_MAX_LEN 256
#define AT_CMD_MAX_RETRIES 4
#define AT_RESPONSE_MAX_LEN 256

static const char *TAG = "SIM7080G Driver";

// Static Fxn Declarations:
static esp_err_t sim7080g_uart_init(const sim7080g_uart_config_t sim7080g_uart_config);
static void sim7080g_log_config_params(const sim7080g_handle_t *sim7080g_handle);
static esp_err_t send_at_cmd(const sim7080g_handle_t *sim7080g_handle,
                             const at_cmd_t *cmd,
                             at_cmd_type_t type,
                             const char *args,
                             char *response,
                             size_t response_size,
                             uint32_t timeout_ms);

esp_err_t sim7080g_config(sim7080g_handle_t *sim7080g_handle,
                          const sim7080g_uart_config_t sim7080g_uart_config,
                          const sim7080g_mqtt_config_t sim7080g_mqtt_config)
{
    // TODO - validate config params
    sim7080g_handle->uart_config = sim7080g_uart_config;

    sim7080g_handle->mqtt_config = sim7080g_mqtt_config;

    sim7080g_log_config_params(sim7080g_handle);

    return ESP_OK;
}

// ---------------------  EXTERNAL EXPOSED API FXNs  -------------------------//
esp_err_t sim7080g_init(sim7080g_handle_t *sim7080g_handle)
{
    esp_err_t err = sim7080g_uart_init(sim7080g_handle->uart_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error init UART: %s", esp_err_to_name(err));
        return err;
    }
    sim7080g_handle->uart_initialized = true;

    // TODO - Check if device params are already confgiured the same, if so, skip (to SAVE TIME)
    // THE sim7080g can remain powered and init while the ESP32 restarts - so it may be necessary to always set the mqtt params on driver init
    err = sim7080g_mqtt_set_parameters(sim7080g_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error init MQTT for device (writing config params to sim7080g): %s", esp_err_to_name(err));
        return err;
    }
    sim7080g_handle->mqtt_initialized = true;

    return ESP_OK;
}

esp_err_t sim7080g_check_sim_status(const sim7080g_handle_t *sim7080g_handle)
{
    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle, &AT_CPIN, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 5000);
    if (ret == ESP_OK)
    {
        if (strstr(response, "READY") != NULL)
        {
            ESP_LOGI(TAG, "SIM card is ready");
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "SIM card is not ready: %s", response);
            return ESP_FAIL;
        }
    }
    return ret;
}

esp_err_t sim7080g_check_signal_quality(const sim7080g_handle_t *sim7080g_handle,
                                        int8_t *rssi_out,
                                        uint8_t *ber_out)
{
    // Input validation
    if (!sim7080g_handle || !rssi_out || !ber_out)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char response[256] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_CSQ,
                                AT_CMD_TYPE_EXECUTE,
                                NULL,
                                response,
                                sizeof(response),
                                5000);

    if (ret == ESP_OK)
    {
        int rssi, ber;
        char *csq_response = strstr(response, "+CSQ:");
        if (csq_response && sscanf(csq_response, "+CSQ: %d,%d", &rssi, &ber) == 2)
        {
            // Store values in output parameters
            *rssi_out = (int8_t)rssi;
            *ber_out = (uint8_t)ber;

            ESP_LOGI(TAG, "Signal quality: RSSI=%d, BER=%d", rssi, ber);

            // Interpret and log RSSI value
            if (rssi == 99)
            {
                ESP_LOGI(TAG, "RSSI unknown or not detectable");
            }
            else if (rssi >= 0 && rssi <= 31)
            {
                int16_t dbm = -113 + (2 * rssi);
                ESP_LOGI(TAG, "RSSI: %d dBm", dbm);
            }
            else
            {
                ESP_LOGW(TAG, "RSSI value out of expected range");
                return ESP_ERR_INVALID_RESPONSE;
            }

            // Interpret and log BER value
            if (ber == 99)
            {
                ESP_LOGI(TAG, "BER unknown or not detectable");
            }
            else if (ber >= 0 && ber <= 7)
            {
                static const char *const ber_meanings[] = {
                    "BER < 0.2%",
                    "0.2% <= BER < 0.4%",
                    "0.4% <= BER < 0.8%",
                    "0.8% <= BER < 1.6%",
                    "1.6% <= BER < 3.2%",
                    "3.2% <= BER < 6.4%",
                    "6.4% <= BER < 12.8%",
                    "BER >= 12.8%"};
                ESP_LOGI(TAG, "BER: %s", ber_meanings[ber]);
            }
            else
            {
                ESP_LOGW(TAG, "BER value out of expected range");
                return ESP_ERR_INVALID_RESPONSE;
            }

            return ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse signal quality response: %s", response);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }

    return ret;
}

esp_err_t sim7080g_get_gprs_attach_status(const sim7080g_handle_t *sim7080g_handle,
                                          bool *attached_out)
{
    // Input validation
    if (!sim7080g_handle || !attached_out)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char response[AT_RESPONSE_MAX_LEN] = {0};

    // Send AT+CGATT? command with 75 second timeout as per documentation
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_CGATT,
                                AT_CMD_TYPE_READ,
                                NULL,
                                response,
                                sizeof(response),
                                15000); // 75 seconds as per spec
                                        // TODO - 75 seconds is spec

    if (ret == ESP_OK)
    {
        int status;
        char *cgatt_response = strstr(response, "+CGATT:");

        if (cgatt_response && sscanf(cgatt_response, "+CGATT: %d", &status) == 1)
        {
            // Validate status value is within spec (0 or 1)
            if (status == 0 || status == 1)
            {
                *attached_out = (status == 1);

                if (status == 1)
                {
                    ESP_LOGI(TAG, "Device is attached to the GPRS network");
                }
                else
                {
                    ESP_LOGW(TAG, "Device is not attached to the GPRS network");
                }

                return ESP_OK;
            }
            else
            {
                ESP_LOGE(TAG, "Invalid attachment status value: %d", status);
                return ESP_ERR_INVALID_RESPONSE;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse GPRS attachment status from response: %s",
                     response);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to send CGATT command or command timed out");
    }

    return ret;
}

esp_err_t sim7080g_get_operator_info(const sim7080g_handle_t *sim7080g_handle,
                                     int *operator_code,
                                     int *operator_format,
                                     char *operator_name,
                                     int operator_name_len)
{
    if (!sim7080g_handle || !operator_code || !operator_format ||
        !operator_name || operator_name_len <= 0)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize output buffer
    memset(operator_name, 0, operator_name_len);

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_COPS,
                                AT_CMD_TYPE_READ,
                                NULL,
                                response,
                                sizeof(response),
                                5000);

    if (ret == ESP_OK)
    {
        char *cops_response = strstr(response, "+COPS:");
        if (cops_response)
        {
            int mode = 0;
            int format = 0;
            char operator_str[64] = {0}; // Temporary buffer for operator name
            int act = 0;                 // Access Technology

            // Try to parse the response with all fields
            int parsed = sscanf(cops_response,
                                "+COPS: %d,%d,\"%63[^\"]\",%d",
                                &mode, &format, operator_str, &act);

            if (parsed >= 3) // We need at least mode, format and operator name
            {
                // Validate mode value (0-4)
                if (mode < 0 || mode > 4)
                {
                    ESP_LOGE(TAG, "Invalid operator mode: %d", mode);
                    return ESP_ERR_INVALID_RESPONSE;
                }

                // Validate format value (0-2)
                if (format < 0 || format > 2)
                {
                    ESP_LOGE(TAG, "Invalid operator format: %d", format);
                    return ESP_ERR_INVALID_RESPONSE;
                }

                // Check if operator name will fit in provided buffer
                if (strlen(operator_str) >= operator_name_len)
                {
                    ESP_LOGE(TAG, "Operator name buffer too small");
                    return ESP_ERR_INVALID_SIZE;
                }

                // Store values in output parameters
                *operator_code = mode;
                *operator_format = format;
                strncpy(operator_name, operator_str, operator_name_len - 1);
                operator_name[operator_name_len - 1] = '\0'; // Ensure null termination

                ESP_LOGI(TAG, "Operator info: mode=%d, format=%d, name=%s, AcT=%d",
                         mode, format, operator_name, act);

                return ESP_OK;
            }
            else
            {
                // Try alternate format without operator name (might be in limited service)
                if (sscanf(cops_response, "+COPS: %d", &mode) == 1)
                {
                    *operator_code = mode;
                    *operator_format = 0;
                    strncpy(operator_name, "NO SERVICE", operator_name_len - 1);
                    operator_name[operator_name_len - 1] = '\0';

                    ESP_LOGW(TAG, "Limited service, mode=%d", mode);
                    return ESP_OK;
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to parse operator info response");
                    return ESP_ERR_INVALID_RESPONSE;
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "No COPS response found in: %s", response);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }

    return ret;
}

esp_err_t sim7080g_get_apn(const sim7080g_handle_t *sim7080g_handle, char *apn, int apn_len)
{
    if (!sim7080g_handle || !apn || apn_len <= 0)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t err = send_at_cmd(sim7080g_handle, &AT_CGNAPN, AT_CMD_TYPE_EXECUTE, NULL, response, sizeof(response), 5000);
    if (err == ESP_OK)
    {
        int valid;
        char apn_str[64] = {0};
        if (sscanf(response, "+CGNAPN: %d,\"%63[^\"]\"", &valid, apn_str) == 2)
        {
            if (valid == 1)
            {
                strncpy(apn, apn_str, apn_len - 1);
                apn[apn_len - 1] = '\0';
                ESP_LOGI(TAG, "APN: %s", apn);
                return ESP_OK;
            }
            else
            {
                ESP_LOGW(TAG, "No APN available");
                return ESP_ERR_NOT_FOUND;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse APN response: %s", response);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }
    ESP_LOGE(TAG, "Failed to get APN");
    return err;
}

esp_err_t sim7080g_set_apn(const sim7080g_handle_t *sim7080g_handle, const char *apn)
{
    if (!sim7080g_handle || !apn)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char cmd[AT_CMD_MAX_LEN];
    snprintf(cmd, sizeof(cmd), "0,1,\"%s\"", apn);
    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle, &AT_CNCFG, AT_CMD_TYPE_WRITE, cmd, response, sizeof(response), 10000);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "APN configured successfully");
        return ESP_OK;
    }
    return ret;
}

esp_err_t sim7080g_app_network_activate(const sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle, &AT_CNACT, AT_CMD_TYPE_WRITE, "0,1", response, sizeof(response), 15000);
    if (ret == ESP_OK)
    {
        if (strstr(response, "+APP PDP: 0,ACTIVE") != NULL)
        {
            ESP_LOGI(TAG, "Network activated successfully");
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to activate network: %s", response);
            return ESP_FAIL;
        }
    }
    return ret;
}

esp_err_t sim7080g_get_app_network_active(const sim7080g_handle_t *sim7080g_handle,
                                          int pdpidx,
                                          int *status,
                                          char *address,
                                          int address_len)
{
    // Input validation
    if (!sim7080g_handle || !status || !address || address_len <= 0)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (pdpidx < 0 || pdpidx > 3)
    {
        ESP_LOGE(TAG, "Invalid PDP context index: %d", pdpidx);
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize output parameters
    *status = 0;
    memset(address, 0, address_len);

    char response[256] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_CNACT,
                                AT_CMD_TYPE_READ,
                                NULL,
                                response,
                                sizeof(response),
                                20000);

    if (ret == ESP_OK)
    {
        char *cnact_response = strstr(response, "+CNACT:");
        bool found_context = false;

        // Process each CNACT response line (there may be multiple contexts)
        while (cnact_response)
        {
            int parsed_idx;
            int parsed_status;
            char parsed_addr[64] = {0};

            // Parse the response line
            int fields = sscanf(cnact_response,
                                "+CNACT: %d,%d,\"%63[^\"]\"",
                                &parsed_idx,
                                &parsed_status,
                                parsed_addr);

            if (fields >= 2) // At minimum need index and status
            {
                if (parsed_idx == pdpidx)
                {
                    // Found our target PDP context
                    found_context = true;

                    // Validate status value
                    if (parsed_status < 0 || parsed_status > 2)
                    {
                        ESP_LOGE(TAG, "Invalid status value: %d", parsed_status);
                        return ESP_ERR_INVALID_RESPONSE;
                    }

                    // Store status
                    *status = parsed_status;

                    // If we have an IP address, store it
                    if (fields == 3 && parsed_status > 0)
                    {
                        if (strlen(parsed_addr) >= address_len)
                        {
                            ESP_LOGE(TAG, "IP address buffer too small");
                            return ESP_ERR_INVALID_SIZE;
                        }
                        strncpy(address, parsed_addr, address_len - 1);
                        address[address_len - 1] = '\0';
                    }

                    // Log the status
                    const char *status_str[] = {
                        "Deactivated",
                        "Activated",
                        "In Operation"};
                    ESP_LOGI(TAG, "PDP Context %d: %s, IP: %s",
                             pdpidx,
                             status_str[parsed_status],
                             (parsed_status > 0) ? address : "None");

                    return ESP_OK;
                }
            }
            else
            {
                ESP_LOGW(TAG, "Failed to parse CNACT response line: %s",
                         cnact_response);
            }

            // Look for next CNACT line
            cnact_response = strstr(cnact_response + 1, "+CNACT:");
        }

        if (!found_context)
        {
            ESP_LOGW(TAG, "PDP context %d not found in response", pdpidx);
            return ESP_ERR_NOT_FOUND;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to read network status");
    }

    return ret;
}

esp_err_t sim7080g_connect_to_network_bearer(const sim7080g_handle_t *sim7080g_handle, const char *apn)
{
    if (!sim7080g_handle || !apn)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Beginning sequence of AT commands to connect to sim7080g to network bearer");

    esp_err_t err = sim7080g_check_sim_status(sim7080g_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error checking SIM status: %s", esp_err_to_name(err));
        return err;
    }

    int8_t rssi;
    uint8_t ber;
    err = sim7080g_check_signal_quality(sim7080g_handle, &rssi, &ber);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error checking signal quality: %s", esp_err_to_name(err));
        return err;
    }

    bool attached;
    err = sim7080g_get_gprs_attach_status(sim7080g_handle, &attached);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error checking GPRS attach status: %s", esp_err_to_name(err));
        return err;
    }

    int operator_code;
    int operator_format;
    char operator_name[32];
    err = sim7080g_get_operator_info(sim7080g_handle, &operator_code, &operator_format, operator_name, sizeof(operator_name));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error getting operator info: %s", esp_err_to_name(err));
        return err;
    }

    char current_apn[32];
    int apn_len = sizeof(current_apn);
    err = sim7080g_get_apn(sim7080g_handle, current_apn, apn_len);

    err = sim7080g_set_apn(sim7080g_handle, apn);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting APN: %s", esp_err_to_name(err));
        return err;
    }

    err = sim7080g_app_network_activate(sim7080g_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error activating network: %s", esp_err_to_name(err));
        return err;
    }

    int pdpdix = 0;
    int status;
    char address[32];
    int address_len = sizeof(address);
    err = sim7080g_get_app_network_active(sim7080g_handle, pdpdix, &status, address, address_len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error getting network active status: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Network bearer connected successfully");
    return ESP_OK;
}

esp_err_t sim7080g_mqtt_set_parameters(const sim7080g_handle_t *sim7080g_handle)
{
    // Input validation
    if (!sim7080g_handle)
    {
        ESP_LOGE(TAG, "Invalid device handle");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Beginning sequence of AT commands to set MQTT parameters on sim7080g to match driver handle config");

    esp_err_t ret = ESP_OK;

    // Log configuration parameters for debugging
    ESP_LOGI(TAG, "Configuring MQTT with parameters:");
    ESP_LOGI(TAG, "  URL: %s", sim7080g_handle->mqtt_config.broker_url);
    ESP_LOGI(TAG, "  Port: %d", sim7080g_handle->mqtt_config.port);
    ESP_LOGI(TAG, "  Client ID: %s", sim7080g_handle->mqtt_config.client_id);
    ESP_LOGI(TAG, "  Username: %s", sim7080g_handle->mqtt_config.username);

    char cmd[256] = {0};
    char response[256] = {0};

    // Configure URL and port
    if (snprintf(cmd, sizeof(cmd), "\"URL\",\"%s\",%d",
                 sim7080g_handle->mqtt_config.broker_url,
                 sim7080g_handle->mqtt_config.port) >= sizeof(cmd))
    {
        ESP_LOGE(TAG, "URL command string too long");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Setting MQTT URL");
    ret = send_at_cmd(sim7080g_handle,
                      &AT_SMCONF,
                      AT_CMD_TYPE_WRITE,
                      cmd,
                      response,
                      sizeof(response),
                      5000);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set MQTT URL");
        return ret;
    }

    // Configure Client ID
    if (snprintf(cmd, sizeof(cmd), "\"CLIENTID\",\"%s\"",
                 sim7080g_handle->mqtt_config.client_id) >= sizeof(cmd))
    {
        ESP_LOGE(TAG, "Client ID command string too long");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Setting MQTT Client ID");
    ret = send_at_cmd(sim7080g_handle,
                      &AT_SMCONF,
                      AT_CMD_TYPE_WRITE,
                      cmd,
                      response,
                      sizeof(response),
                      5000);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set MQTT Client ID");
        return ret;
    }

    // Configure Username
    if (snprintf(cmd, sizeof(cmd), "\"USERNAME\",\"%s\"",
                 sim7080g_handle->mqtt_config.username) >= sizeof(cmd))
    {
        ESP_LOGE(TAG, "Username command string too long");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Setting MQTT Username");
    ret = send_at_cmd(sim7080g_handle,
                      &AT_SMCONF,
                      AT_CMD_TYPE_WRITE,
                      cmd,
                      response,
                      sizeof(response),
                      5000);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set MQTT Username");
        return ret;
    }

    // Configure Password
    if (snprintf(cmd, sizeof(cmd), "\"PASSWORD\",\"%s\"",
                 sim7080g_handle->mqtt_config.client_password) >= sizeof(cmd))
    {
        ESP_LOGE(TAG, "Password command string too long");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Setting MQTT Password");
    ret = send_at_cmd(sim7080g_handle,
                      &AT_SMCONF,
                      AT_CMD_TYPE_WRITE,
                      cmd,
                      response,
                      sizeof(response),
                      5000);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set MQTT Password");
        return ret;
    }

    // Set additional MQTT parameters with default values
    const struct
    {
        const char *param;
        const char *value;
    } default_params[] = {
        {"KEEPTIME", "60"}, // 60 second keepalive
        {"CLEANSS", "1"},   // Clean session enabled
        {"QOS", "1"},       // QoS 1 default
        {"RETAIN", "0"},    // Don't retain messages
        {"SUBHEX", "0"},    // Normal (not hex) subscribe data
        {"ASYNCMODE", "0"}  // Synchronous mode
    };

    for (size_t i = 0; i < sizeof(default_params) / sizeof(default_params[0]); i++)
    {
        if (snprintf(cmd, sizeof(cmd), "\"%s\",\"%s\"",
                     default_params[i].param,
                     default_params[i].value) >= sizeof(cmd))
        {
            ESP_LOGE(TAG, "Default parameter command string too long");
            return ESP_ERR_INVALID_SIZE;
        }

        ESP_LOGI(TAG, "Setting MQTT %s=%s", default_params[i].param, default_params[i].value);
        ret = send_at_cmd(sim7080g_handle,
                          &AT_SMCONF,
                          AT_CMD_TYPE_WRITE,
                          cmd,
                          response,
                          sizeof(response),
                          5000);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set MQTT %s", default_params[i].param);
            return ret;
        }
    }

    ESP_LOGI(TAG, "MQTT setting sim7080g device params successful");
    return ESP_OK;
}

esp_err_t sim7080g_mqtt_connect_to_broker(const sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle)
    {
        ESP_LOGE(TAG, "Invalid device handle");
        return ESP_ERR_INVALID_ARG;
    }

    sim7080g_mqtt_connection_status_t curr_status;
    esp_err_t ret = sim7080g_mqtt_get_broker_connection_status(sim7080g_handle, &curr_status);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to check current connection status");
        return ret;
    }

    if (curr_status != MQTT_STATUS_DISCONNECTED)
    {
        ESP_LOGI(TAG, "MQTT broker already connected");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Attempting to connect to MQTT broker");

    char response[AT_RESPONSE_MAX_LEN] = {0};
    ret = send_at_cmd(sim7080g_handle,
                      &AT_SMCONN,
                      AT_CMD_TYPE_EXECUTE,
                      NULL,
                      response,
                      sizeof(response),
                      15000); // 15 second timeout for connection

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send MQTT connect command");
        return ret;
    }

    // Check for error responses
    if (strstr(response, "ERROR") != NULL)
    {
        // Extract error code if present
        char *error_start = strstr(response, "+CME ERROR:");
        if (error_start != NULL)
        {
            int error_code;
            if (sscanf(error_start, "+CME ERROR: %d", &error_code) == 1)
            {
                // Map error code to appropriate ESP error code and log details
                switch (error_code)
                {
                case MQTT_ERR_NETWORK:
                    ESP_LOGE(TAG, "MQTT connection failed: Network error");
                    return MQTT_ERR_NETWORK;

                case MQTT_ERR_PROTOCOL:
                    ESP_LOGE(TAG, "MQTT connection failed: Protocol error");
                    return MQTT_ERR_PROTOCOL;

                case MQTT_ERR_UNAVAILABLE:
                    ESP_LOGE(TAG, "MQTT connection failed: Broker unavailable");
                    return ESP_ERR_NOT_FOUND;

                case MQTT_ERR_TIMEOUT:
                    ESP_LOGE(TAG, "MQTT connection failed: Connection timeout");
                    return ESP_ERR_TIMEOUT;

                case MQTT_ERR_REJECTED:
                    ESP_LOGE(TAG, "MQTT connection failed: Connection rejected by broker");
                    return ESP_ERR_INVALID_STATE;

                default:
                    ESP_LOGE(TAG, "MQTT connection failed with unknown error code: %d",
                             error_code);
                    return ESP_FAIL;
                }
            }
        }
        ESP_LOGE(TAG, "MQTT connection failed with unspecified error: %s", response);
        return ESP_FAIL;
    }

    // Verify successful connection
    if (strstr(response, "OK") != NULL)
    {
        // Double check connection status
        ret = sim7080g_mqtt_get_broker_connection_status(sim7080g_handle, &curr_status);
        if (ret == ESP_OK && curr_status != MQTT_STATUS_DISCONNECTED)
        {
            ESP_LOGI(TAG, "Successfully connected to MQTT broker");
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Got OK but connection status check failed");
            return ESP_ERR_INVALID_STATE;
        }
    }

    ESP_LOGE(TAG, "Unexpected response from MQTT connect command: %s", response);
    return ESP_ERR_INVALID_RESPONSE;
}

esp_err_t sim7080g_mqtt_get_broker_connection_status(
    const sim7080g_handle_t *sim7080g_handle,
    sim7080g_mqtt_connection_status_t *status_out)
{
    if (!sim7080g_handle || !status_out)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    *status_out = MQTT_STATUS_DISCONNECTED;

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_SMSTATE,
                                AT_CMD_TYPE_READ,
                                NULL,
                                response,
                                sizeof(response),
                                5000);

    if (ret == ESP_OK)
    {
        // Look for the "+SMSTATE:" response line
        char *status_line = strstr(response, "+SMSTATE:");
        if (status_line != NULL)
        {
            int raw_status;
            if (sscanf(status_line, "+SMSTATE: %d", &raw_status) == 1)
            {
                // Validate status value
                if (raw_status < 0 || raw_status > 2)
                {
                    ESP_LOGE(TAG, "Invalid MQTT status value: %d", raw_status);
                    return ESP_ERR_INVALID_RESPONSE;
                }

                // Store status in output parameter
                *status_out = (sim7080g_mqtt_connection_status_t)raw_status;

                static const char *const status_str[] = {
                    "Disconnected",
                    "Connected",
                    "Connected (Session Present)"};

                ESP_LOGI(TAG, "MQTT Broker Status: %s", status_str[raw_status]);

                return ESP_OK;
            }
            else
            {
                ESP_LOGE(TAG, "Failed to parse MQTT status number");
                return ESP_ERR_INVALID_RESPONSE;
            }
        }
        else
        {
            ESP_LOGE(TAG, "No SMSTATE response found in: %s", response);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }

    ESP_LOGE(TAG, "Failed to check MQTT broker connection status: %d", ret);
    return ret;
}

// ---------------------  INTERNAL HELPER / STATIC FXNs  ---------------------//

static esp_err_t send_at_cmd(const sim7080g_handle_t *sim7080g_handle,
                             const at_cmd_t *cmd,
                             at_cmd_type_t type,
                             const char *args,
                             char *response,
                             size_t response_size,
                             uint32_t timeout_ms)
{
    if (!sim7080g_handle || !sim7080g_handle->uart_initialized)
    {
        ESP_LOGE(TAG, "SIM7080G driver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (cmd == NULL || response == NULL || response_size == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char at_cmd[AT_CMD_MAX_LEN] = {0};
    const at_cmd_info_t *cmd_info;

    // Determine the correct command string based on the command type
    switch (type)
    {
    case AT_CMD_TYPE_TEST:
        cmd_info = &cmd->test;
        break;
    case AT_CMD_TYPE_READ:
        cmd_info = &cmd->read;
        break;
    case AT_CMD_TYPE_WRITE:
        cmd_info = &cmd->write;
        break;
    case AT_CMD_TYPE_EXECUTE:
        cmd_info = &cmd->execute;
        break;
    default:
        ESP_LOGE(TAG, "Invalid command type");
        return ESP_ERR_INVALID_ARG;
    }

    if (cmd_info->cmd_string == NULL)
    {
        ESP_LOGE(TAG, "Command string is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Format the AT command string
    if (type == AT_CMD_TYPE_WRITE && args != NULL)
    {
        if (snprintf(at_cmd, sizeof(at_cmd), "%s%s\r\n", cmd_info->cmd_string, args) >= sizeof(at_cmd))
        {
            ESP_LOGE(TAG, "AT command too long");
            return ESP_ERR_INVALID_SIZE;
        }
    }
    else
    {
        if (snprintf(at_cmd, sizeof(at_cmd), "%s\r\n", cmd_info->cmd_string) >= sizeof(at_cmd))
        {
            ESP_LOGE(TAG, "AT command too long");
            return ESP_ERR_INVALID_SIZE;
        }
    }

    esp_err_t ret = ESP_FAIL;
    for (int retry = 0; retry < AT_CMD_MAX_RETRIES; retry++)
    {
        ESP_LOGI(TAG, "Sending AT command (attempt %d/%d): %s", retry + 1, AT_CMD_MAX_RETRIES, at_cmd);

        // Clear any pending data in UART buffers
        uart_flush(sim7080g_handle->uart_config.port_num);

        int bytes_written = uart_write_bytes(sim7080g_handle->uart_config.port_num,
                                             at_cmd, strlen(at_cmd));
        if (bytes_written < 0)
        {
            ESP_LOGE(TAG, "Failed to send AT command");
            ret = ESP_FAIL;
            continue;
        }

        int bytes_read = uart_read_bytes(sim7080g_handle->uart_config.port_num,
                                         response,
                                         response_size - 1,
                                         pdMS_TO_TICKS(timeout_ms));
        if (bytes_read < 0)
        {
            ESP_LOGE(TAG, "Failed to read AT command response");
            ret = ESP_FAIL;
            continue;
        }

        // Ensure null-termination
        response[bytes_read] = '\0';

        // Check for expected response or error
        if (strstr(response, "OK") != NULL)
        {
            ESP_LOGI(TAG, "AT command successful");
            return ESP_OK;
        }
        else if (strstr(response, "ERROR") != NULL)
        {
            ESP_LOGE(TAG, "AT command error");
            ret = ESP_FAIL;
            continue;
        }
        else if (bytes_read == 0)
        {
            ESP_LOGW(TAG, "AT command timeout");
            ret = ESP_ERR_TIMEOUT;
            continue;
        }
        else
        {
            ESP_LOGW(TAG, "Unexpected AT command response");
            ret = ESP_FAIL;
        }
    }

    ESP_LOGE(TAG, "AT command failed after %d attempts", AT_CMD_MAX_RETRIES);
    return ret;
}

static void sim7080g_log_config_params(const sim7080g_handle_t *sim7080g_handle)
{
    ESP_LOGI(TAG, "SIM7080G UART Config:");
    ESP_LOGI(TAG, "  - RX GPIO: %d", sim7080g_handle->uart_config.gpio_num_rx);
    ESP_LOGI(TAG, "  - TX GPIO: %d", sim7080g_handle->uart_config.gpio_num_tx);
    ESP_LOGI(TAG, "  - Port Num: %d", sim7080g_handle->uart_config.port_num);

    ESP_LOGI(TAG, "SIM7080G MQTT Config:");
    ESP_LOGI(TAG, "  - Broker URL: %s", sim7080g_handle->mqtt_config.broker_url);
    ESP_LOGI(TAG, "  - Username: %s", sim7080g_handle->mqtt_config.username);
    ESP_LOGI(TAG, "  - Client ID: %s", sim7080g_handle->mqtt_config.client_id);
    ESP_LOGI(TAG, "  - Client Password: %s", sim7080g_handle->mqtt_config.client_password);
    ESP_LOGI(TAG, "  - Port: %d", sim7080g_handle->mqtt_config.port);

    return;
}

static esp_err_t sim7080g_uart_init(const sim7080g_uart_config_t sim7080g_uart_config)
{
    uart_config_t uart_config = {
        .baud_rate = SIM7080G_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install((uart_port_t)sim7080g_uart_config.port_num, SIM87080G_UART_BUFF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error installing UART driver: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config((uart_port_t)sim7080g_uart_config.port_num, &uart_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configuring UART parameters: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin((uart_port_t)sim7080g_uart_config.port_num, sim7080g_uart_config.gpio_num_rx, sim7080g_uart_config.gpio_num_tx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting UART pins: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

// ---------------------  TESTING FXNs  ---------------------//

bool sim7080g_test_uart_loopback(sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle->uart_initialized)
    {
        ESP_LOGE(TAG, "SIM7080G driver not initialized");
        return false;
    }

    const char *test_str = "Hello, SIM7080G!";
    char rx_buffer[128];

    // Send data
    int tx_bytes = uart_write_bytes(sim7080g_handle->uart_config.port_num, test_str, strlen(test_str));
    ESP_LOGI(TAG, "Sent %d bytes: %s", tx_bytes, test_str);

    // Read data
    int len = uart_read_bytes(sim7080g_handle->uart_config.port_num, rx_buffer, sizeof(rx_buffer) - 1, 1000);

    if (len < 0)
    {
        ESP_LOGE(TAG, "UART read error");
        return false;
    }

    // Add null terminator to the byte data read to handle it like a string
    rx_buffer[len] = '\0';

    ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

    // Compare sent and received data
    if (strcmp(test_str, rx_buffer) == 0)
    {
        ESP_LOGI(TAG, "UART loopback test passed!");
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "UART loopback test failed!");
        return false;
    }
}