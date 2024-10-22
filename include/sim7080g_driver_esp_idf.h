#pragma once

#include <esp_err.h>
#include <stdbool.h>

#define SIM7080G_UART_BAUD_RATE 115200
#define SIM87080G_UART_BUFF_SIZE 1024

// TODO - Check these values against  MQTT v3 protocol specs
#define MQTT_BROKER_URL_MAX_CHARS 128
#define MQTT_BROKER_USERNAME_MAX_CHARS 32
#define MQTT_BROKER_CLIENT_ID_MAX_CHARS 32
#define MQTT_BROKER_PASSWORD_MAX_CHARS 32

/// @brief UART config struct defined by user of driver and passed to driver init
/// @note TX and RX here are in the perspective of the SIM7080G, and thus they are swapped in the perspecive of the ESP32
typedef struct
{
    int gpio_num_tx;
    int gpio_num_rx;
    int port_num; // This was chosen to be an INT because the esp-idf uart driver takes an int type
} sim7080g_uart_config_t;

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

typedef struct
{
    sim7080g_uart_config_t uart_config;
    sim7080g_mqtt_config_t mqtt_config;
    bool initialized;
} sim7080g_handle_t;

/// @brief Creates a device handle with the provided configuration
/// @note This must be called before a device handle can be used
/// @param sim7080g_handle
/// @param sim7080g_uart_config
/// @param sim7080g_mqtt_config
/// @return
esp_err_t sim7080g_init(sim7080g_handle_t *sim7080g_handle,
                        const sim7080g_uart_config_t sim7080g_uart_config,
                        const sim7080g_mqtt_config_t sim7080g_mqtt_config);

esp_err_t sim7080g_deinit(sim7080g_handle_t *sim7080g_handle);

