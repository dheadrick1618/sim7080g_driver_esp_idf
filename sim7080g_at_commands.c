#include <string.h>
#include <stdio.h>
#include "sim7080g_at_commands.h"

// Macros used to expand a command based on its type
// For example, TEST_CMD("AT+COPS") expands to "AT+COPS=?"
#define TEST_CMD(cmd) cmd "=?"
#define READ_CMD(cmd) cmd "?"
#define WRITE_CMD(cmd) cmd "="
#define EXECUTE_CMD(cmd) cmd

const at_cmd_t AT_CPIN = {
    .name = "AT+CPIN",
    .test = {TEST_CMD("AT+CPIN"), "OK"},
    .read = {READ_CMD("AT+CPIN"), "+CPIN: %s"},
    .write = {WRITE_CMD("AT+CPIN"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CSQ = {
    .name = "AT+CSQ",
    .test = {TEST_CMD("AT+CSQ"), "OK"},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+CSQ"), "+CSQ: %d,%d"}};

const at_cmd_t AT_CGATT = {
    .name = "AT+CGATT",
    .test = {TEST_CMD("AT+CGATT"), "OK"},
    .read = {READ_CMD("AT+CGATT"), "+CGATT: %d"},
    .write = {WRITE_CMD("AT+CGATT"), "OK"},
    .execute = {0}};

const at_cmd_t AT_COPS = {
    .name = "AT+COPS",
    .test = {TEST_CMD("AT+COPS"), "+COPS: (LIST)"},
    .read = {READ_CMD("AT+COPS"), "+COPS: %d,%d,\"%[^\"]\",%d"},
    .write = {WRITE_CMD("AT+COPS"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CGNAPN = {
    .name = "AT+CGNAPN",
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+CGNAPN"), "+CGNAPN: %d,\"%[^\"]\""}};

const at_cmd_t AT_CNCFG = {
    .name = "AT+CNCFG",
    .test = {TEST_CMD("AT+CNCFG"), "OK"},
    .read = {READ_CMD("AT+CNCFG"), "+CNCFG: %d,%d,\"%[^\"]\""},
    .write = {WRITE_CMD("AT+CNCFG"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CNACT = {
    .name = "AT+CNACT",
    .test = {TEST_CMD("AT+CNACT"), "OK"},
    .read = {READ_CMD("AT+CNACT"), "+CNACT: %d,%d,\"%[^\"]\""},
    .write = {WRITE_CMD("AT+CNACT"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMCONF = {
    .name = "AT+SMCONF",
    .test = {TEST_CMD("AT+SMCONF"), "OK"},
    .read = {READ_CMD("AT+SMCONF"), "+SMCONF: \"%[^\"]\",\"%[^\"]\""},
    .write = {WRITE_CMD("AT+SMCONF"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMCONN = {
    .name = "AT+SMCONN",
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+SMCONN"), "OK"}};

const at_cmd_t AT_SMSUB = {
    .name = "AT+SMSUB",
    .test = {0},
    .read = {0},
    .write = {WRITE_CMD("AT+SMSUB"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMPUB = {
    .name = "AT+SMPUB",
    .test = {0},
    .read = {0},
    .write = {WRITE_CMD("AT+SMPUB"), ">"},
    .execute = {0}};

const at_cmd_t AT_SMUNSUB = {
    .name = "AT+SMUNSUB",
    .test = {TEST_CMD("AT+SMUNSUB"), "OK"},
    .read = {0},
    .write = {WRITE_CMD("AT+SMUNSUB"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMDISC = {
    .name = "AT+SMDISC",
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+SMDISC"), "OK"}};

const at_cmd_t AT_SMSTATE = {
    .name = "AT+SMSTATE",
    .test = {TEST_CMD("AT+SMSTATE"), "+SMSTATE: (0-2)"},
    .read = {READ_CMD("AT+SMSTATE"), "+SMSTATE: %d"},
    .write = {0},
    .execute = {0}};

const at_cmd_t AT_CMEE = {
    .name = "AT+CMEE",
    .test = {TEST_CMD("AT+CMEE"), "OK"},
    .read = {READ_CMD("AT+CMEE"), "+CMEE: %d"},
    .write = {WRITE_CMD("AT+CMEE"), "OK"},
    .execute = {0}};