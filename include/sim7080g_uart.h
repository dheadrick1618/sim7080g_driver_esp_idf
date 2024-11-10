#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <driver/uart.h>
#include "sim7080g_types.h"

#define SIM7080G_UART_BAUD_RATE 115200
#define SIM87080G_UART_BUFF_SIZE 1024

esp_err_t sim7080g_uart_init(const sim7080g_uart_config_t sim7080g_uart_config);

/// @brief Test the UART connection by sending a command and checking for a response
// bool sim7080g_test_uart_loopback(sim7080g_handle_t *sim7080g_handle);