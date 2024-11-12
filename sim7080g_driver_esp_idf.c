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
    char url_with_port[SMCONF_URL_MAX_LEN + 32] = {0};
    char device_url[SMCONF_URL_MAX_LEN] = {0};
    uint16_t device_port = 0;

    // Parse device URL and port by splitting on comma
    char *comma = strchr(device_params.url, ',');
    if (comma != NULL)
    {
        size_t url_len = comma - device_params.url;
        strncpy(device_url, device_params.url, url_len);
        device_port = (uint16_t)atoi(comma + 1);
    }
    else
    {
        strncpy(device_url, device_params.url, sizeof(device_url) - 1);
        device_port = device_params.port;
    }

    // Now compare the actual URL and port separately
    bool url_different = strcmp(device_url, handle->mqtt_config.broker_url) != 0;
    bool port_different = device_port != handle->mqtt_config.port;

    if (url_different || port_different)
    {
        snprintf(url_with_port, sizeof(url_with_port), "%s,%u",
                 handle->mqtt_config.broker_url,
                 handle->mqtt_config.port);

        err = sim7080g_set_mqtt_param(handle, SMCONF_PARAM_URL, url_with_port);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set URL and port: %s", esp_err_to_name(err));
            return err;
        }
        params_updated = true;
        ESP_LOGI(TAG, "Updated broker URL and port to: %s", url_with_port);
    }

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
    const char *TAG = "Network Connect";

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
