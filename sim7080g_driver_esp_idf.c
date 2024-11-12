#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

#include "sim7080g_uart.h"
#include "sim7080g_driver_esp_idf.h"
#include "sim7080g_at_cmds.h"
#include "sim7080g_commands.h"

static const char *TAG = "SIM7080G Driver";

esp_err_t sim7080g_config(sim7080g_handle_t *sim7080g_handle,
                          const sim7080g_uart_config_t sim7080g_uart_config,
                          const sim7080g_mqtt_config_t sim7080g_mqtt_config)
{
    // TODO - validate config params
    sim7080g_handle->uart_config = sim7080g_uart_config;

    sim7080g_handle->mqtt_config = sim7080g_mqtt_config;

    // sim7080g_log_config_params(sim7080g_handle);

    return ESP_OK;
}

// ---------------------  EXTERNAL EXPOSED API FXNs  -------------------------//
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

    // The sim7080g can remain powered and init while the ESP32 restarts - so it may not be necessary to always set the mqtt params on driver init
    // bool params_match;
    // sim7080g_mqtt_check_parameters_match(sim7080g_handle, &params_match);
    // if (params_match == true)
    // {
    //     ESP_LOGI(TAG, "Device MQTT parameter check: Param values already match Config values - skipping MQTT set params on init");
    // }
    // else
    // {
    //     err = sim7080g_mqtt_set_parameters(sim7080g_handle);
    //     if (err != ESP_OK)
    //     {
    //         ESP_LOGE(TAG, "Device MQTT parameter set: Error init MQTT for device (writing config params to sim7080g): %s", esp_err_to_name(err));
    //         sim7080g_handle->mqtt_initialized = false;
    //         return err;
    //     }
    //     else
    //     {
    //         ESP_LOGI(TAG, "Device MQTT parameter set: Success - MQTT params set on device");
    //         sim7080g_handle->mqtt_initialized = true;
    //     }
    // }
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
