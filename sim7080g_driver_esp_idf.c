#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

#include "sim7080g_uart.h"
#include "sim7080g_driver_esp_idf.h"
#include "sim7080g_at_cmds.h"
#include "sim7080g_commands.h"

static const char *TAG = "SIM7080G Driver";

// ---------------------  INTERNAL HELPER FXNs  -------------------------//

// config device mqtt params:
// - First get params from device
// - Then compare to config params
// - Check each param - if it matches the config param, skip setting it
// - If it does not match, set the config param on the device
static esp_err_t config_device_mqtt_params(const sim7080g_handle_t *handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Get current device MQTT params
    smconf_config_t device_params = {0};
    esp_err_t err = sim7080g_get_mqtt_config(handle, &device_params);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error getting device MQTT params: %s", esp_err_to_name(err));
        return err;
    }

    // ESP_LOGI(TAG, "Comparing values:");
    // ESP_LOGI(TAG, "  URL: Current='%s' New='%s'", device_params.url, handle->mqtt_config.broker_url);
    // ESP_LOGI(TAG, "  Port: Current=%u New=%u", device_params.port, handle->mqtt_config.port);
    // ESP_LOGI(TAG, "  Client ID: Current='%s' New='%s'", device_params.client_id, handle->mqtt_config.client_id);
    // ESP_LOGI(TAG, "  Username: Current='%s' New='%s'", device_params.username, handle->mqtt_config.username);
    // ESP_LOGI(TAG, "  Keeptime: Current=%u New=60", device_params.keeptime);
    // ESP_LOGI(TAG, "  Clean Session: Current=%d New=1", device_params.clean_session);

    // Flag to track if any parameters were updated
    bool params_updated = false;

    // 1. Configure Client ID
    if (strcmp(device_params.client_id, handle->mqtt_config.client_id) != 0)
    {
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_CLIENTID,
                                      handle->mqtt_config.client_id);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set client ID: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Updated client ID to: %s", handle->mqtt_config.client_id);
    }

    // 2. Configure URL and Port together
    // Configure URL only - since SMCONF doesn't have a separate port parameter
    char device_url[SMCONF_URL_MAX_LEN] = {0};

    // Parse device URL ignoring any port part
    char *comma = strchr(device_params.url, ',');
    if (comma != NULL)
    {
        size_t url_len = comma - device_params.url;
        strncpy(device_url, device_params.url, url_len);
    }
    else
    {
        strncpy(device_url, device_params.url, sizeof(device_url) - 1);
    }

    // Compare just the URL portion
    bool url_different = strcmp(device_url, handle->mqtt_config.broker_url) != 0;

    if (url_different)
    {
        // Set just the URL without appending port
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_URL, handle->mqtt_config.broker_url);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set URL: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Updated broker URL to: %s", handle->mqtt_config.broker_url);
    }

    // err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_URL, handle->mqtt_config.broker_url);

    // 3. Configure Username
    if (strcmp(device_params.username, handle->mqtt_config.username) != 0)
    {
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_USERNAME,
                                      handle->mqtt_config.username);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set username: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Updated username to: %s", handle->mqtt_config.username);
    }

    // 4. Configure Password (don't log actual password)
    if (strcmp(device_params.password, handle->mqtt_config.client_password) != 0)
    {
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_PASSWORD,
                                      handle->mqtt_config.client_password);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set password: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Updated password");
    }

    // 5. Configure recommended default parameters if they differ from current settings

    // Keep-alive time (60 seconds is a good default)
    if (device_params.keeptime != 60)
    {
        char keeptime_str[8];
        snprintf(keeptime_str, sizeof(keeptime_str), "%u", 60);
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_KEEPTIME, keeptime_str);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set keeptime: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Set keeptime to 60 seconds");
    }

    // Clean Session (always true for reliability)
    if (!device_params.clean_session)
    {
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_CLEANSS, "1");
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set clean session: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Enabled clean session");
    }

    // QoS (use QoS 1 for reliable delivery with reasonable overhead)
    if (device_params.qos != SMCONF_QOS_AT_LEAST_ONCE)
    {
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_QOS, "1");
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set QoS: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Set QoS to 1 (at least once)");
    }

    // Don't set retain flag by default
    if (device_params.retain)
    {
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_RETAIN, "0");
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to clear retain flag: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Cleared retain flag");
    }

    // Use synchronous mode for better control
    if (device_params.async_mode)
    {
        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_ASYNCMODE, "0");
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set sync mode: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Set synchronous mode");
    }

    if (!params_updated)
    {
        ESP_LOGI(TAG, "All MQTT parameters already correctly configured");
    }
    else
    {
        ESP_LOGI(TAG, "MQTT parameters updated successfully");
    }

    return ESP_OK;
}
// ---------------------  EXTERNAL EXPOSED API FXNs  -------------------------//
// Set the config member structs of the driver handler here - these values are then used during driver init and operation
esp_err_t sim7080g_config(sim7080g_handle_t *sim7080g_handle,
                          const sim7080g_uart_config_t sim7080g_uart_config,
                          const sim7080g_mqtt_config_t sim7080g_mqtt_config)
{
    // TODO - validate config params
    sim7080g_handle->uart_config = sim7080g_uart_config;

    sim7080g_handle->mqtt_config = sim7080g_mqtt_config;

    return ESP_OK;
}

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
    vTaskDelay(pdMS_TO_TICKS(500)); // Give UART time to init

    ate_mode_t echo_mode = ATE_MODE_OFF;
    esp_err_t ret = sim7080g_set_echo_mode(sim7080g_handle, echo_mode);
    if (ret != ESP_OK)
    {
        printf("Failed to disable command echo\n");
        return ret;
    }
    else
    {
        printf("Command echo disabled\n");
    }

    cmee_mode_t error_mode = CMEE_MODE_VERBOSE;
    ret = sim7080g_set_error_report_mode(sim7080g_handle, error_mode);
    if (ret != ESP_OK)
    {
        printf("Failed to enable verbose error reporting\n");
        return ret;
    }
    else
    {
        printf("Verbose error reporting enabled\n");
    }

    err = config_device_mqtt_params(sim7080g_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configuring device MQTT params: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "SIM7080G Driver initialized");
    return ESP_OK;
}

esp_err_t sim7080g_connect_to_network_bearer(const sim7080g_handle_t *sim7080g_handle, const char *apn)
{
    if ((sim7080g_handle == NULL) || (apn == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err;
    const char *TAG = "SIM7080G Connect To Network Bearer";

    // FIRST check if already connected
    cnact_parsed_response_t status_info = {0};
    err = sim7080g_get_network_status(sim7080g_handle, &status_info);
    if (err == ESP_OK)
    {
        // Check if any context is already active
        for (uint8_t i = 0; i < status_info.num_contexts; i++)
        {
            if (status_info.contexts[i].status == CNACT_STATUS_ACTIVATED)
            {
                ESP_LOGI(TAG, "Network already activated with IP: %s",
                         status_info.contexts[i].ip_address);
                return ESP_OK;
            }
        }
    }
    else
    {
        ESP_LOGW(TAG, "Failed to check network status, attempting connection anyway");
    }

    // Step 1: Disable RF functionality first to ensure clean state
    ESP_LOGI(TAG, "Disabling RF functionality...");
    err = sim7080g_set_functionality(sim7080g_handle, CFUN_FUNCTIONALITY_MIN);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to disable RF");
        return err;
    }

    // Step 2: Set up PDP context with APN
    ESP_LOGI(TAG, "Setting up PDP context with APN: %s", apn);
    err = sim7080g_define_pdp_context(sim7080g_handle,
                                      1, // Use context ID 1
                                      CGDCONT_PDP_TYPE_IP,
                                      apn);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to define PDP context");
        return err;
    }

    // Step 3: Re-enable RF functionality
    ESP_LOGI(TAG, "Re-enabling RF functionality...");
    err = sim7080g_set_functionality(sim7080g_handle, CFUN_FUNCTIONALITY_FULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable RF");
        return err;
    }

    // Step 4: Wait for SIM status to be ready
    ESP_LOGI(TAG, "Waiting for SIM to be ready...");
    for (int retry = 0; retry < 10; retry++)
    {
        cpin_status_t sim_status;
        err = sim7080g_check_sim_status(sim7080g_handle, &sim_status);
        if (err == ESP_OK && sim_status == CPIN_STATUS_READY)
        {
            break;
        }
        if (retry == 9)
        {
            ESP_LOGE(TAG, "SIM failed to become ready");
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Step 5: Check GPRS attachment
    ESP_LOGI(TAG, "Checking GPRS attachment...");
    cgatt_state_t gprs_state;
    err = sim7080g_get_gprs_attachment(sim7080g_handle, &gprs_state);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to check GPRS state");
        return err;
    }

    if (gprs_state != CGATT_STATE_ATTACHED)
    {
        ESP_LOGI(TAG, "GPRS not attached, attempting attachment...");
        err = sim7080g_set_gprs_attachment(sim7080g_handle, CGATT_STATE_ATTACHED);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to attach GPRS");
            return err;
        }
    }

    // Step 6: Configure PDP settings
    ESP_LOGI(TAG, "Configuring PDP settings...");
    cncfg_context_config_t pdp_config = {
        .pdp_idx = 0,
        .ip_type = CNCFG_IP_TYPE_IPV4,
    };
    strncpy(pdp_config.apn, apn, CNCFG_MAX_APN_LEN);

    err = sim7080g_set_pdp_config(sim7080g_handle, &pdp_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure PDP settings");
        return err;
    }

    // Step 7: Activate the network connection
    ESP_LOGI(TAG, "Activating network connection...");
    err = sim7080g_activate_network(sim7080g_handle, 0, CNACT_ACTION_ACTIVATE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to activate network");
        return err;
    }

    // Step 8: Verify the activation was successful by checking status
    ESP_LOGI(TAG, "Verifying network activation...");
    for (int retry = 0; retry < 10; retry++)
    {
        cnact_parsed_response_t status_info;
        err = sim7080g_get_network_status(sim7080g_handle, &status_info);
        if (err == ESP_OK)
        {
            bool found_active = false;
            for (uint8_t i = 0; i < status_info.num_contexts; i++)
            {
                if (status_info.contexts[i].pdp_idx == 0 &&
                    status_info.contexts[i].status == CNACT_STATUS_ACTIVATED)
                {
                    found_active = true;
                    ESP_LOGI(TAG, "Network activated successfully with IP: %s",
                             status_info.contexts[i].ip_address);
                    break;
                }
            }
            if (found_active)
            {
                return ESP_OK;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGE(TAG, "Network activation verification failed");
    return ESP_FAIL;
}

esp_err_t sim7080g_mqtt_connect(const sim7080g_handle_t *sim7080g_handle)
{
    if (sim7080g_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const char *TAG = "SIM7080G MQTT Connect";

    // Check if connected already - if so do not connect
    smstate_status_t state;
    esp_err_t err = sim7080g_mqtt_check_connection_status(sim7080g_handle, &state);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Failed to check MQTT connection status");
        return ESP_FAIL;
    }
    if (state == SMSTATE_STATUS_CONNECTED)
    {
        ESP_LOGI(TAG, "Already connected to MQTT broker");
        return ESP_OK;
    }
    else if (state == SMSTATE_STATUS_CONNECTED_WITH_SESSION)
    {
        ESP_LOGI(TAG, "Already connected to MQTT broker with session");
        return ESP_OK;
    }

    // Connect to MQTT broker
    err = sim7080g_mqtt_connect_to_broker(sim7080g_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to connect to MQTT broker");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Connected to MQTT broker");
    return ESP_OK;
}

esp_err_t sim7080g_mqtt_disconnect(const sim7080g_handle_t *sim7080g_handle)
{
    if (sim7080g_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const char *TAG = "SIM7080G MQTT Disconnect";

    // Check if connected already - if so do not disconnect
    smstate_status_t state;
    esp_err_t err = sim7080g_mqtt_check_connection_status(sim7080g_handle, &state);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Failed to check MQTT connection status");
        return ESP_FAIL;
    }
    if (state == SMSTATE_STATUS_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Already disconnected from MQTT broker");
        return ESP_OK;
    }

    // Disconnect from MQTT broker
    err = sim7080g_mqtt_disconnect_from_broker(sim7080g_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to disconnect from MQTT broker");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Disconnected from MQTT broker");
    return ESP_OK;
}

// MQTT subscribe

// Check if the physical layer is connected
esp_err_t sim7080g_is_physical_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected_out)
{
    if ((sim7080g_handle == NULL) || (connected_out == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    *connected_out = false;
    const char *TAG = "Physical Layer Check";

    /* Check 1: SIM Card Status
     * AT Command: AT+CPIN?
     * Checks if SIM is present and ready */
    cpin_status_t sim_status;
    err = sim7080g_check_sim_status(sim7080g_handle, &sim_status);
    if ((err != ESP_OK) || (sim_status != CPIN_STATUS_READY))
    {
        ESP_LOGE(TAG, "SIM not ready: %s",
                 (err == ESP_OK) ? cpin_status_to_str(sim_status) : "Check failed");
        return err;
    }

    /* Check 2: Signal Quality
     * AT Command: AT+CSQ
     * Ensures signal is strong enough for operation */
    int8_t rssi_dbm;
    uint8_t ber;
    err = sim7080g_check_signal_quality(sim7080g_handle, &rssi_dbm, &ber);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Signal quality check failed");
        return err;
    }

    /* Signal strength criteria:
     * - RSSI should not be 99 (not detectable)
     * - RSSI should be better than -100 dBm for reliable operation */
    if ((rssi_dbm == 99) || (rssi_dbm) < -100)
    {
        ESP_LOGW(TAG, "Signal too weak or not detectable: RSSI=%d dBm", rssi_dbm);
        return ESP_OK; // Not an error, just not connected
    }

    /* Check 3: EPS Network Registration Status
     * AT Command: AT+CEREG?
     * Verifies registration with the LTE network */
    cereg_parsed_response_t cereg_response = {0};
    err = sim7080g_get_eps_network_reg_info(sim7080g_handle, &cereg_response);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Network registration check failed");
        return err;
    }

    if ((cereg_response.status != CEREG_STATUS_REGISTERED_HOME) &&
        (cereg_response.status != CEREG_STATUS_REGISTERED_ROAMING))
    {
        ESP_LOGW(TAG, "Not registered to network: %s",
                 cereg_status_to_str(cereg_response.status));
        return ESP_OK; // Not an error, just not connected
    }

    /* Log the network technology type if available */
    if (cereg_response.has_location_info)
    {
        ESP_LOGI(TAG, "Connected using: %s",
                 cereg_act_to_str(cereg_response.act));
    }

    /* Check 4: Radio Functionality
     * AT Command: AT+CFUN?
     * Verifies radio is in full functional state */
    cfun_functionality_t device_functionality;
    err = sim7080g_get_functionality(sim7080g_handle, &device_functionality);
    if ((err != ESP_OK) || (device_functionality != CFUN_FUNCTIONALITY_FULL))
    {
        ESP_LOGE(TAG, "Radio not in full functionality mode: %s",
                 (err == ESP_OK) ? cfun_functionality_to_str(device_functionality)
                                 : "Check failed");
        return err;
    }

    /* All physical layer checks passed */
    *connected_out = true;
    ESP_LOGI(TAG, "Physical layer connection verified");
    return ESP_OK;
}

// Check is network layer is connected
esp_err_t sim7080g_is_network_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected)
{
    if ((sim7080g_handle == NULL) || (connected == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    *connected = false;

    /* Check 1: GPRS Attachment Status
     * AT Command: AT+CGATT?
     * Verifies device is attached to GPRS service */
    cgatt_state_t gprs_state;
    err = sim7080g_get_gprs_attachment(sim7080g_handle, &gprs_state);
    if ((err != ESP_OK) || (gprs_state != CGATT_STATE_ATTACHED))
    {
        return err;
    }

    /* Check 2: PDP Context Status
     * AT Command: AT+CNACT?
     * Verifies PDP context is activated and we have an IP address */
    cnact_parsed_response_t pdp_status;
    err = sim7080g_get_network_status(sim7080g_handle, &pdp_status);
    if (err != ESP_OK)
    {
        return err;
    }

    /* Look for any activated PDP context */
    for (uint8_t i = 0; i < pdp_status.num_contexts; i++)
    {
        if (pdp_status.contexts[i].status == CNACT_STATUS_ACTIVATED)
        {
            *connected = true;
            break;
        }
    }

    return ESP_OK;
}

// Check if the application layer is connected
esp_err_t sim7080g_is_application_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected)
{
    if ((sim7080g_handle == NULL) || (connected == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    *connected = false;

    /* Check MQTT Connection Status
     * AT Command: AT+SMSTATE?
     * Verifies MQTT connection to broker is active */
    smstate_parsed_response_t mqtt_status;
    err = sim7080g_mqtt_check_connection_status(sim7080g_handle, &mqtt_status.status);
    if (err != ESP_OK)
    {
        return err;
    }

    /* Consider both connected states as valid */
    if ((mqtt_status.status == SMSTATE_STATUS_CONNECTED) ||
        (mqtt_status.status == SMSTATE_STATUS_CONNECTED_WITH_SESSION))
    {
        *connected = true;
    }

    return ESP_OK;
}

esp_err_t sim7080g_get_device_status(sim7080g_handle_t *handle, device_status_t *status)
{
    if (handle == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    memset(status, 0, sizeof(device_status_t));

    // 1. Get Signal Quality Information
    err = sim7080g_check_signal_quality(handle, &status->signal_info.rssi_dbm,
                                        &status->signal_info.ber);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get signal quality");
        return err;
    }
    status->signal_info.category = csq_rssi_dbm_to_strength_category(status->signal_info.rssi_dbm);

    // 2. Get Network Registration Status
    cereg_parsed_response_t cereg_status;
    err = sim7080g_get_eps_network_reg_info(handle, &cereg_status);
    if (err == ESP_OK)
    {
        status->network_info.is_registered =
            (cereg_status.status == CEREG_STATUS_REGISTERED_HOME ||
             cereg_status.status == CEREG_STATUS_REGISTERED_ROAMING);
        status->network_info.is_roaming =
            (cereg_status.status == CEREG_STATUS_REGISTERED_ROAMING);

        if (cereg_status.has_location_info)
        {
            strncpy(status->network_info.tac, cereg_status.tac, sizeof(status->network_info.tac) - 1);
            strncpy(status->network_info.cell_id, cereg_status.ci, sizeof(status->network_info.cell_id) - 1);
            status->network_info.access_tech = cereg_status.act;
        }
    }

    // 3. Get Operator Information
    cops_parsed_response_t cops_info;
    err = sim7080g_get_operator_info(handle, &cops_info);
    if (err == ESP_OK)
    {
        strncpy(status->network_info.operator_name, cops_info.operator_name,
                sizeof(status->network_info.operator_name) - 1);
    }

    // 4. Get PDP/Connection Status
    cnact_parsed_response_t cnact_status;
    err = sim7080g_get_network_status(handle, &cnact_status);
    if (err == ESP_OK)
    {
        for (uint8_t i = 0; i < cnact_status.num_contexts; i++)
        {
            if (cnact_status.contexts[i].status == CNACT_STATUS_ACTIVATED)
            {
                status->connection_info.pdp_active = true;
                strncpy(status->connection_info.ip_address,
                        cnact_status.contexts[i].ip_address,
                        sizeof(status->connection_info.ip_address) - 1);
                break;
            }
        }
    }

    // 5. Get APN Information
    cgnapn_parsed_response_t apn_info;
    err = sim7080g_get_network_apn(handle, &apn_info);
    if (err == ESP_OK && apn_info.status == CGNAPN_APN_PROVIDED)
    {
        strncpy(status->connection_info.apn, apn_info.network_apn,
                sizeof(status->connection_info.apn) - 1);
    }

    return ESP_OK;
}

esp_err_t sim7080g_publish_device_status(sim7080g_handle_t *handle, const char *base_topic)
{
    if (handle == NULL || base_topic == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    device_status_t status;
    esp_err_t err = sim7080g_get_device_status(handle, &status);
    if (err != ESP_OK)
    {
        return err;
    }

    // Create buffers with safe sizes
    char message[1024];        // Increased buffer size
    char network_info[384];    // Increased for safety
    char connection_info[384]; // Increased for safety
    int written;

    // Format network info safely
    written = snprintf(network_info, sizeof(network_info),
                       "{\"registered\":%s,\"roaming\":%s,\"operator\":\"%s\","
                       "\"technology\":\"%s\",\"tac\":\"%s\",\"cell_id\":\"%s\"}",
                       status.network_info.is_registered ? "true" : "false",
                       status.network_info.is_roaming ? "true" : "false",
                       status.network_info.operator_name,
                       cereg_act_to_str(status.network_info.access_tech),
                       status.network_info.tac,
                       status.network_info.cell_id);

    if (written < 0 || written >= sizeof(network_info))
    {
        ESP_LOGE("Telemetry", "Network info buffer overflow");
        return ESP_ERR_INVALID_SIZE;
    }

    // Format connection info safely
    written = snprintf(connection_info, sizeof(connection_info),
                       "{\"pdp_active\":%s,\"ip_address\":\"%s\",\"apn\":\"%s\"}",
                       status.connection_info.pdp_active ? "true" : "false",
                       status.connection_info.ip_address,
                       status.connection_info.apn);

    if (written < 0 || written >= sizeof(connection_info))
    {
        ESP_LOGE("Telemetry", "Connection info buffer overflow");
        return ESP_ERR_INVALID_SIZE;
    }

    // Combine everything safely
    written = snprintf(message, sizeof(message),
                       "{\"signal\":{\"rssi_dbm\":%d,\"category\":\"%s\",\"ber\":%u},"
                       "\"network\":%s,\"connection\":%s}",
                       status.signal_info.rssi_dbm,
                       signal_strength_category_to_str(status.signal_info.category),
                       status.signal_info.ber,
                       network_info,
                       connection_info);

    if (written < 0 || written >= sizeof(message))
    {
        ESP_LOGE("Telemetry", "Message buffer overflow");
        return ESP_ERR_INVALID_SIZE;
    }

    // Publish to single topic
    err = sim7080g_mqtt_publish(handle, base_topic, message, 0, false);
    if (err != ESP_OK)
    {
        ESP_LOGE("Telemetry", "Failed to publish device status: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}