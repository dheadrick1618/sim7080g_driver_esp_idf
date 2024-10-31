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
static esp_err_t sim7080g_echo_off(const sim7080g_handle_t *sim7080g_handle);
static esp_err_t sim7080g_uart_init(const sim7080g_uart_config_t sim7080g_uart_config);
static void sim7080g_log_config_params(const sim7080g_handle_t *sim7080g_handle);
static esp_err_t send_at_cmd(const sim7080g_handle_t *sim7080g_handle,
                             const at_cmd_t *cmd,
                             at_cmd_type_t type,
                             const char *args,
                             char *response,
                             size_t response_size,
                             uint32_t timeout_ms);
static esp_err_t sim7080g_mqtt_check_parameters_match(const sim7080g_handle_t *sim7080g_handle,
                                                      bool *params_match_out);

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
        ESP_LOGE(TAG, "UART not initiailzed : %s", esp_err_to_name(err));
        sim7080g_handle->uart_initialized = false;
        return err;
    }
    else
    {
        ESP_LOGI(TAG, "UART initialized");
        sim7080g_handle->uart_initialized = true;
    }

    // The sim7080g can remain powered and init while the ESP32 restarts - so it may not be necessary to always set the mqtt params on driver init
    bool params_match;
    sim7080g_mqtt_check_parameters_match(sim7080g_handle, &params_match);
    if (params_match == true)
    {
        ESP_LOGI(TAG, "Device MQTT parameter check: Param values already match Config values - skipping MQTT set params on init");
    }
    else
    {
        err = sim7080g_mqtt_set_parameters(sim7080g_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Device MQTT parameter set: Error init MQTT for device (writing config params to sim7080g): %s", esp_err_to_name(err));
            sim7080g_handle->mqtt_initialized = false;
            return err;
        }
        else
        {
            ESP_LOGI(TAG, "Device MQTT parameter set: Success - MQTT params set on device");
            sim7080g_handle->mqtt_initialized = true;
        }
    }

    err = sim7080g_echo_off(sim7080g_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to turn off echo: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "SIM7080G Driver initialized");
    return ESP_OK;
}

esp_err_t sim7080g_check_sim_status(const sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending check SIM status cmd");

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
    if (!sim7080g_handle || !rssi_out || !ber_out)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending check signal quality cmd");

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
    if (!sim7080g_handle || !attached_out)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending get operator info cmd");

    char response[AT_RESPONSE_MAX_LEN] = {0};

    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_CGATT,
                                AT_CMD_TYPE_READ,
                                NULL,
                                response,
                                sizeof(response),
                                15000);
    // TODO - 75 seconds is spec - but this is a long time to wait in REAL life...

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

    ESP_LOGI(TAG, "Sending get operator info cmd");

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

    ESP_LOGI(TAG, "Sending get APN cmd");

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t err = send_at_cmd(sim7080g_handle, &AT_CGNAPN, AT_CMD_TYPE_EXECUTE, NULL, response, sizeof(response), 8000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send AT command");
        return err;
    }

    // Check if response contains "+CGNAPN:"
    char *cgnapn_start = strstr(response, "+CGNAPN:");
    if (!cgnapn_start)
    {
        ESP_LOGE(TAG, "No CGNAPN response found");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Parse using strtok to handle potential variations in response format
    char *line = strtok(cgnapn_start, "\r\n");
    if (!line)
    {
        ESP_LOGE(TAG, "Failed to tokenize response");
        return ESP_ERR_INVALID_RESPONSE;
    }

    int valid;
    char apn_str[64] = {0};

    // Try different parsing approaches
    if (sscanf(line, "+CGNAPN: %d,\"%63[^\"]\"", &valid, apn_str) == 2)
    {
        // Standard format: +CGNAPN: 1,"simbase"
        if (valid != 1)
        {
            ESP_LOGW(TAG, "APN not valid (valid=%d)", valid);
            return ESP_ERR_NOT_FOUND;
        }
    }
    else if (sscanf(line, "+CGNAPN: \"%63[^\"]\"", apn_str) == 1)
    {
        // Alternative format without valid flag: +CGNAPN: "simbase"
        valid = 1;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to parse APN response format: %s", line);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Copy the APN if we got here
    if (strlen(apn_str) > 0)
    {
        strncpy(apn, apn_str, apn_len - 1);
        apn[apn_len - 1] = '\0';
        ESP_LOGI(TAG, "Successfully parsed APN: %s", apn);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "No valid APN found in response");
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sim7080g_set_apn(const sim7080g_handle_t *sim7080g_handle, const char *apn)
{
    if (!sim7080g_handle || !apn)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending Set APN cmd to set APN to %s", apn);

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
    // ESP_LOGI(TAG, "Sending Activating network APP network PDP context cmd");
    // To ensure clear state - deactivate first (if currently active), then clear any pending errors
    int pdpidx = 0;
    int status;
    char address[64] = {0};
    esp_err_t ret = sim7080g_get_app_network_active(sim7080g_handle, pdpidx, &status, address, sizeof(address));
    if (ret == ESP_OK)
    {
        if (status == 2)
        { // IF network  is  already active
            ret = sim7080g_app_network_deactivate(sim7080g_handle);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to deactivate network before activating");
                return ret;
            }
            return ESP_OK;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get network status before activating");
        return ret;
    }

    ret = sim7080g_cycle_cfun(sim7080g_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to cycle CFUN before activating network");
        return ret;
    }

    // Repeat this command multiple times - response needs to be validated - sometimes it can return OK and still be deactive
    for (int i = 0; i < 3; i++)
    {
        char response[AT_RESPONSE_MAX_LEN] = {0};
        ret = send_at_cmd(sim7080g_handle, &AT_CNACT, AT_CMD_TYPE_WRITE, "0,1", response, sizeof(response), 15000);
        if (ret == ESP_OK)
        {
            if (strstr(response, "+APP PDP: 0,ACTIVE") != NULL)
            {
                ESP_LOGI(TAG, "Network activated successfully");
                return ESP_OK;
            }
        }
    }
    ESP_LOGE(TAG, "Failed to activate network");
    return ESP_FAIL;
}

esp_err_t sim7080g_cycle_cfun(const sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle, &AT_CFUN, AT_CMD_TYPE_WRITE, "0", response, sizeof(response), 10000);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send CFUN=0 command");
        return ret;
    }

    ESP_LOGI(TAG, "Waiting for CFUN=0 to take effect");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ret = send_at_cmd(sim7080g_handle, &AT_CFUN, AT_CMD_TYPE_WRITE, "1", response, sizeof(response), 10000);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send CFUN=1 command");
        return ret;
    }

    ESP_LOGI(TAG, "Waiting for CFUN=1 to take effect");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    return ESP_OK;
}

// TODO -
// esp_err_t sim7080g_get_cfun(const sim7080g_handle_t *sim7080g_handle)
// {
//     if (!sim7080g_handle)
//     {
//         ESP_LOGE(TAG, "Invalid parameters");
//         return ESP_ERR_INVALID_ARG;
//     }
//     ESP_LOGI(TAG, "Sending get CFUN cmd");
// }

esp_err_t sim7080g_app_network_deactivate(const sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Sending app network deactivate cmd");

    for (int i = 0; i < 3; i++)
    {
        /// Loops becasue the send at cmd fxn might get an OK - but the device might remain active
        char response[AT_RESPONSE_MAX_LEN] = {0};
        esp_err_t ret = send_at_cmd(sim7080g_handle, &AT_CNACT, AT_CMD_TYPE_WRITE, "0,0", response, sizeof(response), 15000);
        if (ret == ESP_OK)
        {
            if (strstr(response, "+APP PDP: 0,DEACTIVE") != NULL)
            {
                ESP_LOGI(TAG, "Network deactivated successfully");
                return ESP_OK;
            }
        }
    }
    ESP_LOGE(TAG, "Failed to deactivate network");
    return ESP_FAIL;
}

esp_err_t sim7080g_get_app_network_active(const sim7080g_handle_t *sim7080g_handle,
                                          int pdpidx,
                                          int *status,
                                          char *address,
                                          int address_len)
{
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
    // ESP_LOGI(TAG, "Reading network status for PDP context %d", pdpidx);

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
    else
    {
        if (rssi <= 0)
        {
            ESP_LOGE(TAG, "Signal quality is too low (-115dBm or less) to connect to network");
            return ESP_FAIL;
        }
        if (rssi == 99)
        {
            ESP_LOGE(TAG, "Signal quality is unknown or not detectable");
            return ESP_FAIL;
        }
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
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error getting APN: %s", esp_err_to_name(err));
        return err;
    }

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
    if (status <= 0) // 1 - connected, 2 - in operation
    {
        ESP_LOGE(TAG, "Network bearer not connected");
        return ESP_FAIL;
    }

    // TODO DEBUG THIS - Somehow this can be reached when the network is not actually connected
    ESP_LOGI(TAG, "Network bearer connected successfully");
    return ESP_OK;
}

esp_err_t sim7080g_mqtt_set_parameters(const sim7080g_handle_t *sim7080g_handle)
{
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

    char cmd[AT_CMD_MAX_LEN] = {0};
    char response[AT_RESPONSE_MAX_LEN] = {0};

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
        {"QOS", "0"},       // QoS 1 default
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
        return ESP_OK;
    }

    // Verify network broker connected before attempting MQTT connect
    int pdpidx = 0;
    int status;
    char address[32];
    ret = sim7080g_get_app_network_active(sim7080g_handle, pdpidx, &status, address, sizeof(address));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get network active status");
        return ret;
    }
    if (status <= 0)
    {
        ESP_LOGE(TAG, "Network bearer not connected");
        return ESP_FAIL;
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
    ESP_LOGI(TAG, "Checking MQTT broker connection status");

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

esp_err_t sim7080g_mqtt_publish(const sim7080g_handle_t *sim7080g_handle,
                                const char *topic,
                                const char *message,
                                uint8_t qos,
                                bool retain)
{
    if (!sim7080g_handle || !topic || !message)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (qos > 2)
    {
        ESP_LOGE(TAG, "Invalid QoS value: %d", qos);
        return ESP_ERR_INVALID_ARG;
    }

    // Get message length
    size_t message_len = strlen(message);
    if (message_len == 0)
    {
        ESP_LOGW(TAG, "Empty message content");
        return ESP_ERR_INVALID_ARG;
    }

    // First construct and send the publish command
    char cmd[256] = {0};
    if (snprintf(cmd, sizeof(cmd), "AT+SMPUB=\"%s\",%zu,%d,%d\r\n",
                 topic,
                 message_len,
                 qos,
                 retain ? 1 : 0) >= sizeof(cmd))
    {
        ESP_LOGE(TAG, "Publish command too long");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Sending MQTT publish command: %s", cmd);

    // Send command and wait for '>' prompt
    int bytes_written = uart_write_bytes(sim7080g_handle->uart_config.port_num,
                                         cmd,
                                         strlen(cmd));
    if (bytes_written != strlen(cmd))
    {
        ESP_LOGE(TAG, "Failed to send complete publish command");
        return ESP_ERR_INVALID_STATE;
    }

    // Wait for '>' prompt with timeout
    char response[AT_RESPONSE_MAX_LEN] = {0};
    int bytes_read = uart_read_bytes(sim7080g_handle->uart_config.port_num,
                                     response,
                                     sizeof(response) - 1,
                                     pdMS_TO_TICKS(1000));

    if (bytes_read <= 0)
    {
        ESP_LOGE(TAG, "Timeout waiting for '>' prompt");
        return ESP_ERR_TIMEOUT;
    }

    response[bytes_read] = '\0';
    ESP_LOGD(TAG, "Response after publish command: %s", response);

    if (strstr(response, ">") == NULL)
    {
        ESP_LOGE(TAG, "Did not receive '>' prompt");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Now send the actual message content
    ESP_LOGI(TAG, "Sending message content (length %zu bytes)", message_len);
    ESP_LOGD(TAG, "Message: %s", message);

    bytes_written = uart_write_bytes(sim7080g_handle->uart_config.port_num,
                                     message,
                                     message_len);
    if (bytes_written != message_len)
    {
        ESP_LOGE(TAG, "Failed to send complete message content");
        return ESP_ERR_INVALID_STATE;
    }

    memset(response, 0, sizeof(response));
    bytes_read = uart_read_bytes(sim7080g_handle->uart_config.port_num,
                                 response,
                                 sizeof(response) - 1,
                                 pdMS_TO_TICKS(5000));

    if (bytes_read <= 0)
    {
        ESP_LOGE(TAG, "Timeout waiting for publish confirmation");
        return ESP_ERR_TIMEOUT;
    }

    response[bytes_read] = '\0';
    ESP_LOGD(TAG, "Publish response: %s", response);

    if (strstr(response, "ERROR") != NULL)
    {
        char *error_start = strstr(response, "+CME ERROR:");
        if (error_start)
        {
            int error_code;
            if (sscanf(error_start, "+CME ERROR: %d", &error_code) == 1)
            {
                ESP_LOGE(TAG, "Publish failed with CME ERROR: %d", error_code);
                return ESP_ERR_INVALID_RESPONSE;
            }
        }
        ESP_LOGE(TAG, "Publish failed with ERROR response");
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (strstr(response, "OK") == NULL)
    {
        ESP_LOGE(TAG, "Did not receive OK response");
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_LOGI(TAG, "Successfully published %zu bytes to topic '%s'",
             message_len, topic);
    return ESP_OK;
}

esp_err_t sim7080g_set_verbose_error_reporting(const sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle)
    {
        ESP_LOGE(TAG, "Invalid device handle");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting verbose error reporting");

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_CMEE,
                                AT_CMD_TYPE_WRITE,
                                "2",
                                response,
                                sizeof(response),
                                5000);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set verbose error reporting");
        return ret;
    }

    ESP_LOGI(TAG, "Verbose error reporting enabled");
    return ESP_OK;
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
        ESP_LOGE(TAG, "Send AT cmd failed: SIM7080G driver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (cmd == NULL || response == NULL || response_size == 0)
    {
        ESP_LOGE(TAG, "Send AT cmd failed: Invalid parameters");
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
        ESP_LOGE(TAG, "Send AT cmd failed: Invalid command type");
        return ESP_ERR_INVALID_ARG;
    }

    if (cmd_info->cmd_string == NULL)
    {
        ESP_LOGE(TAG, "Send AT cmd failed: Command string is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Format the AT command string
    if (type == AT_CMD_TYPE_WRITE && args != NULL)
    {
        if (snprintf(at_cmd, sizeof(at_cmd), "%s%s\r\n", cmd_info->cmd_string, args) >= sizeof(at_cmd))
        {
            ESP_LOGE(TAG, "Send AT cmd failed: AT command too long");
            return ESP_ERR_INVALID_SIZE;
        }
    }
    else
    {
        if (snprintf(at_cmd, sizeof(at_cmd), "%s\r\n", cmd_info->cmd_string) >= sizeof(at_cmd))
        {
            ESP_LOGE(TAG, "Send AT cmd failed: AT command too long");
            return ESP_ERR_INVALID_SIZE;
        }
    }

    esp_err_t ret = ESP_FAIL;
    for (int retry = 0; retry < AT_CMD_MAX_RETRIES; retry++)
    {
        ESP_LOGI(TAG, "Sending AT command (attempt %d/%d): %s", retry + 1, AT_CMD_MAX_RETRIES, at_cmd);
        ESP_LOGI(TAG, "Command description: %s", cmd->description);

        // Clear any pending data in UART buffers
        uart_flush(sim7080g_handle->uart_config.port_num);

        int bytes_written = uart_write_bytes(sim7080g_handle->uart_config.port_num,
                                             at_cmd, strlen(at_cmd));
        if (bytes_written < 0)
        {
            ESP_LOGE(TAG, "Send AT cmd failed: Failed to send AT command");
            ret = ESP_FAIL;
            continue;
        }

        int bytes_read = uart_read_bytes(sim7080g_handle->uart_config.port_num,
                                         response,
                                         response_size - 1,
                                         pdMS_TO_TICKS(timeout_ms));
        if (bytes_read < 0)
        {
            ESP_LOGE(TAG, "Send AT cmd failed: Failed to read AT command response");
            ret = ESP_FAIL;
            continue;
        }

        ESP_LOGI(TAG, "Received %d bytes. Raw Response: %s", bytes_read, response);

        // Ensure null-termination
        response[bytes_read] = '\0';

        // Check for expected response or error
        if (strstr(response, "OK") != NULL)
        {
            ESP_LOGI(TAG, "Send AT cmd SUCCESS: AT command send returned OK");
            return ESP_OK;
        }
        else if (strstr(response, "ERROR") != NULL)
        {
            ESP_LOGE(TAG, "Send AT cmd failed: AT command send returned ERROR");
            ret = ESP_FAIL;
            continue;
        }
        else if (bytes_read == 0)
        {
            ESP_LOGW(TAG, "Send AT cmd failed: AT command timeout");
            ret = ESP_ERR_TIMEOUT;
            continue;
        }
        else
        {
            ESP_LOGW(TAG, "Send AT cmd failed: Unexpected AT command response");
            ret = ESP_FAIL;
        }
    }

    ESP_LOGE(TAG, "Send AT cmd failed after %d attempts", AT_CMD_MAX_RETRIES);
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

/**
 * @brief Check if current device MQTT parameters match those in the handle config
 *
 * @param sim7080g_handle Pointer to initialized device handle
 * @param params_match_out Pointer to store match result
 * @return esp_err_t ESP_OK if check completed successfully
 */
static esp_err_t sim7080g_mqtt_check_parameters_match(const sim7080g_handle_t *sim7080g_handle,
                                                      bool *params_match_out)
{
    if (!sim7080g_handle || !params_match_out)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize output
    *params_match_out = false;

    // Get current device parameters
    mqtt_parameters_t current_params;
    esp_err_t ret = sim7080g_mqtt_get_parameters(sim7080g_handle, &current_params);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get current MQTT parameters");
        return ret;
    }

    // Compare relevant parameters from handle config
    bool match = true; // Start true, set false if any mismatch

    // Check URL
    if (strcmp(current_params.broker_url, sim7080g_handle->mqtt_config.broker_url) != 0)
    {
        ESP_LOGD(TAG, "URL mismatch - Current: %s, Config: %s",
                 current_params.broker_url, sim7080g_handle->mqtt_config.broker_url);
        match = false;
    }

    // Check port
    if (current_params.port != sim7080g_handle->mqtt_config.port)
    {
        ESP_LOGD(TAG, "Port mismatch - Current: %d, Config: %d",
                 current_params.port, sim7080g_handle->mqtt_config.port);
        match = false;
    }

    // Check client ID
    if (strcmp(current_params.client_id, sim7080g_handle->mqtt_config.client_id) != 0)
    {
        ESP_LOGD(TAG, "Client ID mismatch - Current: %s, Config: %s",
                 current_params.client_id, sim7080g_handle->mqtt_config.client_id);
        match = false;
    }

    // Check username
    if (strcmp(current_params.username, sim7080g_handle->mqtt_config.username) != 0)
    {
        ESP_LOGD(TAG, "Username mismatch - Current: %s, Config: %s",
                 current_params.username, sim7080g_handle->mqtt_config.username);
        match = false;
    }

    // Check password
    if (strcmp(current_params.client_password, sim7080g_handle->mqtt_config.client_password) != 0)
    {
        ESP_LOGD(TAG, "Password mismatch - Current: %s, Config: %s",
                 current_params.client_password, sim7080g_handle->mqtt_config.client_password);
        match = false;
    }

    *params_match_out = match;

    if (match)
    {
        ESP_LOGI(TAG, "MQTT parameters match current config");
    }
    else
    {
        ESP_LOGI(TAG, "MQTT parameters differ from current config");
    }

    return ESP_OK;
}

/**
 * @brief Parse MQTT parameters from bulk response
 */
esp_err_t sim7080g_mqtt_get_parameters(const sim7080g_handle_t *sim7080g_handle,
                                       mqtt_parameters_t *params_out)
{
    if (!sim7080g_handle || !params_out)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize output structure
    memset(params_out, 0, sizeof(mqtt_parameters_t));

    char response[512] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_SMCONF,
                                AT_CMD_TYPE_READ,
                                NULL,
                                response,
                                sizeof(response),
                                5000);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read MQTT parameters");
        return ret;
    }

    // Parse each line of the response
    char *line = strtok(response, "\r\n");
    bool found_any = false;

    while (line != NULL)
    {
        // Skip the initial +SMCONF: line
        if (strncmp(line, "CLIENTID:", 9) == 0)
        {
            // Parse string parameter format: 'CLIENTID: "value"'
            char value[256];
            if (sscanf(line + 9, " \"%[^\"]\"", value) == 1)
            {
                strncpy(params_out->client_id, value, sizeof(params_out->client_id) - 1);
                found_any = true;
            }
        }
        else if (strncmp(line, "URL:", 4) == 0)
        {
            // Parse URL and port format: 'URL: "url",port'
            char url[256];
            int port;
            if (sscanf(line + 4, " \"%[^\"]\"%*[,]%d", url, &port) == 2)
            {
                strncpy(params_out->broker_url, url, sizeof(params_out->broker_url) - 1);
                params_out->port = (uint16_t)port;
                found_any = true;
            }
        }
        else if (strncmp(line, "USERNAME:", 9) == 0)
        {
            char value[256];
            if (sscanf(line + 9, " \"%[^\"]\"", value) == 1)
            {
                strncpy(params_out->username, value, sizeof(params_out->username) - 1);
                found_any = true;
            }
        }
        else if (strncmp(line, "PASSWORD:", 9) == 0)
        {
            char value[256];
            if (sscanf(line + 9, " \"%[^\"]\"", value) == 1)
            {
                strncpy(params_out->client_password, value, sizeof(params_out->client_password) - 1);
                found_any = true;
            }
        }
        else if (strncmp(line, "KEEPTIME:", 9) == 0)
        {
            int value;
            if (sscanf(line + 9, " %d", &value) == 1)
            {
                params_out->keepalive = (uint16_t)value;
                found_any = true;
            }
        }
        else if (strncmp(line, "CLEANSS:", 8) == 0)
        {
            int value;
            if (sscanf(line + 8, " %d", &value) == 1)
            {
                params_out->clean_session = (value == 1);
                found_any = true;
            }
        }
        else if (strncmp(line, "QOS:", 4) == 0)
        {
            int value;
            if (sscanf(line + 4, " %d", &value) == 1)
            {
                params_out->qos = (uint8_t)value;
                found_any = true;
            }
        }
        else if (strncmp(line, "RETAIN:", 7) == 0)
        {
            int value;
            if (sscanf(line + 7, " %d", &value) == 1)
            {
                params_out->retain = (value == 1);
                found_any = true;
            }
        }
        else if (strncmp(line, "SUBHEX:", 7) == 0)
        {
            int value;
            if (sscanf(line + 7, " %d", &value) == 1)
            {
                params_out->sub_hex = (value == 1);
                found_any = true;
            }
        }
        else if (strncmp(line, "ASYNCMODE:", 10) == 0)
        {
            int value;
            if (sscanf(line + 10, " %d", &value) == 1)
            {
                params_out->async_mode = (value == 1);
                found_any = true;
            }
        }

        line = strtok(NULL, "\r\n");
    }

    if (!found_any)
    {
        ESP_LOGW(TAG, "No MQTT parameters found in response");
        return ESP_ERR_NOT_FOUND;
    }

    // Log all retrieved parameters
    ESP_LOGI(TAG, "Current MQTT Parameters:");
    ESP_LOGI(TAG, "  URL: %s", params_out->broker_url);
    ESP_LOGI(TAG, "  Port: %d", params_out->port);
    ESP_LOGI(TAG, "  Client ID: %s", params_out->client_id);
    ESP_LOGI(TAG, "  Username: %s", params_out->username);
    ESP_LOGI(TAG, "  Keepalive: %d", params_out->keepalive);
    ESP_LOGI(TAG, "  Clean Session: %d", params_out->clean_session);
    ESP_LOGI(TAG, "  QoS: %d", params_out->qos);
    ESP_LOGI(TAG, "  Retain: %d", params_out->retain);
    ESP_LOGI(TAG, "  SubHex: %d", params_out->sub_hex);
    ESP_LOGI(TAG, "  AsyncMode: %d", params_out->async_mode);

    return ESP_OK;
}

// ----------------------------------------------------- NEW API FXNS ----------------------------------------------------- //
// -----------------------------------------------------              ----------------------------------------------------- //

// Check:
// - AT+CPIN? (SIM status)
// - AT+CSQ (Signal quality)
// - AT+CPSI? (System mode)
esp_err_t sim7080g_is_physical_layer_connected(const sim7080g_handle_t *handle, bool *connected)
{
    if (!handle || !connected)
    {
        ESP_LOGE(TAG, "Is physical layer connected fail: Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    *connected = false;
    esp_err_t err;
    char response[AT_RESPONSE_MAX_LEN] = {0};

    // Check SIM card status with AT+CPIN?
    err = send_at_cmd(handle, &AT_CPIN, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 5000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "SIM card status check failed: %s", esp_err_to_name(err));
        return err;
    }

    if (strstr(response, "READY") == NULL)
    {
        ESP_LOGE(TAG, "SIM card not ready: %s", response);
        return ESP_FAIL;
    }

    // Check signal quality with AT+CSQ
    memset(response, 0, sizeof(response));
    err = send_at_cmd(handle, &AT_CSQ, AT_CMD_TYPE_EXECUTE, NULL, response, sizeof(response), 5000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Signal quality check failed: %s", esp_err_to_name(err));
        return err;
    }

    int rssi, ber;
    char *csq_response = strstr(response, "+CSQ:");
    if (!csq_response || sscanf(csq_response, "+CSQ: %d,%d", &rssi, &ber) != 2)
    {
        ESP_LOGE(TAG, "Failed to parse signal quality response: %s", response);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Check if signal strength is acceptable (RSSI >= -105 dBm, corresponds to CSQ >= 5)
    // and not unknown (99)
    if (rssi >= 1 && rssi != 99)
    {
        *connected = true;
        ESP_LOGI(TAG, "Physical layer connected (RSSI: %d, BER: %d)", rssi, ber);
    }
    else
    {
        ESP_LOGW(TAG, "Physical layer not connected (RSSI: %d, BER: %d)", rssi, ber);
    }

    // TODO - CPSI Command check

    return ESP_OK;
}

// Check:
// - AT+CEREG? (Network registration)
// - AT+CGATT? (GPRS attach status)
// - AT+CGDCONT? (PDP context)
esp_err_t sim7080g_is_data_link_layer_connected(const sim7080g_handle_t *handle, bool *connected)
{
    if (!handle || !connected)
    {
        ESP_LOGE(TAG, "Is data link layer connected fail: Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    *connected = false;
    esp_err_t err;
    char response[AT_RESPONSE_MAX_LEN] = {0};

    // TODO - this
    // Check network registration status with AT+CEREG?
    err = send_at_cmd(handle, &AT_CEREG, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 5000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Network registration status check failed: %s", esp_err_to_name(err));
        return err;
    }

    // Parse just the mode and status
    int n, stat;
    char *cereg_response = strstr(response, "+CEREG:");
    if (!cereg_response || sscanf(cereg_response, "+CEREG: %d,%d", &n, &stat) != 2)
    {
        ESP_LOGE(TAG, "Failed to parse network registration status: %s", response);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Check registration status (1 = home network, 5 = roaming)
    if (stat == 1 || stat == 5)
    {
        ESP_LOGI(TAG, "Network registration ok (status: %s)",
                 stat == 1 ? "home network" : "roaming");
    }
    else
    {
        const char *status_str;
        switch (stat)
        {
        case 0:
            status_str = "not registered, not searching";
            break;
        case 2:
            status_str = "searching";
            break;
        case 3:
            status_str = "registration denied";
            break;
        case 4:
            status_str = "unknown";
            break;
        default:
            status_str = "invalid status";
            break;
        }
        ESP_LOGE(TAG, "Network not registered: %s", status_str);
        return ESP_OK; // Not an error, just not registered
    }

    // Check GPRS attachment status with AT+CGATT?
    err = send_at_cmd(handle, &AT_CGATT, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 15000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPRS attach status check failed: %s", esp_err_to_name(err));
        return err;
    }

    int attach_status;
    char *cgatt_response = strstr(response, "+CGATT:");
    if (!cgatt_response || sscanf(cgatt_response, "+CGATT: %d", &attach_status) != 1)
    {
        ESP_LOGE(TAG, "Failed to parse GPRS attach status: %s", response);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (!attach_status)
    {
        ESP_LOGE(TAG, "Data link layer not connected: Not attached to GPRS");
        return ESP_OK;
    }

    // Check operator info with AT+COPS?
    memset(response, 0, sizeof(response));
    err = send_at_cmd(handle, &AT_COPS, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 5000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Operator info check failed: %s", esp_err_to_name(err));
        return err;
    }

    int mode, format, act_c;
    char operator_str[64] = {0};
    char *cops_response = strstr(response, "+COPS:");

    if (cops_response &&
        sscanf(cops_response, "+COPS: %d,%d,\"%63[^\"]\",%d", &mode, &format, operator_str, &act_c) >= 3)
    {
        if (mode != 2)
        { // 2 = deregistered
            *connected = true;
            ESP_LOGI(TAG, "Data link layer connected (Operator: %s)", operator_str);
        }
        else
        {
            ESP_LOGE(TAG, "Data link layer not connected (Operator mode: %d)", mode);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to parse operator info: %s", response);
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

// Check:
// AT+CNACT? (PDP context)
// AT+CGPADDR (IP address)
esp_err_t sim7080g_is_network_layer_connected(const sim7080g_handle_t *handle, bool *connected)
{
    if (!handle || !connected)
    {
        ESP_LOGE(TAG, "Is network layer connected fail: Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    *connected = false;
    esp_err_t err;
    char response[AT_RESPONSE_MAX_LEN] = {0};

    // Check PDP context status with AT+CNACT?
    err = send_at_cmd(handle, &AT_CNACT, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 20000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PDP context status check failed: %s", esp_err_to_name(err));
        return err;
    }

    // Parse response for the first PDP context (index 0)
    char *cnact_response = strstr(response, "+CNACT:");
    if (cnact_response)
    {
        int pdp_idx, status;
        char ip_addr[64] = {0};

        if (sscanf(cnact_response, "+CNACT: %d,%d,\"%63[^\"]\"",
                   &pdp_idx, &status, ip_addr) >= 2)
        {
            if (pdp_idx == 0 && status > 0 && strlen(ip_addr) > 0)
            {
                *connected = true;
                ESP_LOGI(TAG, "Network layer connected (IP: %s)", ip_addr);
            }
            else
            {
                ESP_LOGE(TAG, "Network layer not connected (PDP status: %d)", status);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse PDP context status: %s", response);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }
    else
    {
        ESP_LOGE(TAG, "No CNACT response found");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // TODO - this
    //  Check IP address with AT+CGPADDR

    return ESP_OK;
}

// Check:
// AT+CASTATE (TCP/UDP connection status)
// TODO - FIX THIS (if its even needed?)
// esp_err_t sim7080g_is_transport_layer_connected(const sim7080g_handle_t *handle, bool *connected)
// {
//     if (!handle || !connected)
//     {
//         ESP_LOGE(TAG, "Is transport layer connected fail: Invalid parameters");
//         return ESP_ERR_INVALID_ARG;
//     }

//     // TODO - this
//     // Check TCP/UDP connection status with AT+CASTATE

//     *connected = false;
//     char response[AT_RESPONSE_MAX_LEN] = {0};
//     esp_err_t err;

//     err = send_at_cmd(handle, &AT_CASTATE, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 5000);
//     if (err != ESP_OK)
//     {
//         ESP_LOGE(TAG, "Transport layer status check failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     char *castate_response = strstr(response, "+CASTATE:");
//     if (castate_response)
//     {
//         int status;
//         if (sscanf(castate_response, "+CASTATE: %d", &status) == 1)
//         {
//             // Status values: 0=disconnected, 1=connected
//             if (status > 0)
//             {
//                 *connected = true;
//                 ESP_LOGI(TAG, "Transport layer connected (CASTATE: %d)", status);
//             }
//             else
//             {
//                 ESP_LOGE(TAG, "Transport layer not connected (CASTATE: %d)", status);
//             }
//         }
//         else
//         {
//             ESP_LOGE(TAG, "Failed to parse transport layer status: %s", response);
//             return ESP_ERR_INVALID_RESPONSE;
//         }
//     }
//     else
//     {
//         ESP_LOGE(TAG, "No CASTATE response found");
//         return ESP_ERR_INVALID_RESPONSE;
//     }

//     return ESP_OK;
// }

// Check:
// AT+SMSTATE (MQTT connection status)
esp_err_t sim7080g_is_application_layer_connected(const sim7080g_handle_t *handle, bool *connected)
{
    if (!handle || !connected)
    {
        ESP_LOGE(TAG, "Is application layer connected fail: Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    *connected = false;
    esp_err_t err;
    char response[AT_RESPONSE_MAX_LEN] = {0};

    // Check MQTT connection status with AT+SMSTATE?
    err = send_at_cmd(handle, &AT_SMSTATE, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 5000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "MQTT status check failed: %s", esp_err_to_name(err));
        return err;
    }

    char *smstate_response = strstr(response, "+SMSTATE:");
    if (smstate_response)
    {
        int status;
        if (sscanf(smstate_response, "+SMSTATE: %d", &status) == 1)
        {
            // Status values: 0=disconnected, 1=connected, 2=connected with session
            if (status > 0)
            {
                *connected = true;
                ESP_LOGI(TAG, "Application layer connected (MQTT Status: %d)", status);
            }
            else
            {
                ESP_LOGE(TAG, "Application layer not connected (MQTT Status: %d)", status);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse MQTT status: %s", response);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }
    else
    {
        ESP_LOGE(TAG, "No SMSTATE response found");
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

static esp_err_t sim7080g_echo_off(const sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle || !sim7080g_handle->uart_initialized)
    {
        ESP_LOGE(TAG, "SIM7080G driver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    char response[AT_RESPONSE_MAX_LEN] = {0};
    esp_err_t ret = send_at_cmd(sim7080g_handle,
                                &AT_ECHO_OFF,
                                AT_CMD_TYPE_EXECUTE,
                                NULL,
                                response,
                                sizeof(response),
                                5000);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to turn off echo");
        return ret;
    }

    ESP_LOGI(TAG, "Echo off");
    return ESP_OK;
}

// ---------------------  INTERNAL TESTING FXNs  ---------------------//

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