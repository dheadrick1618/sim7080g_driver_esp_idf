// sim7080g_types.h - Common types and structures
// THIS FILE CONTAINS BASIC TYPES USED ACROSS MULTIPLE FILES
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/// @brief UART config struct defined by user of driver and passed to driver init
/// @note TX and RX here are in the perspective of the SIM7080G, and thus they are swapped in the perspecive of the ESP32
typedef struct
{
    int gpio_num_tx;
    int gpio_num_rx;
    int port_num; // This was chosen to be an INT because the esp-idf uart driver takes an int type
} sim7080g_uart_config_t;

// TODO - Check these values against  MQTT v3 protocol specs
#define MQTT_BROKER_URL_MAX_CHARS 128U
#define MQTT_BROKER_USERNAME_MAX_CHARS 128U
#define MQTT_BROKER_CLIENT_ID_MAX_CHARS 128U
#define MQTT_BROKER_PASSWORD_MAX_CHARS 128U
#define MQTT_PACKET_MAX_MESSAGE_CHARS 1024U // A message consists of the topic AND the data
#define MQTT_PACKET_MAX_TOPIC_CHARS 128U    // Max topic length
#define MQTT_PACKET_MAX_DATA_CHARS 896U     // Max data length

/// @brief For config device with MQTT broker.
/// @note Thinger refers to the client password as the 'device credentials'
typedef struct
{
    char broker_url[MQTT_BROKER_URL_MAX_CHARS];
    char username[MQTT_BROKER_USERNAME_MAX_CHARS];
    char client_id[MQTT_BROKER_CLIENT_ID_MAX_CHARS];
    char client_password[MQTT_BROKER_PASSWORD_MAX_CHARS];
    uint16_t port;
} sim7080g_mqtt_config_t;

/**
 * @brief MQTT broker connection status values
 */
typedef enum
{
    MQTT_STATUS_DISCONNECTED = 0,      // Not connected to broker
    MQTT_STATUS_CONNECTED = 1,         // Connected to broker
    MQTT_STATUS_CONNECTED_SESSION = 2, // Connected with session present
    MQTT_STATUS_MAX = 3
} sim7080g_mqtt_connection_status_t;

typedef enum
{
    MQTT_ERR_NONE = 0,
    MQTT_ERR_NETWORK = 1,     // Network error
    MQTT_ERR_PROTOCOL = 2,    // Protocol error
    MQTT_ERR_UNAVAILABLE = 3, // Server unavailable
    MQTT_ERR_TIMEOUT = 4,     // Connection timeout
    MQTT_ERR_REJECTED = 5,    // Connection rejected
    MQTT_ERR_UNKNOWN = 99     // Unknown error
} mqtt_error_code_t;

/**
 * @brief Structure to hold all MQTT parameters
 * @note  DO NOT INCLUDE the 'mqtt://' protocol prefix OR port number in the URL
 * @note  NOTE used for config as of now (just for runtime value checking and logging) - the user only sets the config struct
 */
typedef struct
{
    char broker_url[MQTT_BROKER_URL_MAX_CHARS]; // NOTE: DO NOT INCLUDE the 'mqtt://' protocol prefix OR port num in the URL
    uint16_t port;
    char client_id[MQTT_BROKER_CLIENT_ID_MAX_CHARS];
    char username[MQTT_BROKER_USERNAME_MAX_CHARS];
    char client_password[MQTT_BROKER_PASSWORD_MAX_CHARS];
    uint16_t keepalive;
    bool clean_session;
    uint8_t qos;
    bool retain;
    bool sub_hex;
    bool async_mode;
} mqtt_parameters_t;

// Core handle structure used by all user exposed API fxns
typedef struct sim7080g_handle_t
{
    sim7080g_uart_config_t uart_config;
    sim7080g_mqtt_config_t mqtt_config;
    bool uart_initialized;
    bool mqtt_initialized;
} sim7080g_handle_t;

// --------------------- COMMON AT Command Definitions -------------------------//

// Base response structure all AT command responses inherit from
typedef struct
{
    bool success;             // Indicates if command succeeded
    const char *raw_response; // Original response string
    esp_err_t error_code;     // Specific error code if any
} at_response_base_t;

typedef enum
{
    AT_CMD_TYPE_TEST = 0U,
    AT_CMD_TYPE_READ,
    AT_CMD_TYPE_WRITE,
    AT_CMD_TYPE_EXECUTE,
} at_cmd_type_t;

typedef struct
{
    const char *cmd_string;
    const char *response_format;
} at_cmd_info_t;

/// @brief  FULL definition of an AT command
typedef struct
{
    const char *name;
    const char *description;
    at_cmd_info_t test;
    at_cmd_info_t read;
    at_cmd_info_t write;
    at_cmd_info_t execute;
} at_cmd_t;

// TODO - DEAL WITH DEPENDENCY LOOP AND SOME DEFINES IN THE 'AT CMD RESPONSE' FILE THAT MAYBE SHOULD BE HERE

typedef enum
{
    SIGNAL_STRENGTH_NONE = 0,
    SIGNAL_STRENGTH_POOR = 1,
    SIGNAL_STRENGTH_FAIR = 2,
    SIGNAL_STRENGTH_GOOD = 3,
    SIGNAL_STRENGTH_EXCELLENT = 4,
    SIGNAL_STRENGTH_MAX = 5
} signal_strength_category_t;

typedef enum
{
    CEREG_ACT_GSM = 0,    // GSM access technology
    CEREG_ACT_LTE_M1 = 7, // LTE M1 A GB access technology
    CEREG_ACT_LTE_NB = 9, // LTE NB S1 access technology
    CEREG_ACT_MAX = 10
} cereg_act_t;

typedef struct
{
    struct
    {
        int8_t rssi_dbm;                     // Signal strength in dBm
        signal_strength_category_t category; // Signal quality category
        uint8_t ber;                         // Bit error rate
    } signal_info;

    struct
    {
        bool is_registered;      // Network registration status
        bool is_roaming;         // Whether device is roaming
        char operator_name[32];  // Network operator name
        cereg_act_t access_tech; // Access technology (LTE-M, NB-IoT, etc)
        char tac[5];             // Tracking Area Code
        char cell_id[5];         // Cell ID
    } network_info;

    struct
    {
        bool pdp_active;     // Whether PDP context is active
        char ip_address[16]; // Assigned IP address
        char apn[100];       // Access Point Name being used
    } connection_info;

    // Add timestamp or uptime if available
    uint32_t uptime_seconds; // Device uptime in seconds
} device_status_t;