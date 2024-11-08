#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include "sim7080g_uart.h"
#include "sim7080g_commands.h"
#include "sim7080g_at_cmd_responses.h"

// --------- Used for the send AT command fxn --------- //
#define AT_RESPONSE_MAX_LEN 256U
#define AT_CMD_MAX_LEN 256U
#define AT_CMD_MAX_RETRIES 4U

static const char *TAG = "SIM7080G Commands";

// --------------------------------------- STATIC / HELPER FXNS --------------------------------------- //

// send AT command
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

// parse AT response

// Log MQTT configuration parameters

// --------------------------------------- FXNS to use SIM7080G AT Commands --------------------------------------- //

// Cycle CFUN - soft reset (CFUN)

// Test AT communication with device (AT)

// Disable command Echo (ATE0)

// Enable verbose error reporting (CMEE)

// Check SIM status (CPIN)
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

// Check signal quality (CSQ)

// Get GPRS attached status

// Get operator info

// Get APN
// Set APN

// Get PDP context status (CNACT)
// Set (activate OR deactivate) PDP context (CNACT)

// Get MQTT configuration parameters (SMCONF)
// Set MQTT configuration parameters (SMCONF)

// Connect to MQTT broker
// Disconnect from MQTT broker

// Publish to MQTT broker
// Subscribe to MQTT broker
