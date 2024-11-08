#include "sim7080g_commands.h"
#include "sim7080g_at_cmd_responses.h"

#define AT_RESPONSE_MAX_LEN 256U
#define AT_CMD_MAX_LEN 256U

static const char *TAG = "SIM7080G Commands";

// --------------------------------------- STATIC / HELPER FXNS --------------------------------------- //

// send AT command

// parse AT response

// Log MQTT configuration parameters

// --------------------------------------- FXNS to use SIM7080G AT Commands --------------------------------------- //

// Cycle CFUN - soft reset (CFUN)

// Test AT communication with device (AT)

// Disable command Echo (ATE0)

// Enable verbose error reporting (CMEE)

// Check SIM status (CPIN)
// esp_err_t sim7080g_check_sim_status(const sim7080g_handle_t *sim7080g_handle)
// {
//     if (!sim7080g_handle)
//     {
//         ESP_LOGE(TAG, "Invalid parameters");
//         return ESP_ERR_INVALID_ARG;
//     }

//     ESP_LOGI(TAG, "Sending check SIM status cmd");

//     char response[AT_RESPONSE_MAX_LEN] = {0};
//     esp_err_t ret = send_at_cmd(sim7080g_handle, &AT_CPIN, AT_CMD_TYPE_READ, NULL, response, sizeof(response), 5000);
//     if (ret == ESP_OK)
//     {
//         if (strstr(response, "READY") != NULL)
//         {
//             ESP_LOGI(TAG, "SIM card is ready");
//             return ESP_OK;
//         }
//         else
//         {
//             ESP_LOGE(TAG, "SIM card is not ready: %s", response);
//             return ESP_FAIL;
//         }
//     }
//     return ret;
// }

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
