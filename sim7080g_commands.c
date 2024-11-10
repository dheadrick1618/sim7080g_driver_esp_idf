#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include "sim7080g_uart.h"
#include "sim7080g_commands.h"
#include "sim7080g_at_cmd_responses.h"

// --------- Used for the send AT command fxn --------- //
#define AT_RESPONSE_MAX_LEN 256U
#define AT_CMD_MAX_LEN 256U
#define AT_CMD_MAX_RETRIES 4U // TODO - Maybe give each command a specific retry count

// Specific type for device response to AT commands (differentiate a system/driver error with and device response status)
typedef enum
{
    AT_RESPONSE_OK = 0,
    AT_RESPONSE_ERROR,
    AT_RESPONSE_UNEXPECTED
} at_response_status_t;

static const char *TAG = "SIM7080G Device Commands";

// --------------------------------------- STATIC / HELPER FXNS --------------------------------------- //

/// @brief Verify handle and pointers are not NULL, that UART is initialized, and that response buffer is not empty
static esp_err_t validate_at_cmd_params(const sim7080g_handle_t *sim7080g_handle,
                                        const at_cmd_t *cmd,
                                        const char *response,
                                        size_t response_size)
{
    if ((NULL == sim7080g_handle) || (NULL == cmd) ||
        (NULL == response) || (0U == response_size))
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (!sim7080g_handle->uart_initialized)
    {
        ESP_LOGE(TAG, "UART not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

/// @brief Get the command info structure for the specified command type (test, read, write, execute), and validate command string
static esp_err_t get_cmd_info(const at_cmd_t *cmd,
                              at_cmd_type_t type,
                              const at_cmd_info_t **cmd_info)
{
    if (NULL == cmd)
    {
        return ESP_ERR_INVALID_ARG;
    }

    switch (type)
    {
    case AT_CMD_TYPE_TEST:
        *cmd_info = &cmd->test;
        break;
    case AT_CMD_TYPE_READ:
        *cmd_info = &cmd->read;
        break;
    case AT_CMD_TYPE_WRITE:
        *cmd_info = &cmd->write;
        break;
    case AT_CMD_TYPE_EXECUTE:
        *cmd_info = &cmd->execute;
        break;
    default:
        ESP_LOGE(TAG, "Invalid command type");
        return ESP_ERR_INVALID_ARG;
    }

    if (NULL == (*cmd_info)->cmd_string)
    {
        ESP_LOGE(TAG, "Command string is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/// @brief If the command is write type, format outgoing command string with its arguments. Otherwise just append \r\n
/// @note ensure at_cmd buffer is large enough to hold the formatted command (fits in buffer)
static esp_err_t format_at_cmd(const at_cmd_info_t *cmd_info,
                               at_cmd_type_t type,
                               const char *args,
                               char *at_cmd,
                               size_t at_cmd_size)
{
    int32_t written_len;

    if ((NULL == cmd_info) || (NULL == at_cmd))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ((AT_CMD_TYPE_WRITE == type) && (NULL != args))
    {
        written_len = snprintf(at_cmd, at_cmd_size, "%s%s\r\n",
                               cmd_info->cmd_string, args);
    }
    else
    {
        written_len = snprintf(at_cmd, at_cmd_size, "%s\r\n",
                               cmd_info->cmd_string);
    }

    if ((written_len < 0) || ((size_t)written_len >= at_cmd_size))
    {
        ESP_LOGE(TAG, "Command formatting failed or buffer too small");
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

/// @brief Send an AT command and read ther response using the esp idf UART driver (assumed already initialized)
/// @note This is NOT the fxn that API fxns should call, instead this is a helper fxn that only handles:
/// @note UART flush
/// @note UART write, read and timeout handling
/// @note  - it simply updates the response buffer with the raw response
static esp_err_t send_receive_at_cmd(const sim7080g_handle_t *sim7080g_handle,
                                     const char *at_cmd,
                                     char *response,
                                     size_t response_size,
                                     uint32_t timeout_ms)
{
    if ((NULL == sim7080g_handle) || (NULL == at_cmd) ||
        (NULL == response) || (0U == response_size))
    {
        return ESP_ERR_INVALID_ARG;
    }

    uart_flush(sim7080g_handle->uart_config.port_num);

    int32_t bytes_written = uart_write_bytes(sim7080g_handle->uart_config.port_num,
                                             at_cmd, strlen(at_cmd));
    if (bytes_written < 0)
    {
        ESP_LOGE(TAG, "Failed to send command");
        return ESP_FAIL;
    }

    int32_t bytes_read = uart_read_bytes(sim7080g_handle->uart_config.port_num,
                                         response,
                                         response_size - 1U,
                                         pdMS_TO_TICKS(timeout_ms));
    if (bytes_read < 0)
    {
        ESP_LOGE(TAG, "Failed to read response");
        return ESP_FAIL;
    }

    if (0 == bytes_read)
    {
        ESP_LOGW(TAG, "Command timeout");
        return ESP_ERR_TIMEOUT;
    }

    response[bytes_read] = '\0';
    ESP_LOGD(TAG, "Received %d bytes: %s", bytes_read, response);

    return ESP_OK;
}

// Check for OK or ERROR in the response, and use custome AT response type to differentiate (prevents confusion with driver/system errors and device response as 'error')
static at_response_status_t check_at_response_status(const char *response)
{
    if (NULL == response)
    {
        return AT_RESPONSE_UNEXPECTED;
    }

    if (NULL != strstr(response, "OK"))
    {
        return AT_RESPONSE_OK;
    }

    if (NULL != strstr(response, "ERROR"))
    {
        return AT_RESPONSE_ERROR;
    }

    return AT_RESPONSE_UNEXPECTED;
}

esp_err_t send_at_cmd_with_parser(const sim7080g_handle_t *sim7080g_handle,
                                  const at_cmd_t *cmd,
                                  at_cmd_type_t type,
                                  const char *args,
                                  void *parsed_response,
                                  const at_cmd_handler_config_t *handler_config)
{
    char at_cmd[AT_CMD_MAX_LEN] = {0};
    char response[AT_RESPONSE_MAX_LEN] = {0};
    const at_cmd_info_t *cmd_info = NULL;
    esp_err_t ret;

    ret = validate_at_cmd_params(sim7080g_handle, cmd, response, sizeof(response));
    if (ESP_OK != ret)
    {
        return ret;
    }

    ret = get_cmd_info(cmd, type, &cmd_info);
    if (ESP_OK != ret)
    {
        return ret;
    }

    ret = format_at_cmd(cmd_info, type, args, at_cmd, sizeof(at_cmd));
    if (ESP_OK != ret)
    {
        return ret;
    }

    for (uint32_t retry = 0U; retry < AT_CMD_MAX_RETRIES; retry++)
    {
        if (retry > 0U)
        {
            vTaskDelay(pdMS_TO_TICKS(handler_config->retry_delay_ms));
            ESP_LOGI(TAG, "Retrying command (attempt %u/%u)",
                     retry + 1U, AT_CMD_MAX_RETRIES);
        }

        ret = send_receive_at_cmd(sim7080g_handle, at_cmd, response,
                                  sizeof(response), handler_config->timeout_ms);
        if (ESP_OK != ret)
        {
            continue;
        }

        ret = check_at_response_status(response);
        if (ESP_OK != ret)
        {
            continue;
        }

        // Parse response if parser provided
        if ((NULL != handler_config->parser) && (NULL != parsed_response))
        {
            ret = handler_config->parser(response, parsed_response);
            if (ESP_OK != ret)
            {
                ESP_LOGE(TAG, "Failed to parse response");
                return ESP_ERR_INVALID_RESPONSE;
            }
        }

        return ESP_OK;
    }

    ESP_LOGE(TAG, "Command failed after %u attempts", AT_CMD_MAX_RETRIES);
    return ESP_FAIL;
}

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
