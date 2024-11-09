#include "sim7080g_at_cmd_responses.h"
#include <string.h>
#include <stdio.h>

// --------------------- CPIN -------------------------//
// -----------------------------------------------------//
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

cpin_status_t cpin_str_to_status(const char *const status_str)
{
    static const struct
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

    if (NULL == status_str)
    {
        return CPIN_STATUS_UNKNOWN;
    }

    /* Trim any trailing whitespace or \r\n */
    char trimmed[32];
    size_t len = strlen(status_str);
    if (len >= sizeof(trimmed))
    {
        return CPIN_STATUS_UNKNOWN;
    }

    strncpy(trimmed, status_str, sizeof(trimmed) - 1U);
    trimmed[sizeof(trimmed) - 1U] = '\0';

    while (len > 0U && (trimmed[len - 1U] == ' ' ||
                        trimmed[len - 1U] == '\r' ||
                        trimmed[len - 1U] == '\n'))
    {
        trimmed[len - 1U] = '\0';
        len--;
    }

    for (size_t i = 0U; i < (sizeof(status_map) / sizeof(status_map[0])); i++)
    {
        if (0 == strcmp(trimmed, status_map[i].str))
        {
            return status_map[i].status;
        }
    }

    return CPIN_STATUS_UNKNOWN;
}

esp_err_t parse_cpin_response(const char *const response_str, cpin_response_t *const response)
{
    const char *const EXPECTED_PREFIX = "+CPIN: ";
    const size_t PREFIX_LEN = 7U;
    /* Use fixed size array instead of VLA */
    char status_str[32U]; /* Fixed size buffer */
    size_t status_len;
    const char *status_start;
    const char *line_end;

    if ((NULL == response_str) || (NULL == response))
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Initialize response and local buffer */
    (void)memset(response, 0, sizeof(cpin_response_t));
    (void)memset(status_str, 0, sizeof(status_str));

    /* Verify response starts with expected prefix */
    if (0 != strncmp(response_str, EXPECTED_PREFIX, PREFIX_LEN))
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* Extract status string up to first \r or \n */
    status_start = response_str + PREFIX_LEN;
    line_end = strpbrk(status_start, "\r\n");

    if (NULL != line_end)
    {
        status_len = (size_t)(line_end - status_start);
    }
    else
    {
        status_len = strlen(status_start);
    }

    /* Check if status string would fit in our buffer */
    if ((0U == status_len) || (status_len >= sizeof(status_str)))
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* Copy and null-terminate status string */
    (void)memcpy(status_str, status_start, status_len);
    status_str[status_len] = '\0';

    /* Convert string status to enum */
    response->status = cpin_str_to_status(status_str);
    if (CPIN_STATUS_UNKNOWN == response->status)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* Set requires_new_pin flag */
    response->requires_new_pin = ((CPIN_STATUS_SIM_PUK == response->status) ||
                                  (CPIN_STATUS_SIM_PUK2 == response->status));

    return ESP_OK;
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