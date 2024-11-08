
#include <driver/uart.h>
#include <esp_log.h>
#include "sim7080g_uart.h"

static const char *TAG = "SIM7080G UART";

esp_err_t sim7080g_uart_init(const sim7080g_uart_config_t sim7080g_uart_config)
{
    uart_config_t uart_config = {
        .baud_rate = SIM7080G_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install((uart_port_t)sim7080g_uart_config.port_num, SIM87080G_UART_BUFF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error installing UART driver: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config((uart_port_t)sim7080g_uart_config.port_num, &uart_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configuring UART parameters: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin((uart_port_t)sim7080g_uart_config.port_num, sim7080g_uart_config.gpio_num_rx, sim7080g_uart_config.gpio_num_tx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting UART pins: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

// ---------------------  INTERNAL TESTING FXNs  ---------------------//

// bool sim7080g_test_uart_loopback(sim7080g_handle_t *sim7080g_handle)
// {
//     if (!sim7080g_handle->uart_initialized)
//     {
//         ESP_LOGE(TAG, "SIM7080G driver not initialized");
//         return false;
//     }

//     const char *test_str = "Hello, SIM7080G!";
//     char rx_buffer[128];

//     // Send data
//     int tx_bytes = uart_write_bytes(sim7080g_handle->uart_config.port_num, test_str, strlen(test_str));
//     ESP_LOGI(TAG, "Sent %d bytes: %s", tx_bytes, test_str);

//     // Read data
//     int len = uart_read_bytes(sim7080g_handle->uart_config.port_num, rx_buffer, sizeof(rx_buffer) - 1, 1000);

//     if (len < 0)
//     {
//         ESP_LOGE(TAG, "UART read error");
//         return false;
//     }

//     // Add null terminator to the byte data read to handle it like a string
//     rx_buffer[len] = '\0';

//     ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

//     // Compare sent and received data
//     if (strcmp(test_str, rx_buffer) == 0)
//     {
//         ESP_LOGI(TAG, "UART loopback test passed!");
//         return true;
//     }
//     else
//     {
//         ESP_LOGE(TAG, "UART loopback test failed!");
//         return false;
//     }
// }