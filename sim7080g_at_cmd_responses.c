#include <string.h>
#include <stdio.h>
#include <ctype.h> // For 'isspace' fxn
#include <esp_err.h>
#include <esp_log.h>
#include "sim7080g_types.h"
#include "sim7080g_at_cmd_responses.h"

// ------------------- TEST (AT) ----------------------//
// ----------------------------------------------------//

esp_err_t parse_at_test_response(const char *response_str, at_test_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
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
esp_err_t parse_cpin_response(const char *response_str, cpin_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if (response_str == NULL || parsed_response == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For WRITE command, just verify we got OK
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
        else
        {
            // TODO - Setup error handling fxn to log error
        }
        return ESP_ERR_INVALID_RESPONSE;
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

// --------------------- CFUN -------------------------//
// -----------------------------------------------------//

esp_err_t parse_cfun_response(const char *response_str, cfun_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For WRITE command, just verify we got OK
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
        else
        {
            // TODO - Setup error handling fxn to log error
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    const char *cfun_start = strstr(response_str, "+CFUN:");
    if (cfun_start == NULL)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Rest of the existing parsing logic for QUERY...
    cfun_start += 6;
    memset(parsed_response, 0, sizeof(cfun_parsed_response_t));
    int fun_level = 0;
    int items_matched = sscanf(cfun_start, " %d %*[\r\n]", &fun_level);

    if (items_matched != 1)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if ((fun_level < 0) || (fun_level >= (int)CFUN_FUNCTIONALITY_MAX))
    {
        return ESP_ERR_INVALID_STATE;
    }

    parsed_response->functionality = (cfun_functionality_t)fun_level;
    return ESP_OK;
}

const char *cfun_functionality_to_str(cfun_functionality_t functionality)
{
    static const char *const strings[] = {
        "Minimum functionality",
        "Full functionality",
        "Invalid status", // 2 is not used
        "Invalid status", // 3 is not used
        "Disable RF circuits",
        "Factory Test Mode",
        "Reset",
        "Offline Mode"};

    if (functionality >= CFUN_FUNCTIONALITY_MAX)
    {
        return "Invalid Status";
    }
    return strings[functionality];
}

// --------------------- CSQ -------------------------//
// ----------------------------------------------------//

esp_err_t parse_csq_response(const char *response_str, csq_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Find the +CSQ: response in the string
    const char *csq_start = strstr(response_str, "+CSQ:");
    if (csq_start == NULL)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Skip past "+CSQ: "
    csq_start += 5;

    // Parse RSSI and BER values
    int rssi = 0;
    int ber = 0;
    int items_matched = sscanf(csq_start, " %d,%d", &rssi, &ber);

    if (items_matched != 2)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Validate RSSI
    if (((rssi < 0) || (rssi > 31)) && (rssi != CSQ_RSSI_NOT_DETECTABLE))
    {
        return ESP_ERR_INVALID_STATE;
    }

    // Validate BER
    if (((ber < 0) || (ber > 7)) && (ber != CSQ_BER_NOT_DETECTABLE))
    {
        return ESP_ERR_INVALID_STATE;
    }

    parsed_response->rssi = (csq_rssi_t)rssi;
    parsed_response->ber = (csq_ber_t)ber;
    parsed_response->rssi_dbm = csq_rssi_to_dbm(parsed_response->rssi);

    return ESP_OK;
}

const char *csq_rssi_to_str(csq_rssi_t rssi)
{
    if (rssi == CSQ_RSSI_NOT_DETECTABLE)
    {
        return "Not detectable";
    }
    else if (rssi >= CSQ_RSSI_MAX)
    {
        return "Invalid RSSI";
    }
    else
    {
        static char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d (%d dBm)", rssi, csq_rssi_to_dbm(rssi));
        return buffer;
    }
}

const char *csq_ber_to_str(csq_ber_t ber)
{
    if (ber == CSQ_BER_NOT_DETECTABLE)
    {
        return "Not detectable";
    }
    else if (ber >= CSQ_BER_MAX)
    {
        return "Invalid BER";
    }
    else
    {
        static char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d%%", ber);
        return buffer;
    }
}

int8_t csq_rssi_to_dbm(csq_rssi_t rssi)
{
    if (rssi == CSQ_RSSI_NOT_DETECTABLE)
    {
        return INT8_MIN; // Use INT8_MIN to represent "not detectable"
    }
    else if (rssi == 0)
    {
        return -115;
    }
    else if (rssi == 1)
    {
        return -111;
    }
    else if (rssi >= 2 && rssi <= 30)
    {
        return (int8_t)(-110 + ((rssi - 2) * 2));
    }
    else if (rssi == 31)
    {
        return -52;
    }

    return INT8_MIN; // Invalid RSSI
}

// --------------------- ATE -------------------------//
// ----------------------------------------------------//

esp_err_t parse_ate_response(const char *response_str, ate_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For ATE command, we only need to verify OK response (theres ONLY execution type command)
    // The mode is set by the command itself, not parsed from response
    if (strstr(response_str, "OK") != NULL)
    {
        return ESP_OK;
    }

    return ESP_FAIL;
}

const char *ate_mode_to_str(ate_mode_t mode)
{
    static const char *const strings[] = {
        "Echo mode off",
        "Echo mode on"};

    if (mode >= ATE_MODE_MAX)
    {
        return "Invalid Mode";
    }
    return strings[mode];
}

// --------------------- CMEE -------------------------//
// ----------------------------------------------------//

esp_err_t parse_cmee_response(const char *response_str, cmee_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        // For CMEE command, we only need to verify OK response
        // The mode is set by the command itself, not parsed from response
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
    }

    // Find the +CMEE: response in the string
    const char *cmee_start = strstr(response_str, "+CMEE:");
    if (cmee_start == NULL)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Skip past "+CMEE: "
    cmee_start += 6;

    // Parse the mode
    int mode = 0;
    int items_matched = sscanf(cmee_start, " %d", &mode);

    if (items_matched != 1)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if ((mode < 0) || (mode >= (int)CMEE_MODE_MAX))
    {
        return ESP_ERR_INVALID_STATE;
    }

    parsed_response->mode = (cmee_mode_t)mode;
    return ESP_OK;
}

const char *cmee_mode_to_str(cmee_mode_t mode)
{
    static const char *const strings[] = {
        "Disable CME ERROR Codes (just use 'ERROR' instead)",
        "Enable Numeric CME ERROR codes",
        "Enable Verbose CME ERROR codes"};

    if (mode >= CMEE_MODE_MAX)
    {
        return "Invalid Mode";
    }
    return strings[mode];
}

// --------------------- CGDCONT -------------------------//
// ----------------------------------------------------//

esp_err_t parse_cgdcont_response(const char *response_str, cgdcont_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For WRITE command, just verify we got OK
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    // For READ command, parse the current PDP context configurations
    if (cmd_type == AT_CMD_TYPE_READ)
    {
        const char *pos = response_str;
        parsed_response->num_contexts = 0;

        while ((pos = strstr(pos, "+CGDCONT: ")) != NULL)
        {
            if (parsed_response->num_contexts >= CGDCONT_MAX_CID)
            {
                return ESP_ERR_INVALID_SIZE;
            }

            cgdcont_config_t *context = &parsed_response->contexts[parsed_response->num_contexts];
            char pdp_type_str[32] = {0};

            // Parse the context ID, PDP type, and APN
            int matched = sscanf(pos, "+CGDCONT: %hhu,\"%31[^\"]\",\"%99[^\"]\"",
                                 &context->cid,
                                 pdp_type_str,
                                 context->apn);

            if (matched < 2) // At minimum need CID and PDP type
            {
                return ESP_ERR_INVALID_RESPONSE;
            }

            // Validate CID
            if (context->cid < CGDCONT_MIN_CID || context->cid > CGDCONT_MAX_CID)
            {
                return ESP_ERR_INVALID_ARG;
            }

            // Convert PDP type string to enum
            context->pdp_type = cgdcont_str_to_pdp_type(pdp_type_str);
            if (context->pdp_type >= CGDCONT_PDP_TYPE_MAX)
            {
                return ESP_ERR_INVALID_ARG;
            }

            parsed_response->num_contexts++;
            pos += 9; // Move past "+CGDCONT: "
        }

        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

const char *cgdcont_pdp_type_to_str(cgdcont_pdp_type_t pdp_type)
{
    static const char *const strings[] = {
        "IP",
        "PPP",
        "IPV6",
        "IPV4V6",
        "Non-IP"};

    if (pdp_type >= CGDCONT_PDP_TYPE_MAX)
    {
        return "Invalid PDP Type";
    }
    return strings[pdp_type];
}

cgdcont_pdp_type_t cgdcont_str_to_pdp_type(const char *pdp_type_str)
{
    if (!pdp_type_str)
    {
        return CGDCONT_PDP_TYPE_MAX;
    }

    struct
    {
        const char *str;
        cgdcont_pdp_type_t type;
    } type_map[] = {
        {"IP", CGDCONT_PDP_TYPE_IP},
        {"PPP", CGDCONT_PDP_TYPE_PPP},
        {"IPV6", CGDCONT_PDP_TYPE_IPV6},
        {"IPV4V6", CGDCONT_PDP_TYPE_IPV4V6},
        {"Non-IP", CGDCONT_PDP_TYPE_NONIP}};

    for (size_t i = 0; i < sizeof(type_map) / sizeof(type_map[0]); i++)
    {
        if (strcmp(pdp_type_str, type_map[i].str) == 0)
        {
            return type_map[i].type;
        }
    }

    return CGDCONT_PDP_TYPE_MAX;
}

// --------------------- CGATT -------------------------//
// ----------------------------------------------------//

esp_err_t parse_cgatt_response(const char *response_str, cgatt_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For WRITE command, just verify we got OK
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    // For READ command, parse the GPRS attachment state
    if (cmd_type == AT_CMD_TYPE_READ)
    {
        // Find the +CGATT: response in the string
        const char *cgatt_start = strstr(response_str, "+CGATT:");
        if (cgatt_start == NULL)
        {
            return ESP_ERR_INVALID_RESPONSE;
        }

        // Skip past "+CGATT: "
        cgatt_start += 7;

        int state = 0;
        if (sscanf(cgatt_start, "%d", &state) != 1)
        {
            return ESP_ERR_INVALID_RESPONSE;
        }

        // Validate state value
        if ((state != CGATT_STATE_DETACHED) && (state != CGATT_STATE_ATTACHED))
        {
            return ESP_ERR_INVALID_STATE;
        }

        parsed_response->state = (cgatt_state_t)state;
        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

const char *cgatt_state_to_str(cgatt_state_t state)
{
    static const char *const strings[] = {
        "Detached from GPRS service",
        "Attached to GPRS service"};

    if (state >= CGATT_STATE_MAX)
    {
        return "Invalid GPRS attachment state";
    }
    return strings[state];
}

// --------------------- COPS -------------------------//
// ----------------------------------------------------//
esp_err_t parse_cops_response(const char *response_str, cops_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For READ command, parse current operator info
    if (cmd_type == AT_CMD_TYPE_READ)
    {
        // Find the +COPS: response in the string
        const char *cops_start = strstr(response_str, "+COPS:");
        if (cops_start == NULL)
        {
            return ESP_ERR_INVALID_RESPONSE;
        }

        // Skip past "+COPS: "
        cops_start += 7;

        // Initialize the parsed_response structure
        memset(parsed_response, 0, sizeof(cops_parsed_response_t));

        int mode = 0;
        int format = 0;
        int act = 0;

        // Try to parse with operator name and access technology
        int matched = sscanf(cops_start, "%d,%d,\"%31[^\"]\",%d",
                             &mode, &format,
                             parsed_response->operator_name,
                             &act);

        if (matched < 1) // At minimum need mode
        {
            return ESP_ERR_INVALID_RESPONSE;
        }

        // Validate mode
        if ((mode < 0) || (mode >= (int)COPS_MODE_MAX))
        {
            return ESP_ERR_INVALID_STATE;
        }
        parsed_response->mode = (cops_mode_t)mode;

        // If we got format and operator name
        if (matched >= 2)
        {
            if ((format < 0) || (format >= (int)COPS_FORMAT_MAX))
            {
                return ESP_ERR_INVALID_STATE;
            }
            parsed_response->format = (cops_format_t)format;
        }

        // If we got access technology
        if (matched == 4)
        {
            if ((act < 0) || (act >= (int)COPS_ACT_MAX))
            {
                return ESP_ERR_INVALID_STATE;
            }
            parsed_response->access_tech = (cops_access_tech_t)act;
        }

        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

const char *cops_mode_to_str(cops_mode_t mode)
{
    static const char *const strings[] = {
        "Automatic",
        "Manual",
        "Deregister",
        "Set Format Only",
        "Manual/Automatic"};

    if (mode >= COPS_MODE_MAX)
    {
        return "Invalid Mode";
    }
    return strings[mode];
}

const char *cops_format_to_str(cops_format_t format)
{
    static const char *const strings[] = {
        "Long Alphanumeric",
        "Short Alphanumeric",
        "Numeric"};

    if (format >= COPS_FORMAT_MAX)
    {
        return "Invalid Format";
    }
    return strings[format];
}

const char *cops_access_tech_to_str(cops_access_tech_t tech)
{
    static const char *const strings[] = {
        "GSM",
        "GSM Compact",
        "Unknown",
        "GSM EGPRS",
        "Unknown",
        "Unknown",
        "Unknown",
        "LTE M1",
        "Unknown",
        "LTE NB"};

    if (tech >= COPS_ACT_MAX)
    {
        return "Invalid Access Technology";
    }
    return strings[tech];
}

// --------------------- CGNAPN -------------------------//
// -----------------------------------------------------//

esp_err_t parse_cgnapn_response(const char *response_str, cgnapn_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Find the +CGNAPN: response in the string
    const char *cgnapn_start = strstr(response_str, "+CGNAPN:");
    if (cgnapn_start == NULL)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Skip past "+CGNAPN: "
    cgnapn_start += 9;

    // Initialize response structure
    memset(parsed_response, 0, sizeof(cgnapn_parsed_response_t));

    int status = 0;

    // Parse status and APN
    // Note: Using sizeof-1 to ensure space for null terminator
    int matched = sscanf(cgnapn_start, "%d,\"%[^\"]\"",
                         &status,
                         parsed_response->network_apn);

    // We must at least get the status
    if (matched < 1)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Validate status
    if ((status != CGNAPN_APN_NOT_PROVIDED) &&
        (status != CGNAPN_APN_PROVIDED))
    {
        return ESP_ERR_INVALID_STATE;
    }

    parsed_response->status = (cgnapn_status_t)status;

    return ESP_OK;
}

const char *cgnapn_status_to_str(cgnapn_status_t status)
{
    static const char *const strings[] = {
        "Network APN not provided",
        "Network APN provided"};

    if (status >= CGNAPN_STATUS_MAX)
    {
        return "Invalid Status";
    }
    return strings[status];
}

// --------------------- CNCFG -------------------------//
// -----------------------------------------------------//
esp_err_t parse_cncfg_response(const char *response_str, cncfg_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *TAG = "parse_cncfg_response";

    // For WRITE command, just verify we got OK
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    // For READ command, parse all context configurations
    if (cmd_type == AT_CMD_TYPE_READ)
    {
        const char *search_pos = response_str;
        parsed_response->num_contexts = 0;

        while ((search_pos = strstr(search_pos, "+CNCFG:")) != NULL)
        {
            if (parsed_response->num_contexts >= CNCFG_MAX_CONTEXTS)
            {
                return ESP_ERR_INVALID_SIZE;
            }

            cncfg_context_config_t *context = &parsed_response->contexts[parsed_response->num_contexts];

            // Initialize fields
            memset(context, 0, sizeof(cncfg_context_config_t));

            char tmp_apn[CNCFG_MAX_APN_LEN] = {0};
            char tmp_username[CNCFG_MAX_USERNAME_LEN] = {0};
            char tmp_password[CNCFG_MAX_PASSWORD_LEN] = {0};
            int pdp_idx = 0;
            int ip_type = 0;
            int auth_type = 0;

            // Move past "CNCFG: "
            const char *line_start = search_pos + 7;

            // Find the end of this line
            const char *line_end = strstr(line_start, "\r\n");
            if (line_end == NULL)
            {
                line_end = line_start + strlen(line_start);
            }

            // Use a temporary buffer for this line
            char line_buffer[256] = {0};
            size_t line_len = line_end - line_start;
            if (line_len >= sizeof(line_buffer))
            {
                return ESP_ERR_INVALID_SIZE;
            }
            memcpy(line_buffer, line_start, line_len);

            // ESP_LOGI(TAG, "Parsing line: %s", line_buffer);

            // Parse the line using standard string operations
            char *token = strtok(line_buffer, ",");
            if (token == NULL)
                return ESP_ERR_INVALID_RESPONSE;
            pdp_idx = atoi(token);

            token = strtok(NULL, ",");
            if (token == NULL)
                return ESP_ERR_INVALID_RESPONSE;
            ip_type = atoi(token);

            // Parse APN (handle empty string case)
            token = strtok(NULL, ",");
            if (token == NULL)
                return ESP_ERR_INVALID_RESPONSE;
            if (strlen(token) > 2)
            { // More than just the quotes
                sscanf(token, "\"%[^\"]\"", tmp_apn);
            }

            // Parse username
            token = strtok(NULL, ",");
            if (token == NULL)
                return ESP_ERR_INVALID_RESPONSE;
            if (strlen(token) > 2)
            {
                sscanf(token, "\"%[^\"]\"", tmp_username);
            }

            // Parse password
            token = strtok(NULL, ",");
            if (token == NULL)
                return ESP_ERR_INVALID_RESPONSE;
            if (strlen(token) > 2)
            {
                sscanf(token, "\"%[^\"]\"", tmp_password);
            }

            // Parse auth type
            token = strtok(NULL, ",");
            if (token == NULL)
                return ESP_ERR_INVALID_RESPONSE;
            auth_type = atoi(token);

            // Validate fields
            if ((pdp_idx < 0) || (pdp_idx >= CNCFG_MAX_CONTEXTS))
            {
                ESP_LOGE(TAG, "Invalid PDP index: %d", pdp_idx);
                return ESP_ERR_INVALID_ARG;
            }
            context->pdp_idx = (uint8_t)pdp_idx;

            if ((ip_type < 0) || (ip_type >= (int)CNCFG_IP_TYPE_MAX))
            {
                ESP_LOGE(TAG, "Invalid IP type: %d", ip_type);
                return ESP_ERR_INVALID_ARG;
            }
            context->ip_type = (cncfg_ip_type_t)ip_type;

            if ((auth_type < 0) || (auth_type >= (int)CNCFG_AUTH_MAX))
            {
                ESP_LOGE(TAG, "Invalid auth type: %d", auth_type);
                return ESP_ERR_INVALID_ARG;
            }
            context->auth_type = (cncfg_auth_type_t)auth_type;

            // Copy strings
            strncpy(context->apn, tmp_apn, CNCFG_MAX_APN_LEN - 1);
            strncpy(context->username, tmp_username, CNCFG_MAX_USERNAME_LEN - 1);
            strncpy(context->password, tmp_password, CNCFG_MAX_PASSWORD_LEN - 1);

            // ESP_LOGI(TAG, "Successfully parsed context %d: PDP=%d, IP=%d, APN='%s', Auth=%d",
            //          parsed_response->num_contexts, context->pdp_idx, context->ip_type,
            //          context->apn[0] ? context->apn : "<empty>", context->auth_type);

            parsed_response->num_contexts++;
            search_pos = line_end;
        }

        return (parsed_response->num_contexts > 0) ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_ERR_INVALID_RESPONSE;
}
const char *cncfg_ip_type_to_str(cncfg_ip_type_t ip_type)
{
    static const char *const strings[] = {
        "Dual PDN Stack",
        "IPv4",
        "IPv6",
        "Non-IP",
        "Extended Non-IP"};

    if (ip_type >= CNCFG_IP_TYPE_MAX)
    {
        return "Invalid IP Type";
    }
    return strings[ip_type];
}

const char *cncfg_auth_type_to_str(cncfg_auth_type_t auth_type)
{
    static const char *const strings[] = {
        "None",
        "PAP",
        "CHAP",
        "PAP or CHAP"};

    if (auth_type >= CNCFG_AUTH_MAX)
    {
        return "Invalid Auth Type";
    }
    return strings[auth_type];
}

// --------------------- CNACT -------------------------//
// ----------------------------------------------------//

esp_err_t parse_cnact_response(const char *response_str, cnact_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *TAG = "parse_cnact_response";

    // For WRITE command, log what we received and be more lenient
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        // Log the raw response for debugging
        ESP_LOGI(TAG, "CNACT Write Response: %s", response_str);

        // Accept any non-error response for now
        if (strlen(response_str) > 0 && strstr(response_str, "ERROR") == NULL)
        {
            ESP_LOGI(TAG, "CNACT command accepted - awaiting network activation");
            return ESP_OK;
        }

        ESP_LOGE(TAG, "CNACT command rejected with response: %s", response_str);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // For READ command, parse all context statuses
    if (cmd_type == AT_CMD_TYPE_READ)
    {
        const char *pos = response_str;
        parsed_response->num_contexts = 0;

        while ((pos = strstr(pos, "+CNACT:")) != NULL)
        {
            if (parsed_response->num_contexts >= CNACT_MAX_CONTEXTS)
            {
                return ESP_ERR_INVALID_SIZE;
            }

            cnact_context_info_t *context = &parsed_response->contexts[parsed_response->num_contexts];
            int pdp_idx = 0;
            int status = 0;

            // Skip past "+CNACT: "
            pos += 7;

            // Parse context info
            int matched = sscanf(pos, "%d,%d,\"%[^\"]\"",
                                 &pdp_idx,
                                 &status,
                                 context->ip_address);

            if (matched != 3) // Need all three fields
            {
                return ESP_ERR_INVALID_RESPONSE;
            }

            // Validate fields
            if ((pdp_idx < 0) || (pdp_idx >= CNACT_MAX_CONTEXTS))
            {
                return ESP_ERR_INVALID_ARG;
            }
            context->pdp_idx = (uint8_t)pdp_idx;

            if ((status < 0) || (status >= (int)CNACT_STATUS_MAX))
            {
                return ESP_ERR_INVALID_ARG;
            }
            context->status = (cnact_status_t)status;

            parsed_response->num_contexts++;

            // Move to next line
            pos = strchr(pos, '\n');
            if (pos == NULL)
            {
                break;
            }
        }

        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

const char *cnact_action_to_str(cnact_action_t action)
{
    static const char *const strings[] = {
        "Deactivate",
        "Activate",
        "Auto Activate"};

    if (action >= CNACT_ACTION_MAX)
    {
        return "Invalid Action";
    }
    return strings[action];
}

const char *cnact_status_to_str(cnact_status_t status)
{
    static const char *const strings[] = {
        "Deactivated",
        "Activated",
        "In Operation"};

    if (status >= CNACT_STATUS_MAX)
    {
        return "Invalid Status";
    }
    return strings[status];
}

// --------------------- SMCONF -------------------------//
// ----------------------------------------------------//
esp_err_t parse_smconf_response(const char *response_str, smconf_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }
    const char *TAG = "parse_smconf_response";

    // For WRITE command, just verify we got OK
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    // For READ command, parse all parameters
    if (cmd_type == AT_CMD_TYPE_READ)
    {
        smconf_config_t *config = &parsed_response->config;
        memset(config, 0, sizeof(smconf_config_t));

        // Find start of configuration block
        const char *conf_start = strstr(response_str, "+SMCONF:");
        if (conf_start == NULL)
        {
            ESP_LOGE(TAG, "Could not find +SMCONF:");
            return ESP_ERR_INVALID_RESPONSE;
        }

        // Find first param after +SMCONF:
        const char *line_start = strstr(conf_start, "\r\n");
        if (line_start == NULL)
        {
            ESP_LOGE(TAG, "Could not find start of parameters");
            return ESP_ERR_INVALID_RESPONSE;
        }
        line_start += 2; // Skip \r\n

        // Prepare for line-by-line parsing
        char line[SMCONF_MESSAGE_MAX_LEN + 32] = {0};
        char param_name[32] = {0};
        char param_value[SMCONF_MESSAGE_MAX_LEN + 1] = {0};
        const char *current_pos = line_start;

        // Parse line by line until we hit "OK" or end of string
        while (current_pos && *current_pos)
        {
            // Find end of current line
            const char *line_end = strstr(current_pos, "\r\n");
            if (line_end == NULL)
            {
                break;
            }

            // Verify line length
            size_t line_len = line_end - current_pos;
            if (line_len == 0 || line_len >= sizeof(line))
            {
                current_pos = line_end + 2;
                continue;
            }

            // Copy line for parsing
            memset(line, 0, sizeof(line));
            strncpy(line, current_pos, line_len);

            // Skip empty lines or OK
            if (strcmp(line, "OK") == 0 || line[0] == '\0')
            {
                break;
            }

            // ESP_LOGI(TAG, "Parsing line: %s", line);

            // Parse parameter line
            if (sscanf(line, "%31[^:]: \"%[^\"]\"", param_name, param_value) == 2 ||
                sscanf(line, "%31[^:]: %[^\r\n]", param_name, param_value) == 2)
            {
                if (strcmp(param_name, "CLIENTID") == 0)
                {
                    strncpy(config->client_id, param_value, SMCONF_CLIENTID_MAX_LEN - 1);
                }
                else if (strcmp(param_name, "URL") == 0)
                {
                    char *port_str = strchr(param_value, ',');
                    if (port_str != NULL)
                    {
                        *port_str = '\0';
                        port_str++;
                        config->port = (uint16_t)atoi(port_str);
                    }
                    strncpy(config->url, param_value, SMCONF_URL_MAX_LEN - 1);
                }
                else if (strcmp(param_name, "KEEPTIME") == 0)
                {
                    config->keeptime = (uint16_t)atoi(param_value);
                }
                else if (strcmp(param_name, "USERNAME") == 0)
                {
                    strncpy(config->username, param_value, SMCONF_USERNAME_MAX_LEN - 1);
                }
                else if (strcmp(param_name, "PASSWORD") == 0)
                {
                    strncpy(config->password, param_value, SMCONF_PASSWORD_MAX_LEN - 1);
                }
                else if (strcmp(param_name, "CLEANSS") == 0)
                {
                    config->clean_session = (atoi(param_value) == 1);
                }
                else if (strcmp(param_name, "QOS") == 0)
                {
                    int qos = atoi(param_value);
                    if (qos >= 0 && qos < SMCONF_QOS_MAX)
                    {
                        config->qos = (smconf_qos_t)qos;
                    }
                }
                else if (strcmp(param_name, "TOPIC") == 0)
                {
                    strncpy(config->topic, param_value, SMCONF_TOPIC_MAX_LEN - 1);
                }
                else if (strcmp(param_name, "MESSAGE") == 0)
                {
                    strncpy(config->message, param_value, SMCONF_MESSAGE_MAX_LEN - 1);
                }
                else if (strcmp(param_name, "RETAIN") == 0)
                {
                    config->retain = (atoi(param_value) == 1);
                }
                else if (strcmp(param_name, "SUBHEX") == 0)
                {
                    config->subhex = (atoi(param_value) == 1);
                }
                else if (strcmp(param_name, "ASYNCMODE") == 0)
                {
                    config->async_mode = (atoi(param_value) == 1);
                }
                // Ignore any unknown parameters without warning
            }

            // Move to next line
            current_pos = line_end + 2;
        }

        // Log parsed configuration
        ESP_LOGI(TAG, "Parsed MQTT Configuration:");
        ESP_LOGI(TAG, "  Client ID: %s", config->client_id[0] ? config->client_id : "<empty>");
        ESP_LOGI(TAG, "  URL: %s", config->url[0] ? config->url : "<empty>");
        ESP_LOGI(TAG, "  Keep Time: %u", config->keeptime);
        ESP_LOGI(TAG, "  Username: %s", config->username[0] ? config->username : "<empty>");
        ESP_LOGI(TAG, "  Password: %s", config->password[0] ? "*****" : "<empty>");
        ESP_LOGI(TAG, "  Clean Session: %s", config->clean_session ? "Yes" : "No");
        ESP_LOGI(TAG, "  QoS: %s", smconf_qos_to_str(config->qos));
        ESP_LOGI(TAG, "  Topic: %s", config->topic[0] ? config->topic : "<empty>");
        ESP_LOGI(TAG, "  Message: %s", config->message[0] ? config->message : "<empty>");
        ESP_LOGI(TAG, "  Retain: %s", config->retain ? "Yes" : "No");
        ESP_LOGI(TAG, "  SubHex: %s", config->subhex ? "Yes" : "No");
        ESP_LOGI(TAG, "  Async Mode: %s", config->async_mode ? "Yes" : "No");

        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

const char *smconf_param_to_str(smconf_param_t param)
{
    static const char *const strings[] = {
        "CLIENTID",
        "URL",
        "KEEPTIME",
        "USERNAME",
        "PASSWORD",
        "CLEANSS",
        "QOS",
        "TOPIC",
        "MESSAGE",
        "RETAIN",
        "SUBHEX",
        "ASYNCMODE"};

    if (param >= SMCONF_PARAM_MAX)
    {
        return "Invalid Parameter";
    }
    return strings[param];
}

const char *smconf_qos_to_str(smconf_qos_t qos)
{
    static const char *const strings[] = {
        "QoS 0",
        "QoS 1",
        "QoS 2"};

    if (qos >= SMCONF_QOS_MAX)
    {
        return "Invalid QoS";
    }
    return strings[qos];
}

// --------------------- SMCONN -------------------------//
// ----------------------------------------------------//

esp_err_t parse_smconn_response(const char *response_str, smconn_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strstr(response_str, "OK") != NULL)
    {
        parsed_response->status = SMCONN_STATUS_SUCCESS;
        return ESP_OK;
    }
    else if (strstr(response_str, "ERROR") != NULL)
    {
        parsed_response->status = SMCONN_STATUS_ERROR;
        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

// --------------------- SMPUB -------------------------//
// ----------------------------------------------------//
esp_err_t parse_smpub_response(const char *response_str, smpub_parsed_response_t *parsed_response, at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (cmd_type == AT_CMD_TYPE_TEST)
    {
        if (strstr(response_str, "+SMPUB:") != NULL && strstr(response_str, "OK"))
        {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    // For write command
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, ">") != NULL)
        {
            parsed_response->status = SMPUB_STATUS_SUCCESS;
            return ESP_OK;
        }
        else if (strstr(response_str, "ERROR") != NULL)
        {
            parsed_response->status = SMPUB_STATUS_ERROR;
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

const char *smpub_status_to_str(smpub_status_t status)
{
    static const char *const strings[] = {
        "Success",
        "Error",
        "Timeout",
        "Not Connected"};

    if (status >= SMPUB_STATUS_MAX)
    {
        return "Invalid Status";
    }
    return strings[status];
}

// -------------------- SMSTATE -------------------------//
// ----------------------------------------------------//

esp_err_t parse_smstate_response(const char *response_str,
                                 smstate_parsed_response_t *parsed_response,
                                 at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For READ command, parse the state
    if (cmd_type == AT_CMD_TYPE_READ)
    {
        // Find the +SMSTATE: response
        const char *state_start = strstr(response_str, "+SMSTATE:");
        if (state_start == NULL)
        {
            return ESP_ERR_INVALID_RESPONSE;
        }

        // Skip past "+SMSTATE: "
        state_start += 9;

        int status = 0;
        if (sscanf(state_start, "%d", &status) != 1)
        {
            return ESP_ERR_INVALID_RESPONSE;
        }

        // Validate status value
        if (status < 0 || status >= SMSTATE_STATUS_MAX)
        {
            return ESP_ERR_INVALID_STATE;
        }

        parsed_response->status = (smstate_status_t)status;
        return ESP_OK;
    }

    // For TEST command
    if (cmd_type == AT_CMD_TYPE_TEST)
    {
        if (strstr(response_str, "+SMSTATE: (0-2)") != NULL)
        {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

const char *smstate_status_to_str(smstate_status_t status)
{
    static const char *const strings[] = {
        "MQTT Disconnected",
        "MQTT Connected",
        "MQTT Connected with Session Present"};

    if (status >= SMSTATE_STATUS_MAX)
    {
        return "Invalid Status";
    }
    return strings[status];
}

// --------------------- CEREG -------------------------//
// -----------------------------------------------------//
esp_err_t parse_cereg_response(const char *response_str,
                               cereg_parsed_response_t *parsed_response,
                               at_cmd_type_t cmd_type)
{
    if ((response_str == NULL) || (parsed_response == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    // For WRITE command, just verify we got OK
    if (cmd_type == AT_CMD_TYPE_WRITE)
    {
        if (strstr(response_str, "OK") != NULL)
        {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Find the +CEREG: response in the string
    const char *cereg_start = strstr(response_str, "+CEREG:");
    if (cereg_start == NULL)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Skip past "+CEREG: "
    cereg_start += 7;

    // Initialize parsed response
    memset(parsed_response, 0, sizeof(cereg_parsed_response_t));

    // Parse mode and status (always present)
    int mode = 0, status = 0;
    int matched = sscanf(cereg_start, "%d,%d", &mode, &status);
    if (matched < 2)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Validate mode and status
    if ((mode < 0) || (mode >= (int)CEREG_MODE_MAX) ||
        (status < 0) || (status >= (int)CEREG_STATUS_MAX))
    {
        return ESP_ERR_INVALID_STATE;
    }

    parsed_response->mode = (cereg_mode_t)mode;
    parsed_response->status = (cereg_status_t)status;

    // Look for optional location info
    if (strstr(cereg_start, ",\"") != NULL)
    {
        parsed_response->has_location_info = true;
        int act = 0;

        matched = sscanf(strstr(cereg_start, ",\""),
                         ",\"%4[^\"]\",,\"%4[^\"]\",%d",
                         parsed_response->tac,
                         parsed_response->ci,
                         &act);

        if (matched == 3)
        {
            if ((act < 0) || (act >= (int)CEREG_ACT_MAX))
            {
                return ESP_ERR_INVALID_STATE;
            }
            parsed_response->act = (cereg_act_t)act;
        }

        // Look for optional PSM info (only when mode is 4)
        if (parsed_response->mode == CEREG_MODE_ENABLE_URC_PSM &&
            strstr(response_str, ",[,[") != NULL)
        {
            parsed_response->has_psm_info = true;
            matched = sscanf(strstr(response_str, ",[,["),
                             ",[,[%8[^]],[%8[^]]]",
                             parsed_response->active_time,
                             parsed_response->periodic_tau);

            if (matched != 2)
            {
                return ESP_ERR_INVALID_RESPONSE;
            }
        }
    }

    return ESP_OK;
}

const char *cereg_mode_to_str(cereg_mode_t mode)
{
    static const char *const strings[] = {
        "Disable network registration URC",
        "Enable network registration URC",
        "Enable network registration and location URC",
        "Reserved",
        "Enable network registration, location and PSM URC"};

    if (mode >= CEREG_MODE_MAX)
    {
        return "Invalid Mode";
    }
    return strings[mode];
}

const char *cereg_status_to_str(cereg_status_t status)
{
    static const char *const strings[] = {
        "Not registered, not searching",
        "Registered, home network",
        "Not registered, searching",
        "Registration denied",
        "Unknown",
        "Registered, roaming"};

    if (status >= CEREG_STATUS_MAX)
    {
        return "Invalid Status";
    }
    return strings[status];
}

const char *cereg_act_to_str(cereg_act_t act)
{
    static const char *const strings[] = {
        "GSM",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "LTE M1",
        "Unknown",
        "LTE NB"};

    if (act >= CEREG_ACT_MAX)
    {
        return "Invalid Access Technology";
    }
    return strings[act];
}
