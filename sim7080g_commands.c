#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include "sim7080g_types.h"
#include "sim7080g_uart.h"
#include "sim7080g_commands.h"
#include "sim7080g_at_cmd_responses.h"
#include "sim7080g_at_cmd_handler.h"

static const char *TAG = "SIM7080G Commands for Interacting with device";

static esp_err_t at_test_parser_wrapper(const char *response, void *parsed_response)
{
    return parse_at_test_response(response, (at_test_parsed_response_t *)parsed_response);
}

static esp_err_t cpin_parser_wrapper(const char *response, void *parsed_response)
{
    return parse_cpin_response(response, (cpin_parsed_response_t *)parsed_response);
}

// --------------------------------------- FXNS to use SIM7080G AT Commands --------------------------------------- //
// ----------------------------- (main driver uses these fxns inside its user exposed fxns) ------------------- //

// Cycle CFUN - soft reset (CFUN)

// Test AT communication with device (AT)
esp_err_t sim7080g_test_at(const sim7080g_handle_t *handle, at_test_status_t *at_test_status_out)
{
    if (handle == NULL || at_test_status_out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Sending: TEST AT command");

    at_test_parsed_response_t response = {0};

    static const at_cmd_handler_config_t config = {
        .parser = at_test_parser_wrapper, // Command-specific parser
        .timeout_ms = 5000U,              // Command timeout
        .retry_delay_ms = 1000U           // Delay between retries
    };

    esp_err_t ret = send_at_cmd_with_parser(handle,              // Device handle
                                            &AT_TEST,            // Command definition
                                            AT_CMD_TYPE_EXECUTE, // Command type (TECHINCALLY its an EXECUTE command type, despite the name)
                                            NULL,                // No arguments for TEST
                                            &response,           // Response structure
                                            &config              // Command config
    );

    if (ret == ESP_OK)
    {
        *at_test_status_out = response.status;
        ESP_LOGI(TAG, "AT Test Status: %s", at_test_status_to_str(*at_test_status_out));
    }

    return ret;
}

// Disable command Echo (ATE0)

// Enable verbose error reporting (CMEE)

// Check SIM status (CPIN)
esp_err_t sim7080g_check_sim_status(const sim7080g_handle_t *handle, cpin_status_t *sim_status_out)
{
    if (handle == NULL || sim_status_out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Sending: CHECK SIM STATUS command");

    // Response structure only used within this function
    // The user only cares about the resultling status enum
    cpin_parsed_response_t response = {0};

    // Configure command handling
    static const at_cmd_handler_config_t config = {
        .parser = cpin_parser_wrapper, // Command-specific parser
        .timeout_ms = 5000U,           // Command timeout
        .retry_delay_ms = 1000U        // Delay between retries
    };

    // Send command and parse response
    esp_err_t ret = send_at_cmd_with_parser(
        handle,           // Device handle
        &AT_CPIN,         // Command definition
        AT_CMD_TYPE_READ, // Command type
        NULL,             // No arguments for READ
        &response,        // Response structure
        &config           // Command config
    );

    if (ret == ESP_OK)
    {
        *sim_status_out = response.status;
        ESP_LOGI(TAG, "SIM Status: %s", cpin_status_to_str(response.status));
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
