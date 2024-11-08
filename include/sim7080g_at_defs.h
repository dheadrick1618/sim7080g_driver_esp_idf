#pragma once

typedef enum
{
    AT_CMD_TYPE_TEST,
    AT_CMD_TYPE_READ,
    AT_CMD_TYPE_WRITE,
    AT_CMD_TYPE_EXECUTE,
} at_cmd_type_t;

typedef struct
{
    const char *cmd_string;
    const char *response_format;
} at_cmd_info_t;

typedef struct
{
    const char *name;
    const char *description;
    at_cmd_info_t test;
    at_cmd_info_t read;
    at_cmd_info_t write;
    at_cmd_info_t execute;
} at_cmd_t;