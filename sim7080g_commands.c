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

static esp_err_t ate_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_ate_response(response, (ate_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cmee_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cmee_response(response, (cmee_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cgdcont_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cgdcont_response(response, (cgdcont_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cgatt_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cgatt_response(response, (cgatt_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cops_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cops_response(response, (cops_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cgnapn_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cgnapn_response(response, (cgnapn_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cncfg_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cncfg_response(response, (cncfg_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t cnact_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_cnact_response(response, (cnact_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t smconf_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_smconf_response(response, (smconf_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t smconn_parser_wrapper(const char *response, void *parsed_response, at_cmd_type_t cmd_type)
{
    return parse_smconn_response(response, (smconn_parsed_response_t *)parsed_response, cmd_type);
}

static esp_err_t smstate_parser_wrapper(const char *response,
                                        void *parsed_response,
                                        at_cmd_type_t cmd_type)
{
    return parse_smstate_response(response,
                                  (smstate_parsed_response_t *)parsed_response,
                                  cmd_type);
}

// --------------------------------------- FXNS to use SIM7080G AT Commands --------------------------------------- //
// ----------------------------- (main driver uses these fxns inside its user exposed fxns) ------------------- //

// Test AT communication with device (AT)
// NOTE :  For some reason the device is sometimes picky and doesnt respond to this command if it is the first command it receives after power on
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
esp_err_t sim7080g_set_echo_mode(const sim7080g_handle_t *handle, ate_mode_t mode)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (mode >= ATE_MODE_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting command echo mode to: %s", ate_mode_to_str(mode));

    ate_parsed_response_t response = {0};
    char args[8] = {0}; // Buffer for the mode argument

    // Format the mode argument
    snprintf(args, sizeof(args), "%d", mode);

    static const at_cmd_handler_config_t config = {
        .parser = ate_parser_wrapper,
        .timeout_ms = 1000U,   // 1 second timeout should be sufficient for this simple command
        .retry_delay_ms = 100U // 100ms between retries
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_ATE,
        AT_CMD_TYPE_EXECUTE,
        args,
        &response,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set echo mode");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully set echo mode to: %s", ate_mode_to_str(mode));
    return ESP_OK;
}

// Set Error Report Mode (CMEE) - Used by drier to enable verbose error reporting - 3
esp_err_t sim7080g_set_error_report_mode(const sim7080g_handle_t *handle, cmee_mode_t mode)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (mode >= CMEE_MODE_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting error report mode to: %s", cmee_mode_to_str(mode));

    cmee_parsed_response_t response = {0};
    char args[8] = {0}; // Buffer for the mode argument

    // Format the mode argument
    snprintf(args, sizeof(args), "%d", mode);

    static const at_cmd_handler_config_t config = {
        .parser = cmee_parser_wrapper,
        .timeout_ms = 3000U,   // 3 second timeout should be sufficient for this simple command
        .retry_delay_ms = 100U // 100ms between retries
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CMEE,
        AT_CMD_TYPE_WRITE,
        args,
        &response,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set error report mode");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully set error report mode to: %s", cmee_mode_to_str(mode));
    return ESP_OK;
}

// Define PDP context (CGDCONT) - Used by driver to set APN and PDP type (for network layer connection)
esp_err_t sim7080g_define_pdp_context(const sim7080g_handle_t *handle,
                                      uint8_t cid,
                                      cgdcont_pdp_type_t pdp_type,
                                      const char *apn)
{
    if ((handle == NULL) || (apn == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ((cid < CGDCONT_MIN_CID) || (cid > CGDCONT_MAX_CID))
    {
        ESP_LOGE(TAG, "Invalid CID: %u", cid);
        return ESP_ERR_INVALID_ARG;
    }

    if (pdp_type >= CGDCONT_PDP_TYPE_MAX)
    {
        ESP_LOGE(TAG, "Invalid PDP type");
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(apn) >= CGDCONT_APN_MAX_LEN)
    {
        ESP_LOGE(TAG, "APN string too long");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Setting PDP context - CID: %u, Type: %s, APN: %s",
             cid, cgdcont_pdp_type_to_str(pdp_type), apn);

    char args[CGDCONT_APN_MAX_LEN + 32] = {0}; // Extra space for CID, type, and format chars
    snprintf(args, sizeof(args), "%u,\"%s\",\"%s\"",
             cid, cgdcont_pdp_type_to_str(pdp_type), apn);

    cgdcont_parsed_response_t response = {0};

    static const at_cmd_handler_config_t config = {
        .parser = cgdcont_parser_wrapper,
        .timeout_ms = 10000U, // 10 sec
        .retry_delay_ms = 1000U};

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CGDCONT,
        AT_CMD_TYPE_WRITE,
        args,
        &response,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set PDP context");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully set PDP context");
    return ESP_OK;
}

// Get GPRS attached status
esp_err_t sim7080g_get_gprs_attachment(const sim7080g_handle_t *handle, cgatt_state_t *state_out)
{
    if ((handle == NULL) || (state_out == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Reading GPRS attachment state");

    cgatt_parsed_response_t response = {0};

    static const at_cmd_handler_config_t config = {
        .parser = cgatt_parser_wrapper,
        .timeout_ms = 5000U,    // 5 sec
        .retry_delay_ms = 1000U // 1 sec
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CGATT,
        AT_CMD_TYPE_READ,
        NULL,
        &response,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read GPRS attachment state");
        return ret;
    }

    *state_out = response.state;
    ESP_LOGI(TAG, "Current GPRS state: %s", cgatt_state_to_str(response.state));
    return ESP_OK;
}

esp_err_t sim7080g_set_gprs_attachment(const sim7080g_handle_t *handle, cgatt_state_t state)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (state >= CGATT_STATE_MAX)
    {
        ESP_LOGE(TAG, "Invalid GPRS attachment state");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting GPRS attachment state to: %s", cgatt_state_to_str(state));

    char args[8] = {0};
    snprintf(args, sizeof(args), "%d", state);

    cgatt_parsed_response_t response = {0};

    static const at_cmd_handler_config_t config = {
        .parser = cgatt_parser_wrapper,
        .timeout_ms = 75000U,   // 75 second timeout as per spec
        .retry_delay_ms = 1000U // 1 sec
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CGATT,
        AT_CMD_TYPE_WRITE,
        args,
        &response,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set GPRS attachment state");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully set GPRS attachment state");
    return ESP_OK;
}

esp_err_t sim7080g_get_operator_info(const sim7080g_handle_t *handle, cops_parsed_response_t *operator_info)
{
    if ((handle == NULL) || (operator_info == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Reading current operator information");

    static const at_cmd_handler_config_t config = {
        .parser = cops_parser_wrapper,
        .timeout_ms = 45000U,   // 45 second timeout as per spec
        .retry_delay_ms = 1000U // 1 sec
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_COPS,
        AT_CMD_TYPE_READ,
        NULL,
        operator_info,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read operator information");
        return ret;
    }

    ESP_LOGI(TAG, "Operator Info \n- Mode: %s\n- Format: %s\n- Name: %s\n- Technology: %s",
             cops_mode_to_str(operator_info->mode),
             cops_format_to_str(operator_info->format),
             operator_info->operator_name,
             cops_access_tech_to_str(operator_info->access_tech));

    return ESP_OK;
}

// Get APN
esp_err_t sim7080g_get_network_apn(const sim7080g_handle_t *handle,
                                   cgnapn_parsed_response_t *apn_info)
{
    if ((handle == NULL) || (apn_info == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Querying network-provided APN");

    static const at_cmd_handler_config_t config = {
        .parser = cgnapn_parser_wrapper,
        .timeout_ms = 5000U,    // 5 second timeout should be sufficient
        .retry_delay_ms = 1000U // 1 second between retries
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CGNAPN,
        AT_CMD_TYPE_EXECUTE,
        NULL,
        apn_info,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get network APN");
        return ret;
    }

    if (apn_info->status == CGNAPN_APN_PROVIDED)
    {
        ESP_LOGI(TAG, "Network APN Status: %s, APN: %s",
                 cgnapn_status_to_str(apn_info->status),
                 apn_info->network_apn);
    }
    else
    {
        ESP_LOGI(TAG, "Network APN Status: %s",
                 cgnapn_status_to_str(apn_info->status));
    }

    return ESP_OK;
}

// Set APN
esp_err_t sim7080g_set_pdp_config(const sim7080g_handle_t *handle, const cncfg_context_config_t *config)
{
    if ((handle == NULL) || (config == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->pdp_idx >= CNCFG_MAX_CONTEXTS)
    {
        ESP_LOGE(TAG, "Invalid PDP context index");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->ip_type >= CNCFG_IP_TYPE_MAX)
    {
        ESP_LOGE(TAG, "Invalid IP type");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting PDP configuration - Index: %u, IP Type: %s, APN: %s",
             config->pdp_idx,
             cncfg_ip_type_to_str(config->ip_type),
             config->apn);

    // Format command arguments
    char args[CNCFG_MAX_APN_LEN + CNCFG_MAX_USERNAME_LEN + CNCFG_MAX_PASSWORD_LEN + 32] = {0};

    if (config->username[0] && config->password[0])
    {
        // Full configuration with authentication
        snprintf(args, sizeof(args), "%u,%u,\"%s\",\"%s\",\"%s\",%u",
                 config->pdp_idx,
                 config->ip_type,
                 config->apn,
                 config->username,
                 config->password,
                 config->auth_type);
    }
    else
    {
        // Basic configuration without authentication
        snprintf(args, sizeof(args), "%u,%u,\"%s\"",
                 config->pdp_idx,
                 config->ip_type,
                 config->apn);
    }

    static const at_cmd_handler_config_t handler_config = {
        .parser = cncfg_parser_wrapper,
        .timeout_ms = 10000U,   // 10 second timeout
        .retry_delay_ms = 1000U // 1 second between retries
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CNCFG,
        AT_CMD_TYPE_WRITE,
        args,
        NULL,
        &handler_config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set PDP configuration");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully set PDP configuration");
    return ESP_OK;
}

esp_err_t sim7080g_get_pdp_config(const sim7080g_handle_t *handle, cncfg_parsed_response_t *config_info)
{
    if ((handle == NULL) || (config_info == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Reading PDP configurations");

    static const at_cmd_handler_config_t config = {
        .parser = cncfg_parser_wrapper,
        .timeout_ms = 5000U,
        .retry_delay_ms = 1000U};

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CNCFG,
        AT_CMD_TYPE_READ,
        NULL,
        config_info,
        &config);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Successfully read %d PDP configurations:", config_info->num_contexts);
        for (uint8_t i = 0; i < config_info->num_contexts; i++)
        {
            const cncfg_context_config_t *ctx = &config_info->contexts[i];
            ESP_LOGI(TAG, "Context %u - IP Type: %s, APN: %s, Username: %s, Password: %s, Auth: %s",
                     ctx->pdp_idx,
                     cncfg_ip_type_to_str(ctx->ip_type),
                     ctx->apn[0] ? ctx->apn : "<empty>",
                     ctx->username[0] ? ctx->username : "<empty>",
                     ctx->password[0] ? ctx->password : "<empty>",
                     cncfg_auth_type_to_str(ctx->auth_type));
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to read PDP configurations with error: %d", ret);
    }

    return ret;
}

// Get network activation status (CNACT)
esp_err_t sim7080g_get_network_status(const sim7080g_handle_t *handle, cnact_parsed_response_t *status_info)
{
    if ((handle == NULL) || (status_info == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Reading network activation status");

    static const at_cmd_handler_config_t config = {
        .parser = cnact_parser_wrapper,
        .timeout_ms = 5000U,    // 5 second timeout
        .retry_delay_ms = 1000U // 1 second between retries
    };

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_CNACT,
        AT_CMD_TYPE_READ,
        NULL,
        status_info,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read network status");
        return ret;
    }

    // Log all contexts
    for (uint8_t i = 0; i < status_info->num_contexts; i++)
    {
        const cnact_context_info_t *ctx = &status_info->contexts[i];
        ESP_LOGI(TAG, "Context %u: Status: %s, IP: %s",
                 ctx->pdp_idx,
                 cnact_status_to_str(ctx->status),
                 ctx->ip_address);
    }

    return ESP_OK;
}

// Set (activate OR deactivate) PDP context (CNACT)
// NOTE: BE SURE TO CHECK NETWORK STATUS AFTERWARDS TO VERIFY ACTIVATION - this command does not respond with OK (async) so we must check status separately
esp_err_t sim7080g_activate_network(const sim7080g_handle_t *handle, uint8_t pdp_idx, cnact_action_t action)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (pdp_idx >= CNACT_MAX_CONTEXTS)
    {
        ESP_LOGE(TAG, "Invalid PDP context index");
        return ESP_ERR_INVALID_ARG;
    }

    if (action >= CNACT_ACTION_MAX)
    {
        ESP_LOGE(TAG, "Invalid activation action");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting network activation - Context: %u, Action: %s",
             pdp_idx, cnact_action_to_str(action));

    // Just send the raw command without waiting for response
    char command[32] = {0};
    snprintf(command, sizeof(command), "AT+CNACT=%u,%u\r\n", pdp_idx, action);

    int32_t bytes_written = uart_write_bytes(handle->uart_config.port_num,
                                             command, strlen(command));
    if (bytes_written < 0)
    {
        ESP_LOGE(TAG, "Failed to send activation command");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sent network activation command - check status separately");
    return ESP_OK;
}

// Get MQTT configuration parameters (SMCONF)
esp_err_t sim7080g_get_mqtt_config(const sim7080g_handle_t *handle, smconf_config_t *config)
{
    if ((handle == NULL) || (config == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Reading MQTT configuration");

    smconf_parsed_response_t response = {0};

    static const at_cmd_handler_config_t handler_config = {
        .parser = smconf_parser_wrapper,
        .timeout_ms = 5000U,
        .retry_delay_ms = 1000U};

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_SMCONF,
        AT_CMD_TYPE_READ,
        NULL,
        &response,
        &handler_config);

    if (ret == ESP_OK)
    {
        memcpy(config, &response.config, sizeof(smconf_config_t));
        ESP_LOGI(TAG, "Successfully read MQTT configuration");
    }

    return ret;
}

// Set MQTT configuration parameters (SMCONF)
esp_err_t sim7080g_set_mqtt_param(const sim7080g_handle_t *handle,
                                  smconf_param_t param,
                                  const char *value)
{
    if ((handle == NULL) || (value == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (param >= SMCONF_PARAM_MAX)
    {
        ESP_LOGE(TAG, "Invalid MQTT parameter");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting MQTT parameter %s to %s",
             smconf_param_to_str(param), value);

    char args[SMCONF_MESSAGE_MAX_LEN + 32] = {0};
    snprintf(args, sizeof(args), "\"%s\",\"%s\"",
             smconf_param_to_str(param), value);

    static const at_cmd_handler_config_t config = {
        .parser = smconf_parser_wrapper,
        .timeout_ms = 5000U,
        .retry_delay_ms = 1000U};

    esp_err_t ret = send_at_cmd_with_parser(
        handle,
        &AT_SMCONF,
        AT_CMD_TYPE_WRITE,
        args,
        NULL,
        &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set MQTT parameter");
        return ret;
    }

    ESP_LOGI(TAG, "Successfully set MQTT parameter");
    return ESP_OK;
}

// Connect to MQTT broker
esp_err_t sim7080g_mqtt_connect_to_broker(const sim7080g_handle_t *handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    static const char *TAG = "sim7080g_mqtt_connect";

    smconn_parsed_response_t parsed_response = {0};

    // Configure extended timeout for MQTT connection
    at_cmd_handler_config_t handler_config = {
        .parser = smconn_parser_wrapper,
        .timeout_ms = 10000U,   // 10 seconds timeout for connection
        .retry_delay_ms = 1000U // 1 second between retries
    };

    esp_err_t err = send_at_cmd_with_parser(
        handle,
        &AT_SMCONN,
        AT_CMD_TYPE_EXECUTE,
        NULL,
        &parsed_response,
        &handler_config);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to execute SMCONN command");
        return err;
    }

    // Handle specific connection status responses
    switch (parsed_response.status)
    {
    case SMCONN_STATUS_SUCCESS:
        ESP_LOGI(TAG, "Successfully connected to MQTT broker");
        return ESP_OK;

    case SMCONN_STATUS_ERROR:
        ESP_LOGE(TAG, "Failed to connect to MQTT broker");
        return ESP_FAIL;

    default:
        ESP_LOGE(TAG, "Unexpected connection status: %d", parsed_response.status);
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_FAIL;
}
// Disconnect from MQTT broker

// Publish to MQTT broker
esp_err_t sim7080g_mqtt_publish(const sim7080g_handle_t *handle,
                                const char *topic,
                                const char *message,
                                uint8_t qos,
                                bool retain)
{
    if ((NULL == handle) || (NULL == topic) || (NULL == message))
    {
        return ESP_ERR_INVALID_ARG;
    }

    static const char *TAG = "sim7080g_mqtt_publish";
    char at_cmd[AT_CMD_MAX_LEN] = {0};
    char response[AT_CMD_RESPONSE_MAX_LEN] = {0};
    esp_err_t err;

    // Validate parameters
    size_t topic_len = strlen(topic);
    size_t message_len = strlen(message);

    if ((topic_len >= MQTT_MAX_TOPIC_LEN) || (message_len >= MQTT_MAX_MESSAGE_LEN))
    {
        ESP_LOGE(TAG, "Topic or message too long");
        return ESP_ERR_INVALID_SIZE;
    }

    if (qos > 2U)
    {
        ESP_LOGE(TAG, "Invalid QoS level");
        return ESP_ERR_INVALID_ARG;
    }

    // Format command
    int32_t written_len = snprintf(at_cmd, sizeof(at_cmd),
                                   "AT+SMPUB=\"%s\",%zu,%u,%d\r\n",
                                   topic, message_len, qos, retain ? 1 : 0);

    if ((written_len < 0) || ((size_t)written_len >= sizeof(at_cmd)))
    {
        ESP_LOGE(TAG, "Command formatting failed");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Publishing message to topic %s", topic);

    // Send command and wait for prompt
    err = send_receive_publish_cmd(handle, at_cmd, response,
                                   sizeof(response), 15000U);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to receive prompt");
        return err;
    }

    // Send message content with CRLF
    char content_with_crlf[MQTT_MAX_MESSAGE_LEN + 3];
    written_len = snprintf(content_with_crlf, sizeof(content_with_crlf),
                           "%s\r\n", message);

    if ((written_len < 0) || ((size_t)written_len >= sizeof(content_with_crlf)))
    {
        ESP_LOGE(TAG, "Message formatting failed");
        return ESP_ERR_INVALID_SIZE;
    }

    // send the ACTUAL MESSAGE DATA CONTENTs
    err = send_receive_at_cmd(handle, content_with_crlf, response,
                              sizeof(response), 5000U, AT_CMD_TYPE_WRITE);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send message content");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully published message");
    return ESP_OK;
}

// Check MQTT broker connection status
esp_err_t sim7080g_mqtt_check_connection_status(const sim7080g_handle_t *handle,
                                                smstate_status_t *status_out)
{
    if ((handle == NULL) || (status_out == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    static const char *TAG = "sim7080g_mqtt_check_connection";

    smstate_parsed_response_t parsed_response = {0};

    // Configure handler for status check
    at_cmd_handler_config_t handler_config = {
        .parser = smstate_parser_wrapper,
        .timeout_ms = 5000U,    // 5 second timeout should be sufficient
        .retry_delay_ms = 1000U // 1 second between retries
    };

    esp_err_t err = send_at_cmd_with_parser(
        handle,
        &AT_SMSTATE,
        AT_CMD_TYPE_READ,
        NULL,
        &parsed_response,
        &handler_config);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read MQTT state");
        return err;
    }

    *status_out = parsed_response.status;
    ESP_LOGI(TAG, "MQTT Connection Status: %s",
             smstate_status_to_str(parsed_response.status));

    return ESP_OK;
}

// Subscribe to MQTT broker

// Disconnect from MQTT broker
