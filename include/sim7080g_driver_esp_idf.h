/**
 * THESE are the fxns the user will call to interact with the SIM7080G - this should be the only header they need to include to use the driver
 */
#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include "sim7080g_types.h"

/// @brief Creates a device handle that stores the provided configurations
/// @note This must be called before a device can be init
esp_err_t sim7080g_config(sim7080g_handle_t *sim7080g_handle,
                          const sim7080g_uart_config_t sim7080g_uart_config,
                          const sim7080g_mqtt_config_t sim7080g_mqtt_config);

/// @brief Use configured UART and MQTT settings to initialize drivers for this device
/// @note This can only be called after a handle is configured
esp_err_t sim7080g_init(sim7080g_handle_t *sim7080g_handle);
// esp_err_t sim7080g_deinit(sim7080g_handle_t *sim7080g_handle);

// Connect to network(follows sim com example command sequence)
/// @brief Send series of AT commands to set the device various network settings to connect to LTE network bearer
esp_err_t sim7080g_connect_to_network_bearer(const sim7080g_handle_t *sim7080g_handle, const char *apn);

// Connect to MQTT broker
esp_err_t sim7080g_mqtt_connect(const sim7080g_handle_t *sim7080g_handle);

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
esp_err_t sim7080g_is_physical_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected_out);
// esp_err_t sim7080g_is_data_link_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
// esp_err_t sim7080g_is_network_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
// //// esp_err_t sim7080g_is_transport_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);
// // TODO - Implement session layer check once SSL is implemented
// esp_err_t sim7080g_is_application_layer_connected(const sim7080g_handle_t *sim7080g_handle, bool *connected);

esp_err_t sim7080g_get_device_status(sim7080g_handle_t *handle, device_status_t *status);
esp_err_t sim7080g_publish_device_status(sim7080g_handle_t *handle, const char *base_topic);
