#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/uart.h>

#include "sim7080g_driver_esp_idf.h"

static const char *TAG = "SIM7080G Driver";

// Static Fxn Declarations:
static esp_err_t sim7080g_config_uart(const sim7080g_uart_config_t sim7080g_uart_config);

// ---------------------  EXTERNAL EXPOSED API FXNs  -------------------------//
esp_err_t sim7080g_init(sim7080g_handle_t *sim7080g_handle,
                        const sim7080g_uart_config_t sim7080g_uart_config,
                        const sim7080g_mqtt_config_t sim7080g_mqtt_config)
{
    esp_err_t err = sim7080g_config_uart(sim7080g_uart_config);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configuring UART: %s", esp_err_to_name(err));
        return err;
    }

    // config mqtt placeholder

    return ESP_OK;
}

static esp_err_t sim7080g_config_uart(const sim7080g_uart_config_t sim7080g_uart_config)
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
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error installing UART driver: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config((uart_port_t)sim7080g_uart_config.port_num, &uart_config);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configuring UART parameters: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin((uart_port_t)sim7080g_uart_config.port_num, sim7080g_uart_config.gpio_num_rx, sim7080g_uart_config.gpio_num_tx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting UART pins: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

// config mqtt placeholder
