#pragma once

#include <esp_err.h>
#include <stdbool.h>

#include "sim7080g_uart.h"

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

typedef struct
{
    sim7080g_uart_config_t uart_config;
    sim7080g_mqtt_config_t mqtt_config;
    bool uart_initialized;
    bool mqtt_initialized;
} sim7080g_handle_t;

/// @brief Creates a device handle that stores the provided configurations
/// @note This must be called before a device can be init
esp_err_t sim7080g_config(sim7080g_handle_t *sim7080g_handle,
                          const sim7080g_uart_config_t sim7080g_uart_config,
                          const sim7080g_mqtt_config_t sim7080g_mqtt_config);

/// @brief Use configured UART and MQTT settings to initialize drivers for this device
/// @note This can only be called after a handle is configured
esp_err_t sim7080g_init(sim7080g_handle_t *sim7080g_handle);
// esp_err_t sim7080g_deinit(sim7080g_handle_t *sim7080g_handle);

// // Connect to network (follows sim com example command sequence)
// // /// @brief Send series of AT commands to set the device various network settings to connect to LTE network bearer
// esp_err_t sim7080g_connect_to_network_bearer(const sim7080g_handle_t *sim7080g_handle, const char *apn);

// // Connect to MQTT broker
// esp_err_t sim7080g_mqtt_connect_to_broker(const sim7080g_handle_t *sim7080g_handle);

// // Disconnect from MQTT broker
// esp_err_t sim7080g_mqtt_disconnect_from_broker(const sim7080g_handle_t *sim7080g_handle);

// // Publish to MQTT broker
// esp_err_t sim7080g_mqtt_publish(const sim7080g_handle_t *sim7080g_handle,
//                                 const char *topic,
//                                 const char *message,
//                                 uint8_t qos,
//                                 bool retain);

// // Subscribe to MQTT broker
// //// esp_err_t sim7080g_mqtt_subscribe()

// // ---------- FXNS to check connection status of device at various layers of the stack --------- //
// esp_err_t sim7080g_is_physical_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
// esp_err_t sim7080g_is_data_link_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
// esp_err_t sim7080g_is_network_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
// //// esp_err_t sim7080g_is_transport_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
// // TODO - Implement session layer check once SSL is implemented
// esp_err_t sim7080g_is_application_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
