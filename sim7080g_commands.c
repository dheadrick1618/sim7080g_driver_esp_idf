#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include "sim7080g_types.h"
#include "sim7080g_uart.h"
#include "sim7080g_commands.h"
#include "sim7080g_at_cmd_responses.h"
#include "sim7080g_at_cmd_handler.h"

static const char *TAG = "SIM7080G Commands for Interacting with device";

static esp_err_t at_test_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_at_test_response(response, (at_test_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cpin_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cpin_response(response, (cpin_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cfun_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cfun_response(response, (cfun_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t csq_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_csq_response(response, (csq_parsed_response_t *)parsed_response, cmd_type);
}

// --------------------------------------- FXNS to use SIM7080G AT Commands --------------------------------------- //
// ----------------------------- (main driver uses these fxns inside its user exposed fxns) ------------------- //

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

// Set functionality level (CFUN) - this is used by driver fxn to 'soft reset' device (CFUN=0, then CFUN=1)
esp_err_t sim7080g_set_functionality(const sim7080g_handle_t *handle, cfun_functionality_t fun_level)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (fun_level >= CFUN_FUNCTIONALITY_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting phone functionality level to: %s", cfun_functionality_to_str(fun_level));

    cfun_parsed_response_t response = {0};
    char args[8] = {0}; // Buffer for the function level argument

    // Format the function level argument
    snprintf(args, sizeof(args), "%d", fun_level);

    static const at_cmd_handler_config_t config = {
        .parser = cfun_parser_wrapper,
        .timeout_ms = 10000U,   // 10 second timeout as per spec
        .retry_delay_ms = 1000U // 1 second between retries
    };

    // Send command and parse response
    esp_err_t ret = send_at_cmd_with_parser(
        handle,            // Device handle
        &AT_CFUN,          // Command definition
        AT_CMD_TYPE_WRITE, // Command type
        args,              // Function level argument
        &response,         // Response structure
        &config            // Command config
    );

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set phone functionality level");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully set phone functionality level to: %s",
             cfun_functionality_to_str(response.functionality));

    return ESP_OK;
}

// Check signal quality (CSQ) - used by driver 'is physical layer connected' fxn which verifies signal strength is acceptable
esp_err_t sim7080g_check_signal_quality(const sim7080g_handle_t *handle, int8_t *rssi_dbm_out, uint8_t *ber_out)
{
    if ((handle == NULL) || (rssi_dbm_out == NULL) || (ber_out == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending: CHECK SIGNAL QUALITY command");

    csq_parsed_response_t response = {0};

    static const at_cmd_handler_config_t config = {
        .parser = csq_parser_wrapper,
        .timeout_ms = 5000U,
        .retry_delay_ms = 1000U};

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CSQ,
        AT_CMD_TYPE_EXECUTE,
        NULL,
        &response,
        &config);

    if (ret == ESP_OK)
    {
        *rssi_dbm_out = response.rssi_dbm;
        *ber_out = (uint8_t)response.ber;
        ESP_LOGI(TAG, "Signal Quality - RSSI: %s, BER: %s",
                 csq_rssi_to_str(response.rssi),
                 csq_ber_to_str(response.ber));
    }

    return ret;
}

// Disable command Echo (ATE0)

// Enable verbose error reporting (CMEE)

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
