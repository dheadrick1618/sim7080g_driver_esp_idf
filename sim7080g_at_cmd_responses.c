#include <string.h>
#include <stdio.h>
#include <ctype.h> // For 'isspace' fxn
#include <esp_err.h>
#include <esp_log.h>
#include "sim7080g_at_cmd_responses.h"

// ------------------- TEST (AT) ----------------------//
// ----------------------------------------------------//

esp_err_t parse_at_test_response(const char *response_str, at_test_parsed_response_t *parsed_response)
{
    if (!response_str || !parsed_response)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strstr(response_str, "OK") != NULL)
    {
        parsed_response->status = TEST_STATUS_OK;
    }
    else if (strstr(response_str, "ERROR") != NULL)
    {
        parsed_response->status = TEST_STATUS_ERROR;
    }
    else
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

const char *at_test_status_to_str(at_test_status_t status)
{
    static const char *const strings[] = {
        "OK",
        "ERROR"};

    if (status >= TEST_STATUS_MAX)
    {
        return "Invalid Status";
    }
    return strings[status];
}

// --------------------- CPIN -------------------------//
// -----------------------------------------------------//
esp_err_t parse_cpin_response(const char *response_str, cpin_parsed_response_t *parsed_response)
{
    if (response_str == NULL || parsed_response == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Find the +CPIN: response in the string
    const char *cpin_start = strstr(response_str, "+CPIN:");
    if (cpin_start == NULL)
    {
        // ESP_LOGE("FXN: parse_cpin_response", "Could not find +CPIN: in response");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Skip past "+CPIN: "
    cpin_start += 7;

    // Find the end of the status string (look for CR, LF, or null)
    const char *end = strpbrk(cpin_start, "\r\n");
    if (end == NULL)
    {
        end = cpin_start + strlen(cpin_start);
    }

    // Create a temporary buffer for the status string
    char status_str[32] = {0};
    size_t len = end - cpin_start;
    if (len >= sizeof(status_str))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    strncpy(status_str, cpin_start, len);
    status_str[len] = '\0';

    // Remove any trailing whitespace
    while (len > 0 && isspace((unsigned char)status_str[len - 1]))
    {
        status_str[--len] = '\0';
    }

    // Convert the status string to enum
    parsed_response->status = cpin_str_to_status(status_str);
    if (parsed_response->status == CPIN_STATUS_UNKNOWN)
    {
        // ESP_LOGE("FXN: parse_cpin_response", "Unknown CPIN status: %s", status_str);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Set requires_new_pin based on status
    parsed_response->requires_new_pin = (parsed_response->status == CPIN_STATUS_SIM_PUK ||
                                         parsed_response->status == CPIN_STATUS_PH_SIM_PUK ||
                                         parsed_response->status == CPIN_STATUS_SIM_PUK2);

    return ESP_OK;
}

const char *cpin_status_to_str(cpin_status_t status)
{
    static const char *const strings[] = {
        "Ready - No password required",
        "Waiting for SIM PIN",
        "Waiting for SIM PUK",
        "Waiting for Phone-to-SIM password",
        "Waiting for Phone-to-SIM PUK",
        "Waiting for Network personalization password",
        "Waiting for SIM PIN2",
        "Waiting for SIM PUK2",
        "Unknown status"};

    if (status >= CPIN_STATUS_MAX)
    {
        return "Invalid Status";
    }
    return strings[status];
}

cpin_status_t cpin_str_to_status(const char *status_str)
{
    if (!status_str)
    {
        return CPIN_STATUS_UNKNOWN;
    }

    struct
    {
        const char *str;
        cpin_status_t status;
    } status_map[] = {
        {"READY", CPIN_STATUS_READY},
        {"SIM PIN", CPIN_STATUS_SIM_PIN},
        {"SIM PUK", CPIN_STATUS_SIM_PUK},
        {"PH_SIM PIN", CPIN_STATUS_PH_SIM_PIN},
        {"PH_SIM PUK", CPIN_STATUS_PH_SIM_PUK},
        {"PH_NET PIN", CPIN_STATUS_PH_NET_PIN},
        {"SIM PIN2", CPIN_STATUS_SIM_PIN2},
        {"SIM PUK2", CPIN_STATUS_SIM_PUK2}};

    for (size_t i = 0; i < sizeof(status_map) / sizeof(status_map[0]); i++)
    {
        if (strcmp(status_str, status_map[i].str) == 0)
        {
            return status_map[i].status;
        }
    }

    return CPIN_STATUS_UNKNOWN;
}

// --------------------- CEREG -------------------------//
// -----------------------------------------------------//
// const char *cereg_mode_to_str(cereg_mode_t mode)
// {
//     static const char *const strings[] = {
//         "Disable network registration URC",
//         "Enable network registration URC",
//         "Enable network registration and location URC",
//         "Enable network registration, location and PSM URC"};

//     if (mode >= CEREG_MODE_MAX)
//     {
//         return "Invalid Mode";
//     }
//     return strings[mode];
// }

// const char *cereg_status_to_str(cereg_status_t status)
// {
//     static const char *const strings[] = {
//         "Not registered, not searching",
//         "Registered, home network",
//         "Not registered, searching",
//         "Registration denied",
//         "Unknown",
//         "Registered, roaming"};

//     if (status >= CEREG_STATUS_MAX)
//     {
//         return "Invalid Status";
//     }
//     return strings[status];
// }

// const char *cereg_act_to_str(cereg_act_t act)
// {
//     static const char *const strings[] = {
//         "GSM",
//         "Unknown",
//         "Unknown",
//         "Unknown",
//         "Unknown",
//         "Unknown",
//         "Unknown",
//         "LTE M1",
//         "Unknown",
//         "LTE NB"};

//     if (act >= CEREG_ACT_MAX)
//     {
//         return "Invalid Access Technology";
//     }
//     return strings[act];
// }

// esp_err_t parse_cereg_response(const char *response_str, cereg_response_t *response)
// {
//     if (!response_str || !response)
//     {
//         return ESP_ERR_INVALID_ARG;
//     }

//     // Initialize response structure
//     memset(response, 0, sizeof(cereg_response_t));

//     // Basic parsing for minimum parameters (mode and status)
//     int mode, stat;
//     if (sscanf(response_str, "+CEREG: %d,%d", &mode, &stat) != 2)
//     {
//         return ESP_ERR_INVALID_RESPONSE;
//     }

//     response->mode = (cereg_mode_t)mode;
//     response->status = (cereg_status_t)stat;

//     // Check if we have location information
//     if (strstr(response_str, ",\"") != NULL)
//     {
//         response->has_location_info = true;

//         // Parse location information
//         char tac[5] = {0}, ci[5] = {0};
//         int act;

//         if (sscanf(strstr(response_str, ",\""), ",\"%4[^\"]\",,\"%4[^\"]\",%d",
//                    tac, ci, &act) == 3)
//         {
//             strncpy(response->tac, tac, sizeof(response->tac) - 1);
//             strncpy(response->ci, ci, sizeof(response->ci) - 1);
//             response->act = (cereg_act_t)act;
//         }

//         // Check if we have PSM information (only valid when mode is 4)
//         if (response->mode == CEREG_MODE_ENABLE_URC_PSM &&
//             strstr(response_str, ",[,[") != NULL)
//         {
//             response->has_psm_info = true;

//             char active_time[9] = {0}, periodic_tau[9] = {0};
//             if (sscanf(strstr(response_str, ",[,["), ",[,[%8[^]],[%8[^]]]",
//                        active_time, periodic_tau) == 2)
//             {
//                 strncpy(response->active_time, active_time,
//                         sizeof(response->active_time) - 1);
//                 strncpy(response->periodic_tau, periodic_tau,
//                         sizeof(response->periodic_tau) - 1);
//             }
//         }
//     }

//     return ESP_OK;
// }