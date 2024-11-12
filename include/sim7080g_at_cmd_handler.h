// Fxns that were originally static in sim7080g_at_cmd_handler.c are now public in sim7080g_at_cmd_handler.h - makes testing easier
#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <string.h>
#include "sim7080g_uart.h"
#include "sim7080g_types.h"
#include "sim7080g_at_cmds.h"

// --------- Used for the send AT command fxn --------- //
#define AT_CMD_RESPONSE_MAX_LEN 256U
#define AT_CMD_MAX_LEN 256U
#define AT_CMD_MAX_RETRIES 5U // TODO - Maybe give each command a specific retry count

#define AT_CMD_RESPONSE_PROMPT ">"

#define AT_CMD_UART_READ_CHUNK_SIZE 32U
#define AT_CMD_UART_READ_INTERVAL_MS 10U
#define AT_CMD_UART_FLUSH_DELAY_MS 100U
#define AT_CMD_RESPONSE_TERMINATOR_OK "\r\nOK\r\n"
#define AT_CMD_RESPONSE_TERMINATOR_ERROR "\r\nERROR\r\n"

// Specific error type for device response to AT command error returns (differentiate a system/driver error with and device response status)
// i.e. the sim7080g device returning an 'error' is not the same as the driver code failing to send the command (which is system/driver error)
typedef enum
{
    AT_RESPONSE_OK = 0,
    AT_RESPONSE_ERROR,
    AT_RESPONSE_UNEXPECTED
} at_response_status_t;

// GENERIC parser signature for AT command handler fxns. This is used to parse the response of an AT command
//  The parsed response type is a void pointer here.
//  In actual implementation this each parser fxns has an assocaited static wrapper fxn that casts the void pointer to the correct type
// typedef esp_err_t (*at_cmd_parser_t)(const char *response, void *parsed_response);
typedef esp_err_t (*at_response_parser_t)(const char *response_str, void *parsed_response, at_cmd_type_t cmd_type);

typedef struct
{
    at_response_parser_t parser; // Generic parser function pointer
    uint32_t timeout_ms;         // Command timeout
    uint32_t retry_delay_ms;     // Delay between retries
} at_cmd_handler_config_t;

/// @brief Verify handle and pointers are not NULL, that UART is initialized, and that response buffer is not empty
esp_err_t validate_at_cmd_params(const sim7080g_handle_t *sim7080g_handle,
                                 const at_cmd_t *cmd,
                                 const char *response,
                                 size_t response_size);

/// @brief Get the command info structure for the specified command type (test, read, write, execute), and validate command string
esp_err_t get_cmd_info(const at_cmd_t *cmd,
                       at_cmd_type_t type,
                       const at_cmd_info_t **cmd_info);

/// @brief If the command is write type, format outgoing command string with its arguments. Otherwise just append \r\n
/// @note ensure at_cmd buffer is large enough to hold the formatted command (fits in buffer)
esp_err_t format_at_cmd(const at_cmd_info_t *cmd_info,
                        at_cmd_type_t type,
                        const char *args,
                        char *at_cmd,
                        size_t at_cmd_size);

/// @brief Send an AT command and read ther response using the esp idf UART driver (assumed already initialized)
/// @note This is NOT the fxn that API fxns should call, instead this is a helper fxn that only handles:
/// @note UART flush
/// @note UART write, read and timeout handling
/// @note  - it simply updates the response buffer with the raw response
esp_err_t send_receive_at_cmd(const sim7080g_handle_t *sim7080g_handle,
                              const char *at_cmd,
                              char *response,
                              size_t response_size,
                              uint32_t timeout_ms,
                              at_cmd_type_t cmd_type);

/// @brief Check for OK or ERROR in the response, and use custome AT response type to differentiate (prevents confusion with driver/system errors and device response as 'error')
at_response_status_t check_at_response_status(const char *response);

/// @brief Read UART response and store it in response buffer. This uses the 'cmd type' to determine how to handle the command
/// @note:Different AT command types have different response patterns:
/// @note - Write commands typically just respond with "OK" or "ERROR"
/// @note - Read commands usually respond with data + "OK" or "ERROR"
/// @note - Test commands show supported parameters + "OK" or "ERROR"
/// @note - Execute commands vary but often just "OK" or "ERROR"
esp_err_t read_uart_response(const sim7080g_handle_t *sim7080g_handle,
                             char *response,
                             size_t response_size,
                             uint32_t timeout_ms,
                             at_cmd_type_t cmd_type);

/// @brief THIS command is what is called by API fxns using AT commands to interact with the SIM7080g device
esp_err_t send_at_cmd_with_parser(const sim7080g_handle_t *sim7080g_handle,
                                  const at_cmd_t *cmd,
                                  at_cmd_type_t type,
                                  const char *args,
                                  void *parsed_response,
                                  const at_cmd_handler_config_t *handler_config);

esp_err_t send_receive_publish_cmd(const sim7080g_handle_t *sim7080g_handle,
                                   const char *at_cmd,
                                   char *response,
                                   size_t response_size,
                                   uint32_t timeout_ms);