# SIM7080G Driver for ESP32 using ESP IDF

This is a driver for the SIM7080G cellular transceiver manufactured by SIMcom.

This code is intended for use as your project as a library or ESP-IDF 'component'. You can ``` git clone ``` the project directly into your component directory to get started.

For now only UART is used for communicating with the device, and only commands relating to the use of MQTT are defined.

AT command were created in reference of the SIM7070_SIM7080_SIM7090 Series_AT Command Manual_V1.04 provided by SIMcom [here](https://simcom.ee/documents/SIM7000x/SIM7000%20Series_AT%20Command%20Manual_V1.04.pdf)

## Usage

If using this code as a component ensure the driver is included as a requirement in your main CMakeLists.txt file.

```@CMake
idf_component_register(SRCS "sim7080g_driver_dev_project.c"
                    INCLUDE_DIRS "."
                    REQUIRES sim7080g_driver_esp_idf)
```

1. First create a handle for the driver.
2. Set config structs used by driver
3. Call the driver  'config' fxn - passing it these structs.
4. Call the driver 'init' fxn.
5. Call the driver 'connect to network bearer' fxn.
6. Call the driver 'mqtt connect to broker' fxn.
7. Publish data to MQTT topic as desired.

See example usage code below:

```@C
#include <stdio.h>
#include <esp_err.h>
#include "sim7080g_driver_esp_idf.h"

void app_main(void)
{
    sim7080g_uart_config_t sim7080g_uart_config = {
        .gpio_num_rx = 32U,
        .gpio_num_tx = 33U,
        .port_num = 1};

    sim7080g_mqtt_config_t sim7080g_mqtt_config = {
        .broker_url = "mqtt_broker_url_here",
        .username = "broker_account_username",
        .client_id = "id_of_client_we_are_connecting_as",
        .client_password = "password_of_client_we_are_connecting_as",
        .port = port_num_here,
    };

    sim7080g_handle_t sim7080g;

    esp_err_t err = sim7080g_config(&sim7080g, sim7080g_uart_config, sim7080g_mqtt_config);
    if (err != ESP_OK)
    {
        printf("Error configuring SIM7080G driver\n");
        return;
    }
    else
    {
        printf("SIM7080G driver configured. Ready for init\n");
    }

    err = sim7080g_init(&sim7080g);
    if (err != ESP_OK)
    {
        printf("Error initializing SIM7080G driver\n");
        return;
    }
    else
    {
        printf("SIM7080G driver initialized\n");
    }

    const char *apn = "apn_here"; // Depends on the SIM card and carrier
    sim7080g_connect_to_network_bearer(&sim7080g, apn);
    if(err == ESP_OK)
    {
        printf("Network bearer connected successfully\n");
    }
    else
    {
        printf("Error connecting to network bearer\n");
    }

    err = sim7080g_mqtt_connect_to_broker(&sim7080g);
    if (err == ESP_OK)
    {
        printf("Connected to MQTT broker\n");
    }
    else
    {
        printf("Error connecting to MQTT broker\n");
    }

    err = sim7080g_mqtt_publish(&sim7080g, "topic_to_pub_here", "data_to_publish_here", 0, false);
    if(err == ESP_OK)
    {
        printf("Message published successfully\n");
    }
    else
    {
        printf("Error publishing message\n");
    }
}
```

## How it works

### Internal steps for connecting to the device to an MQTT broker

1. (AT+CPIN) Check SIM card status - must return 'READY' otherwise there is an issue with the SIM card.
2. Device automatically attaches to an available network (NB-IoT OR LTE Cat-M). The device can be configured for a specific network only if desired.
3. (AT+CGATT?) Register for PS (Packet Switch) service.
   - When a device 'attaches' to PS service - its telling network it wants to send/receive data packets.
   - Its like telling the network 'I am a device - NOT just a phone'.
4. (AT+CNCFG) Configure device APN (Access Point Name) setting.
    - APN is like gateway configuration that tells the cellular network
      - What type of service the device needs (internet, private network, etc)
      - How to route data for this particular device
    - Different carriers have different APNs
    - Similar to 'choosing what WiFi network to talk to' but for cellular.
5. (AT+CNACT) Activate PDP (Packet Data Protocol) context - essentially this sets up a data connection on the network and results in an IP address being assigned to the device.
    - PDP context is a data structure containing routing info for this data connection
    - When activated its like 'turning on' the data connection
    - Assigns the device an IP address
    - This is similar to when an internet device gets an IP from DHCP over etheret, but with cellular protocols
6. Establish an MQTT connection with the defined broker.
7. Driver is ready to be used for publishing / subscribing data with an MQTT broker.

## Notes

### Protocol / Tech stack

- Physical layer : LTE Cat-M cellular OR NB-IoT cellular
- Data Link Layer: LTE protocols
- Network Layer  : IP
- Transport Layer: TCP
- App Layer      : MQTT
