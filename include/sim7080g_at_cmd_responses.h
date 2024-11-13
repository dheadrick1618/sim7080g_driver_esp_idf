// This file holds all fxns and structs relating to specifc AT commands that can be sent to the sim7080g
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <sim7080g_types.h>

// ------------------- TEST (AT) ----------------------//
// ----------------------------------------------------//
typedef enum
{
    TEST_STATUS_OK = 1,
    TEST_STATUS_ERROR = 2,
    TEST_STATUS_MAX = 3
} at_test_status_t;

// Structure to hold response data
typedef struct
{
    at_test_status_t status;
} at_test_parsed_response_t;

esp_err_t parse_at_test_response(const char *response_str, at_test_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *at_test_status_to_str(at_test_status_t status);

// --------------------- CPIN -------------------------//
// -----------------------------------------------------//
typedef enum
{
    CPIN_STATUS_READY = 0,      // MT is not pending for any password
    CPIN_STATUS_SIM_PIN = 1,    // MT is waiting for SIM PIN
    CPIN_STATUS_SIM_PUK = 2,    // MT is waiting for SIM PUK
    CPIN_STATUS_PH_SIM_PIN = 3, // ME is waiting for phone to SIM card (antitheft)
    CPIN_STATUS_PH_SIM_PUK = 4, // ME is waiting for SIM PUK (antitheft)
    CPIN_STATUS_PH_NET_PIN = 5, // ME is waiting network personalization password
    CPIN_STATUS_SIM_PIN2 = 6,   // PIN2 required (for FDN)
    CPIN_STATUS_SIM_PUK2 = 7,   // PUK2 required
    CPIN_STATUS_UNKNOWN = 8,    // Unknown status
    CPIN_STATUS_ERROR = 9,
    CPIN_STATUS_MAX = 10
} cpin_status_t;

// Structure to hold CPIN response data
typedef struct
{
    cpin_status_t status;  // Current PIN status
    bool requires_new_pin; // Indicates if a new PIN is required (for PUK operations)
} cpin_parsed_response_t;

esp_err_t parse_cpin_response(const char *response_str, cpin_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cpin_status_to_str(cpin_status_t status);
cpin_status_t cpin_str_to_status(const char *status_str);

// ----------------------- CFUN ------------------------//
// ----------------------------------------------------//

typedef enum
{
    CFUN_FUNCTIONALITY_MIN = 0,
    CFUN_FUNCTIONALITY_FULL = 1,
    CFUN_FUNCTIONALITY_DISABLE_RF_TX_AND_RX = 4,
    CFUN_FUNCTIONALITY_FACTORY_TEST_MODE = 5,
    CFUN_FUNCTIONALITY_RESET = 6,
    CFUN_FUNCTIONALITY_OFFLINE_MODE = 7,
    CFUN_FUNCTIONALITY_MAX = 8
} cfun_functionality_t;

typedef struct
{
    cfun_functionality_t functionality;
} cfun_parsed_response_t;

esp_err_t parse_cfun_response(const char *response_str, cfun_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cfun_functionality_to_str(cfun_functionality_t functionality);

// --------------------- CSQ -------------------------//
// ----------------------------------------------------//

typedef enum
{
    CSQ_RSSI_NOT_DETECTABLE = 99,
    CSQ_RSSI_MAX = 32 // Values 0-31 represent actual signal levels
} csq_rssi_t;

typedef enum
{
    CSQ_BER_NOT_DETECTABLE = 99,
    CSQ_BER_MAX = 8 // Values 0-7 represent actual BER levels
} csq_ber_t;

// typedef enum
// {
//     SIGNAL_STRENGTH_NONE = 0,
//     SIGNAL_STRENGTH_POOR = 1,
//     SIGNAL_STRENGTH_FAIR = 2,
//     SIGNAL_STRENGTH_GOOD = 3,
//     SIGNAL_STRENGTH_EXCELLENT = 4,
//     SIGNAL_STRENGTH_MAX = 5
// } signal_strength_category_t;

typedef struct
{
    csq_rssi_t rssi;                     // Received Signal Strength Indication
    csq_ber_t ber;                       // Bit Error Rate
    int8_t rssi_dbm;                     // RSSI converted to dBm for easier interpretation
    signal_strength_category_t category; // Signal strength category
} csq_parsed_response_t;

esp_err_t parse_csq_response(const char *response_str, csq_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *csq_rssi_to_str(csq_rssi_t rssi);
const char *csq_ber_to_str(csq_ber_t ber);
int8_t csq_rssi_to_dbm(csq_rssi_t rssi);
signal_strength_category_t csq_rssi_dbm_to_strength_category(csq_rssi_t rssi);
const char *signal_strength_category_to_str(signal_strength_category_t category);

// --------------------- ATE -------------------------//
// ----------------------------------------------------//
typedef enum
{
    ATE_MODE_OFF = 0,
    ATE_MODE_ON = 1,
    ATE_MODE_MAX = 2
} ate_mode_t;

typedef struct
{
    ate_mode_t mode;
} ate_parsed_response_t;

esp_err_t parse_ate_response(const char *response_str, ate_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *ate_mode_to_str(ate_mode_t mode);

// --------------------- CMEE -------------------------//
// ----------------------------------------------------//

typedef enum
{
    CMEE_MODE_DISABLE = 0,
    CMEE_MODE_NUMERIC = 1,
    CMEE_MODE_VERBOSE = 2,
    CMEE_MODE_MAX = 3
} cmee_mode_t;

typedef struct
{
    cmee_mode_t mode;
} cmee_parsed_response_t;

esp_err_t parse_cmee_response(const char *response_str, cmee_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cmee_mode_to_str(cmee_mode_t mode);

// --------------------- CGDCONT -------------------------//
// ----------------------------------------------------//

typedef enum
{
    CGDCONT_PDP_TYPE_IP = 0,
    CGDCONT_PDP_TYPE_PPP = 1,
    CGDCONT_PDP_TYPE_IPV6 = 2,
    CGDCONT_PDP_TYPE_IPV4V6 = 3,
    CGDCONT_PDP_TYPE_NONIP = 4,
    CGDCONT_PDP_TYPE_MAX = 5
} cgdcont_pdp_type_t;

#define CGDCONT_MIN_CID 1
#define CGDCONT_MAX_CID 15
#define CGDCONT_APN_MAX_LEN 100

typedef struct
{
    uint8_t cid;                   // PDP Context Identifier (1-15)
    cgdcont_pdp_type_t pdp_type;   // Type of packet data protocol
    char apn[CGDCONT_APN_MAX_LEN]; // Access Point Name
} cgdcont_config_t;

typedef struct
{
    cgdcont_config_t contexts[CGDCONT_MAX_CID]; // Array to hold all PDP contexts
    uint8_t num_contexts;                       // Number of defined contexts
} cgdcont_parsed_response_t;

esp_err_t parse_cgdcont_response(const char *response_str, cgdcont_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cgdcont_pdp_type_to_str(cgdcont_pdp_type_t pdp_type);
cgdcont_pdp_type_t cgdcont_str_to_pdp_type(const char *pdp_type_str);

// --------------------- CGATT -------------------------//
// ----------------------------------------------------//

typedef enum
{
    CGATT_STATE_DETACHED = 0,
    CGATT_STATE_ATTACHED = 1,
    CGATT_STATE_MAX = 2
} cgatt_state_t;

typedef struct
{
    cgatt_state_t state;
} cgatt_parsed_response_t;

esp_err_t parse_cgatt_response(const char *response_str, cgatt_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cgatt_state_to_str(cgatt_state_t state);

// --------------------- COPS -------------------------//
// ----------------------------------------------------//
typedef enum
{
    COPS_STATUS_UNKNOWN = 0,
    COPS_STATUS_AVAILABLE = 1,
    COPS_STATUS_CURRENT = 2,
    COPS_STATUS_FORBIDDEN = 3,
    COPS_STATUS_MAX = 4
} cops_operator_status_t;

typedef enum
{
    COPS_FORMAT_LONG = 0,    // Long format alphanumeric
    COPS_FORMAT_SHORT = 1,   // Short format alphanumeric
    COPS_FORMAT_NUMERIC = 2, // Numeric format
    COPS_FORMAT_MAX = 3
} cops_format_t;

typedef enum
{
    COPS_MODE_AUTO = 0,        // Automatic operator selection
    COPS_MODE_MANUAL = 1,      // Manual operator selection
    COPS_MODE_DEREGISTER = 2,  // Manual deregister from network
    COPS_MODE_SET_FORMAT = 3,  // Set format only
    COPS_MODE_MANUAL_AUTO = 4, // Manual/Automatic selection
    COPS_MODE_MAX = 5
} cops_mode_t;

typedef enum
{
    COPS_ACT_GSM = 0,         // GSM access technology
    COPS_ACT_GSM_COMPACT = 1, // GSM compact
    COPS_ACT_GSM_EGPRS = 3,   // GSM EGPRS
    COPS_ACT_LTE_M1 = 7,      // LTE M1
    COPS_ACT_LTE_NB = 9,      // LTE NB
    COPS_ACT_MAX = 10
} cops_access_tech_t;

#define COPS_OPER_NAME_MAX_LEN 32

typedef struct
{
    cops_mode_t mode;
    cops_format_t format;
    char operator_name[COPS_OPER_NAME_MAX_LEN];
    cops_access_tech_t access_tech;
} cops_parsed_response_t;

esp_err_t parse_cops_response(const char *response_str, cops_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cops_mode_to_str(cops_mode_t mode);
const char *cops_format_to_str(cops_format_t format);
const char *cops_access_tech_to_str(cops_access_tech_t tech);

// --------------------- CGNAPN -------------------------//
// ----------------------------------------------------//

typedef enum
{
    CGNAPN_APN_NOT_PROVIDED = 0, // Network did not send APN
    CGNAPN_APN_PROVIDED = 1,     // Network sent APN
    CGNAPN_STATUS_MAX = 2
} cgnapn_status_t;

#define CGNAPN_MAX_APN_LEN 120 // As per test command response

typedef struct
{
    cgnapn_status_t status;
    char network_apn[CGNAPN_MAX_APN_LEN + 1]; // +1 for null terminator
} cgnapn_parsed_response_t;

esp_err_t parse_cgnapn_response(const char *response_str, cgnapn_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cgnapn_status_to_str(cgnapn_status_t status);

// --------------------- CNCFG -------------------------//
// ----------------------------------------------------//
#define CNCFG_MAX_APN_LEN 150
#define CNCFG_MAX_USERNAME_LEN 127
#define CNCFG_MAX_PASSWORD_LEN 127
#define CNCFG_MAX_CONTEXTS 4

typedef enum
{
    CNCFG_IP_TYPE_DUAL = 0,     // Dual PDN Stack
    CNCFG_IP_TYPE_IPV4 = 1,     // IPv4
    CNCFG_IP_TYPE_IPV6 = 2,     // IPv6
    CNCFG_IP_TYPE_NONIP = 3,    // Non-IP
    CNCFG_IP_TYPE_EX_NONIP = 4, // Extended Non-IP
    CNCFG_IP_TYPE_MAX = 5
} cncfg_ip_type_t;

typedef enum
{
    CNCFG_AUTH_NONE = 0,     // No authentication
    CNCFG_AUTH_PAP = 1,      // PAP authentication
    CNCFG_AUTH_CHAP = 2,     // CHAP authentication
    CNCFG_AUTH_PAP_CHAP = 3, // PAP or CHAP authentication
    CNCFG_AUTH_MAX = 4
} cncfg_auth_type_t;

typedef struct
{
    uint8_t pdp_idx;
    cncfg_ip_type_t ip_type;
    char apn[CNCFG_MAX_APN_LEN + 1];
    char username[CNCFG_MAX_USERNAME_LEN + 1];
    char password[CNCFG_MAX_PASSWORD_LEN + 1];
    cncfg_auth_type_t auth_type;
} cncfg_context_config_t;

typedef struct
{
    cncfg_context_config_t contexts[CNCFG_MAX_CONTEXTS];
    uint8_t num_contexts;
} cncfg_parsed_response_t;

esp_err_t parse_cncfg_response(const char *response_str, cncfg_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cncfg_ip_type_to_str(cncfg_ip_type_t ip_type);
const char *cncfg_auth_type_to_str(cncfg_auth_type_t auth_type);

// --------------------- CNACT -------------------------//
// ----------------------------------------------------//

typedef enum
{
    CNACT_ACTION_DEACTIVATE = 0,
    CNACT_ACTION_ACTIVATE = 1,
    CNACT_ACTION_AUTO_ACTIVATE = 2,
    CNACT_ACTION_MAX = 3
} cnact_action_t;

typedef enum
{
    CNACT_STATUS_DEACTIVATED = 0,
    CNACT_STATUS_ACTIVATED = 1,
    CNACT_STATUS_IN_OPERATION = 2,
    CNACT_STATUS_MAX = 3
} cnact_status_t;

#define CNACT_MAX_CONTEXTS 4
#define CNACT_IP_ADDR_MAX_LEN 16 // For IPv4 address string

typedef struct
{
    uint8_t pdp_idx;
    cnact_status_t status;
    char ip_address[CNACT_IP_ADDR_MAX_LEN];
} cnact_context_info_t;

typedef struct
{
    cnact_context_info_t contexts[CNACT_MAX_CONTEXTS];
    uint8_t num_contexts;
} cnact_parsed_response_t;

esp_err_t parse_cnact_response(const char *response_str, cnact_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cnact_action_to_str(cnact_action_t action);
const char *cnact_status_to_str(cnact_status_t status);

// --------------------- SMCONF -------------------------//
// ----------------------------------------------------//

typedef enum
{
    SMCONF_QOS_AT_MOST_ONCE = 0,
    SMCONF_QOS_AT_LEAST_ONCE = 1,
    SMCONF_QOS_EXACTLY_ONCE = 2,
    SMCONF_QOS_MAX = 3
} smconf_qos_t;

typedef enum
{
    SMCONF_PARAM_CLIENTID,
    SMCONF_PARAM_URL,
    SMCONF_PARAM_KEEPTIME,
    SMCONF_PARAM_USERNAME,
    SMCONF_PARAM_PASSWORD,
    SMCONF_PARAM_CLEANSS,
    SMCONF_PARAM_QOS,
    SMCONF_PARAM_TOPIC,
    SMCONF_PARAM_MESSAGE,
    SMCONF_PARAM_RETAIN,
    SMCONF_PARAM_SUBHEX,
    SMCONF_PARAM_ASYNCMODE,
    SMCONF_PARAM_MAX
} smconf_param_t;

#define SMCONF_URL_MAX_LEN (MQTT_BROKER_URL_MAX_CHARS)
#define SMCONF_USERNAME_MAX_LEN (MQTT_BROKER_USERNAME_MAX_CHARS)
#define SMCONF_CLIENTID_MAX_LEN (MQTT_BROKER_CLIENT_ID_MAX_CHARS)
#define SMCONF_PASSWORD_MAX_LEN (MQTT_BROKER_PASSWORD_MAX_CHARS)
#define SMCONF_MESSAGE_MAX_LEN (MQTT_PACKET_MAX_DATA_CHARS)
#define SMCONF_TOPIC_MAX_LEN (MQTT_PACKET_MAX_TOPIC_CHARS)
#define SMCONF_DATA_MAX_LEN (MQTT_PACKET_MAX_DATA_CHARS)
#define SMCONF_PORT_MAX 65535

typedef struct
{
    char client_id[SMCONF_CLIENTID_MAX_LEN + 1];
    char url[SMCONF_URL_MAX_LEN + 1];
    uint16_t port;
    uint16_t keeptime;
    char username[SMCONF_USERNAME_MAX_LEN + 1];
    char password[SMCONF_PASSWORD_MAX_LEN + 1];
    bool clean_session;
    smconf_qos_t qos;
    char topic[SMCONF_TOPIC_MAX_LEN + 1];
    char message[SMCONF_DATA_MAX_LEN + 1];
    bool retain;
    bool subhex;
    bool async_mode;
} smconf_config_t;

typedef struct
{
    smconf_config_t config;
} smconf_parsed_response_t;

esp_err_t parse_smconf_response(const char *response_str, smconf_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *smconf_param_to_str(smconf_param_t param);
const char *smconf_qos_to_str(smconf_qos_t qos);

// --------------------- SMCONN -------------------------//
// ----------------------------------------------------//
typedef enum
{
    SMCONN_STATUS_SUCCESS = 0,
    SMCONN_STATUS_ERROR,
    SMCONN_STATUS_MAX
} smconn_status_t;

typedef struct
{
    smconn_status_t status;
} smconn_parsed_response_t;

// Add function declarations to sim7080g_at_cmd_responses.h
esp_err_t parse_smconn_response(const char *response_str, smconn_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);

// --------------------- SMPUB -------------------------//
// ----------------------------------------------------//

typedef enum
{
    SMPUB_STATUS_SUCCESS = 0,
    SMPUB_STATUS_ERROR,
    SMPUB_STATUS_TIMEOUT,
    SMPUB_STATUS_NOT_CONNECTED,
    SMPUB_STATUS_MAX
} smpub_status_t;

#define MQTT_MAX_TOPIC_LEN 128U
#define MQTT_MAX_MESSAGE_LEN 1024U

typedef struct
{
    smpub_status_t status;
    char response_topic[MQTT_MAX_TOPIC_LEN];
    char response_message[MQTT_MAX_MESSAGE_LEN];
} smpub_parsed_response_t;

esp_err_t parse_smpub_response(const char *response_str, smpub_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);

// --------------------- SMSTATE -------------------------//
// ----------------------------------------------------//

typedef enum
{
    SMSTATE_STATUS_DISCONNECTED = 0,
    SMSTATE_STATUS_CONNECTED = 1,
    SMSTATE_STATUS_CONNECTED_WITH_SESSION = 2,
    SMSTATE_STATUS_MAX = 3
} smstate_status_t;

typedef struct
{
    smstate_status_t status;
} smstate_parsed_response_t;

esp_err_t parse_smstate_response(const char *response_str, smstate_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *smstate_status_to_str(smstate_status_t status);

// --------------------- CEREG -------------------------//
// -----------------------------------------------------//
typedef enum
{
    CEREG_MODE_DISABLE_URC = 0,         // Disable network registration URC
    CEREG_MODE_ENABLE_URC = 1,          // Enable network registration URC
    CEREG_MODE_ENABLE_URC_LOCATION = 2, // Enable network registration and location information URC
    CEREG_MODE_ENABLE_URC_PSM = 4,      // Enable network registration, location and PSM information URC
    CEREG_MODE_MAX = 5
} cereg_mode_t;

typedef enum
{
    CEREG_STATUS_NOT_REGISTERED = 0,     // Not registered, MT is not currently searching
    CEREG_STATUS_REGISTERED_HOME = 1,    // Registered, home network
    CEREG_STATUS_SEARCHING = 2,          // Not registered, but MT is searching
    CEREG_STATUS_DENIED = 3,             // Registration denied
    CEREG_STATUS_UNKNOWN = 4,            // Unknown
    CEREG_STATUS_REGISTERED_ROAMING = 5, // Registered, roaming
    CEREG_STATUS_MAX = 6
} cereg_status_t;

// typedef enum
// {
//     CEREG_ACT_GSM = 0,    // GSM access technology
//     CEREG_ACT_LTE_M1 = 7, // LTE M1 A GB access technology
//     CEREG_ACT_LTE_NB = 9, // LTE NB S1 access technology
//     CEREG_ACT_MAX = 10
// } cereg_act_t;

// Structure to hold CEREG response data
typedef struct
{
    cereg_mode_t mode;      // Current URC mode setting
    cereg_status_t status;  // Registration status
    bool has_location_info; // Indicates if location info is present
    char tac[5];            // Tracking Area Code (hex string)
    char ci[5];             // Cell ID (hex string)
    cereg_act_t act;        // Access Technology
    bool has_psm_info;      // Indicates if PSM info is present
    char active_time[9];    // Active time value
    char periodic_tau[9];   // Periodic TAU value
} cereg_parsed_response_t;

esp_err_t parse_cereg_response(const char *response_str, cereg_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *cereg_mode_to_str(cereg_mode_t mode);
const char *cereg_status_to_str(cereg_status_t status);
const char *cereg_act_to_str(cereg_act_t act);

/// ....
// Helper function declarations
// int16_t csq_rssi_to_dbm(csq_rssi_t rssi);