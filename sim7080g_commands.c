#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include "sim7080g_types.h"
#include "sim7080g_uart.h"
#include "sim7080g_commands.h"
#include "sim7080g_at_cmd_responses.h"
#include "sim7080g_at_cmd_handler.h"

static const char *TAG = "SIM7080G Commands for Interacting with device";

// --------------------------------------- FXNS to use SIM7080G AT Commands --------------------------------------- //

// Cycle CFUN - soft reset (CFUN)

// Test AT communication with device (AT)

// Disable command Echo (ATE0)

// Enable verbose error reporting (CMEE)

// Check SIM status (CPIN)
esp_err_t sim7080g_check_sim_status(const sim7080g_handle_t *handle, cpin_status_t *sim_status_out)
{
    if (handle == NULL || sim_status_out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Response structure only used within this function
    // The user only cares about the resultling status enum
    cpin_response_t response = {0};

    // Configure command handling
    static const at_cmd_handler_config_t config = {
        .parser = parse_cpin_response, // Command-specific parser
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
        // Only update output parameter if command succeeded
        *sim_status_out = response.status;

        // Log the result
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
