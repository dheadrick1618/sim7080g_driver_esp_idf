idf_component_register(SRCS "sim7080g_commands.c" "sim7080g_uart.c" "sim7080g_at_cmd_responses.c" "sim7080g_at_cmds.c" "sim7080g_driver_esp_idf.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_uart)
