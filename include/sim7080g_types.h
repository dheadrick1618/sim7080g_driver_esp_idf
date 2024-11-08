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
#define MQTT_BROKER_URL_MAX_CHARS 128
#define MQTT_BROKER_USERNAME_MAX_CHARS 32
#define MQTT_BROKER_CLIENT_ID_MAX_CHARS 32
#define MQTT_BROKER_PASSWORD_MAX_CHARS 32

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

// Core handle structure
typedef struct sim7080g_handle_t
{
    sim7080g_uart_config_t uart_config;
    sim7080g_mqtt_config_t mqtt_config;
    bool uart_initialized;
    bool mqtt_initialized;
    // ... other common fields
} sim7080g_handle_t;