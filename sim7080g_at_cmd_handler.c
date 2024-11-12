#include <inttypes.h>
#include <esp_err.h>
#include <esp_log.h>

#include "sim7080g_at_cmd_handler.h"

static const char *TAG = "SIM7080G Device Commands";

// --------------------------------- STATIC / HELPER FXNS --------------------------------------- //

esp_err_t validate_at_cmd_params(const sim7080g_handle_t *sim7080g_handle,
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

esp_err_t get_cmd_info(const at_cmd_t *cmd,
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

esp_err_t format_at_cmd(const at_cmd_info_t *cmd_info,
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
    else if ((AT_CMD_TYPE_WRITE == type) && (NULL == args))
    {
        // A 'write' type command CANNOT have no arguments (its format is AT+<CMD>=<ARGS> )
        return ESP_FAIL;
    }
    // Handle EXECUTE command with arguments (special case for commands like ATE)
    else if ((AT_CMD_TYPE_EXECUTE == type) && (NULL != args))
    {
        ESP_LOGI(TAG, "AT_CMD_TYPE_EXECUTE with argument: %s", args);
        written_len = snprintf(at_cmd, at_cmd_size, "%s%s\r\n",
                               cmd_info->cmd_string, args);
    }
    else // IF its NOT a write type command
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

esp_err_t send_receive_at_cmd(const sim7080g_handle_t *sim7080g_handle,
                              const char *at_cmd,
                              char *response,
                              size_t response_size,
                              uint32_t timeout_ms,
                              at_cmd_type_t cmd_type)
{
    if ((NULL == sim7080g_handle) || (NULL == at_cmd) ||
        (NULL == response) || (0U == response_size))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Flush UART before sending new command
    uart_flush(sim7080g_handle->uart_config.port_num);

    // Send command
    int32_t bytes_written = uart_write_bytes(sim7080g_handle->uart_config.port_num,
                                             at_cmd, strlen(at_cmd));
    if (bytes_written < 0)
    {
        ESP_LOGE(TAG, "Failed to send command");
        return ESP_FAIL;
    }

    uint32_t start_time = pdTICKS_TO_MS(xTaskGetTickCount());
    size_t total_bytes_read = 0U;
    bool response_complete = false;
    char temp_buffer[AT_CMD_UART_READ_CHUNK_SIZE];

    // Clear response buffer
    memset(response, 0, response_size);

    while ((pdTICKS_TO_MS(xTaskGetTickCount()) - start_time) < timeout_ms)
    {
        // Read in smaller chunks
        int32_t bytes_read = uart_read_bytes(sim7080g_handle->uart_config.port_num,
                                             (uint8_t *)temp_buffer,
                                             sizeof(temp_buffer) - 1U,
                                             pdMS_TO_TICKS(AT_CMD_UART_READ_INTERVAL_MS));

        if (bytes_read > 0)
        {
            // Ensure we don't overflow the response buffer
            if ((total_bytes_read + (size_t)bytes_read) >= response_size)
            {
                return ESP_FAIL;
            }

            // Append new data to response
            memcpy(response + total_bytes_read, temp_buffer, (size_t)bytes_read);
            total_bytes_read += (size_t)bytes_read;
            response[total_bytes_read] = '\0';

            // Check for response completion based on command type
            if (AT_CMD_TYPE_WRITE == cmd_type)
            {
                // Write commands only expect OK/ERROR
                if (strstr(response, AT_CMD_RESPONSE_TERMINATOR_OK) ||
                    strstr(response, AT_CMD_RESPONSE_TERMINATOR_ERROR))
                {
                    response_complete = true;
                    break;
                }
            }
            else
            {
                // For other commands, we need to look for both data and OK/ERROR
                const char *ok_pos = strstr(response, AT_CMD_RESPONSE_TERMINATOR_OK);
                const char *error_pos = strstr(response, AT_CMD_RESPONSE_TERMINATOR_ERROR);

                // Special handling for read/test commands that expect data
                if (AT_CMD_TYPE_READ == cmd_type || AT_CMD_TYPE_TEST == cmd_type)
                {
                    // Look for command-specific response prefix and terminator
                    if ((ok_pos || error_pos) && strstr(response, "+"))
                    {
                        response_complete = true;
                        break;
                    }
                }
                else if (ok_pos || error_pos)
                {
                    response_complete = true;
                    break;
                }
            }
        }

        // Small delay to prevent tight polling
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (!response_complete)
    {
        ESP_LOGW(TAG, "Response timeout or incomplete after %" PRIu32 " ms", timeout_ms);
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGD(TAG, "Response received in %" PRIu32 " ms",
             pdTICKS_TO_MS(xTaskGetTickCount()) - start_time);
    ESP_LOGD(TAG, "Received %zu bytes: %s", total_bytes_read, response);

    return ESP_OK;
}
/// @brief Check for OK or ERROR in the response, and use custome AT response type to differentiate (prevents confusion with driver/system errors and device response as 'error')
at_response_status_t check_at_response_status(const char *response)
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

esp_err_t read_uart_response(const sim7080g_handle_t *sim7080g_handle,
                             char *response,
                             size_t response_size,
                             uint32_t timeout_ms,
                             at_cmd_type_t cmd_type)
{
    if ((NULL == sim7080g_handle) || (NULL == response) || (0U == response_size))
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t start_time = pdTICKS_TO_MS(xTaskGetTickCount());
    size_t total_bytes_read = 0U;
    bool response_complete = false;
    char temp_buffer[AT_CMD_UART_READ_CHUNK_SIZE];

    // Clear response buffer
    memset(response, 0, response_size);

    while ((pdTICKS_TO_MS(xTaskGetTickCount()) - start_time) < timeout_ms)
    {
        // Read in smaller chunks
        int32_t bytes_read = uart_read_bytes(sim7080g_handle->uart_config.port_num,
                                             (uint8_t *)temp_buffer,
                                             sizeof(temp_buffer) - 1U,
                                             pdMS_TO_TICKS(AT_CMD_UART_READ_INTERVAL_MS));

        if (bytes_read > 0)
        {
            // Ensure we don't overflow the response buffer
            if ((total_bytes_read + (size_t)bytes_read) >= response_size)
            {
                return ESP_FAIL;
            }

            // Append new data to response
            memcpy(response + total_bytes_read, temp_buffer, (size_t)bytes_read);
            total_bytes_read += (size_t)bytes_read;
            response[total_bytes_read] = '\0';

            // Check for response completion based on command type
            if (AT_CMD_TYPE_WRITE == cmd_type)
            {
                // Write commands only expect OK/ERROR
                if (strstr(response, AT_CMD_RESPONSE_TERMINATOR_OK) ||
                    strstr(response, AT_CMD_RESPONSE_TERMINATOR_ERROR))
                {
                    response_complete = true;
                    break;
                }
            }
            else
            {
                // For other commands, we need to look for both data and OK/ERROR
                const char *ok_pos = strstr(response, AT_CMD_RESPONSE_TERMINATOR_OK);
                const char *error_pos = strstr(response, AT_CMD_RESPONSE_TERMINATOR_ERROR);

                // Special handling for read/test commands that expect data
                if (AT_CMD_TYPE_READ == cmd_type || AT_CMD_TYPE_TEST == cmd_type)
                {
                    // Look for command-specific response prefix
                    if ((ok_pos || error_pos) && strstr(response, "+"))
                    {
                        response_complete = true;
                        break;
                    }
                }
                else if (ok_pos || error_pos)
                {
                    response_complete = true;
                    break;
                }
            }
        }

        // Small delay to prevent tight polling
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (!response_complete)
    {
        ESP_LOGW(TAG, "Response timeout or incomplete after %" PRIu32 " ms", timeout_ms);
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGD(TAG, "Response received in %" PRIu32 " ms",
             pdTICKS_TO_MS(xTaskGetTickCount()) - start_time);
    return ESP_OK;
}

esp_err_t send_at_cmd_with_parser(const sim7080g_handle_t *sim7080g_handle,
                                  const at_cmd_t *cmd,
                                  at_cmd_type_t type,
                                  const char *args,
                                  void *parsed_response,
                                  const at_cmd_handler_config_t *handler_config)
{
    char at_cmd[AT_CMD_MAX_LEN] = {0};
    char response[AT_CMD_RESPONSE_MAX_LEN] = {0};
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

    for (unsigned int retry = 0U; retry < AT_CMD_MAX_RETRIES; retry++)
    {
        if (retry > 0U)
        {
            vTaskDelay(pdMS_TO_TICKS(handler_config->retry_delay_ms));
            ESP_LOGI(TAG, "Retrying command (attempt %u/%u)", // Change to %u
                     retry + 1U, AT_CMD_MAX_RETRIES);
        }
        else
        {
            ESP_LOGI(TAG, "Sending command RAW: %s", at_cmd);
        }

        ret = send_receive_at_cmd(sim7080g_handle, at_cmd, response,
                                  sizeof(response), handler_config->timeout_ms, type);
        if (ESP_OK != ret)
        {
            continue;
        }

        ret = check_at_response_status(response);
        if (ESP_OK != ret)
        {
            continue;
        }

        // print raw response:

        ESP_LOGI(TAG, "Raw response: %s", response);

        // Parse response if parser provided
        if ((NULL != handler_config->parser) && (NULL != parsed_response))
        {
            ret = handler_config->parser(response, parsed_response, type);
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
