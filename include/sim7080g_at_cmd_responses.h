// This file holds all fxns and structs relating to specifc AT commands that can be sent to the sim7080g
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

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

typedef struct
{
    csq_rssi_t rssi; // Received Signal Strength Indication
    csq_ber_t ber;   // Bit Error Rate
    int8_t rssi_dbm; // RSSI converted to dBm for easier interpretation
} csq_parsed_response_t;

esp_err_t parse_csq_response(const char *response_str, csq_parsed_response_t *parsed_response, at_cmd_type_t cmd_type);
const char *csq_rssi_to_str(csq_rssi_t rssi);
const char *csq_ber_to_str(csq_ber_t ber);
int8_t csq_rssi_to_dbm(csq_rssi_t rssi);

// --------------------- CEREG -------------------------//
// -----------------------------------------------------//
// typedef enum
// {
//     CEREG_STATUS_NOT_REGISTERED = 0,
//     CEREG_STATUS_REGISTERED_HOME = 1,
//     CEREG_STATUS_
// } cereg_status_t;

// typedef enum
// {
//     CEREG_MODE_DISABLE_URC = 0,         // Disable network registration URC
//     CEREG_MODE_ENABLE_URC = 1,          // Enable network registration URC
//     CEREG_MODE_ENABLE_URC_LOCATION = 2, // Enable network registration and location information URC
//     CEREG_MODE_ENABLE_URC_PSM = 4,      // Enable network registration, location and PSM information URC
//     CEREG_MODE_MAX
// } cereg_mode_t;

// // CEREG registration status
// typedef enum
// {
//     CEREG_STATUS_NOT_REGISTERED = 0,     // Not registered, MT is not currently searching
//     CEREG_STATUS_REGISTERED_HOME = 1,    // Registered, home network
//     CEREG_STATUS_SEARCHING = 2,          // Not registered, but MT is searching
//     CEREG_STATUS_DENIED = 3,             // Registration denied
//     CEREG_STATUS_UNKNOWN = 4,            // Unknown
//     CEREG_STATUS_REGISTERED_ROAMING = 5, // Registered, roaming
//     CEREG_STATUS_MAX
// } cereg_status_t;

// // CEREG access technology
// typedef enum
// {
//     CEREG_ACT_GSM = 0,    // GSM access technology
//     CEREG_ACT_LTE_M1 = 7, // LTE M1 A GB access technology
//     CEREG_ACT_LTE_NB = 9, // LTE NB S1 access technology
//     CEREG_ACT_MAX
// } cereg_act_t;

// // Structure to hold CEREG response data
// typedef struct
// {
//     cereg_mode_t mode;      // <n> parameter
//     cereg_status_t status;  // <stat> parameter
//     char tac[5];            // <tac> parameter (hex string)
//     char rac[3];            // <rac> parameter (hex string)
//     char ci[5];             // <ci> parameter (hex string)
//     cereg_act_t act;        // <AcT> parameter
//     char active_time[9];    // <Active-Time> parameter (bit string)
//     char periodic_tau[9];   // <Periodic-TAU> parameter (bit string)
//     bool has_location_info; // Indicates if location info is present
//     bool has_psm_info;      // Indicates if PSM info is present
// } cereg_response_t;

// const char *cereg_mode_to_str(cereg_mode_t mode);
// const char *cereg_status_to_str(cereg_status_t status);
// const char *cereg_act_to_str(cereg_act_t act);
// esp_err_t parse_cereg_response(const char *response_str, cereg_response_t *response);

/// ....
// Helper function declarations
// int16_t csq_rssi_to_dbm(csq_rssi_t rssi);