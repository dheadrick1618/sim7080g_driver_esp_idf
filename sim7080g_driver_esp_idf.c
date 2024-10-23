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
static esp_err_t sim7080g_init_mqtt(const sim7080g_mqtt_config_t sim7080g_mqtt_config);
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
    sim7080g_handle->uart_config = sim7080g_uart_config;

    sim7080g_handle->mqtt_config = sim7080g_mqtt_config;

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

    err = sim7080g_init_mqtt(sim7080g_handle->mqtt_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error init MQTT: %s", esp_err_to_name(err));
        return err;
    }

    sim7080g_log_config_params(sim7080g_handle);
    sim7080g_handle->initialized = true;
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
                                //TODO - 75 seconds is spec 

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

// ---------------------  INTERNAL HELPER / STATIC FXNs  ---------------------//

static esp_err_t send_at_cmd(const sim7080g_handle_t *sim7080g_handle,
                             const at_cmd_t *cmd,
                             at_cmd_type_t type,
                             const char *args,
                             char *response,
                             size_t response_size,
                             uint32_t timeout_ms)
{
    if (!sim7080g_handle || !sim7080g_handle->initialized)
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

/// @brief Send series of AT commands to configure the device MQTT values to match the driver handle config values
/// @param sim7080g_mqtt_config
/// @return
static esp_err_t sim7080g_init_mqtt(const sim7080g_mqtt_config_t sim7080g_mqtt_config)
{

    return ESP_OK;
}

// ---------------------  TESTING FXNs  ---------------------//

bool sim7080g_test_uart_loopback(sim7080g_handle_t *sim7080g_handle)
{
    if (!sim7080g_handle->initialized)
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