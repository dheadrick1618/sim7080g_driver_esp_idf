#pragma once

#include <esp_err.h>
#include <stdbool.h>

#define SIM7080G_UART_BAUD_RATE 115200
#define SIM87080G_UART_BUFF_SIZE 1024

/// @brief UART config struct defined by user of driver and passed to driver init
/// @note TX and RX here are in the perspective of the SIM7080G, and thus they are swapped in the perspecive of the ESP32
typedef struct
{
    int gpio_num_tx;
    int gpio_num_rx;
    int port_num; // This was chosen to be an INT because the esp-idf uart driver takes an int type
} sim7080g_uart_config_t;

esp_err_t sim7080g_uart_init(const sim7080g_uart_config_t sim7080g_uart_config);

/// @brief Test the UART connection by sending a command and checking for a response
// bool sim7080g_test_uart_loopback(sim7080g_handle_t *sim7080g_handle);