#include <string.h>
#include <stdio.h>
#include "sim7080g_at_cmds.h"

// Macros used to expand a command based on its type
// For example, TEST_CMD("AT+COPS") expands to "AT+COPS=?"
#define TEST_CMD(cmd) cmd "=?"
#define READ_CMD(cmd) cmd "?"
#define WRITE_CMD(cmd) cmd "="
#define EXECUTE_CMD(cmd) cmd

///@brief The 'TEST' AT command simply sends 'AT' to the device which should response with OK, indicating communication is working
///@note The 'test' field is empty because the command is the same as the name (so in our design it is structured like an EXECUTE type command)
const at_cmd_t AT_TEST = {
    .name = "AT",
    .description = "Test AT Command - Test communication with device",
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT"), "OK"}};

const at_cmd_t AT_CPIN = {
    .name = "AT+CPIN",
    .description = "Enter PIN - Controls SIM card PIN operations",
    .test = {
        TEST_CMD("AT+CPIN"),
        "OK"},
    .read = {READ_CMD("AT+CPIN"), "+CPIN: %s"},
    .write = {WRITE_CMD("AT+CPIN"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CFUN = {
    .name = "AT+CFUN",
    .description = "Set Phone Functionality - Set phone functionality to minimum, full, or disable",
    .test = {TEST_CMD("AT+CFUN"), "OK"},
    .read = {READ_CMD("AT+CFUN"), "+CFUN: %d"},
    .write = {WRITE_CMD("AT+CFUN"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CSQ = {
    .name = "AT+CSQ",
    .description = "Signal Quality Report - Get current signal strength (RSSI) and bit error rate (BER)",
    .test = {TEST_CMD("AT+CSQ"), "+CSQ: (0-31,99),(0-7,99)"},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+CSQ"), "+CSQ: %d,%d"}};

const at_cmd_t AT_ATE = {
    .name = "ATE",
    .description = "Set Command Echo Mode - Controls whether device echoes back commands",
    .test = {0},  // ATE has no test command
    .read = {0},  // ATE has no read command
    .write = {0}, // ATE has no write command
    .execute = {
        "ATE", // Format string includes the mode value
        "OK"}};

const at_cmd_t AT_CMEE = {
    .name = "AT+CMEE",
    .description = "Enable Verbose Error Reporting - Enable detailed error codes in response",
    .test = {TEST_CMD("AT+CMEE"), "OK"},
    .read = {READ_CMD("AT+CMEE"), "+CMEE: %d"},
    .write = {WRITE_CMD("AT+CMEE"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CGDCONT = {
    .name = "AT+CGDCONT",
    .description = "Define PDP Context - Set PDP context parameters including Context ID, Type, and APN",
    .test = {
        TEST_CMD("AT+CGDCONT"),
        "+CGDCONT: (1-15),\"IP\",,,(0-2),(0-4),(0)"},
    .read = {READ_CMD("AT+CGDCONT"), "+CGDCONT: %d,\"%[^\"]\",\"%[^\"]\",%*[^,],%*[^,],%*[^,],%*[^\r\n]"},
    .write = {WRITE_CMD("AT+CGDCONT"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CGATT = {
    .name = "AT+CGATT",
    .description = "GPRS Service Attach/Detach - Control device attachment to GPRS service",
    .test = {
        TEST_CMD("AT+CGATT"),
        "+CGATT: (0,1)"},
    .read = {READ_CMD("AT+CGATT"), "+CGATT: %d"},
    .write = {WRITE_CMD("AT+CGATT"), "OK"},
    .execute = {0}};

const at_cmd_t AT_COPS = {
    .name = "AT+COPS",
    .description = "Operator Selection - Get current network operator information",
    .test = {
        TEST_CMD("AT+COPS"),
        "+COPS: (list of supported<stat>,long alphanumeric<oper>)"},
    .read = {READ_CMD("AT+COPS"), "+COPS: %d,%d,\"%[^\"]\"%d"},
    .write = {WRITE_CMD("AT+COPS"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CGNAPN = {
    .name = "AT+CGNAPN",
    .description = "Get Network APN - Retrieve network-provided APN in CAT-M or NB-IOT mode",
    .test = {
        TEST_CMD("AT+CGNAPN"),
        "+CGNAPN: (0,1),120"},
    .read = {0},  // No read command
    .write = {0}, // No write command
    .execute = {
        EXECUTE_CMD("AT+CGNAPN"),
        "+CGNAPN: %d,\"%[^\"]\"" // Expecting status and possibly an APN string
    }};

const at_cmd_t AT_CNCFG = {
    .name = "AT+CNCFG",
    .description = "PDP Configure - Configure PDP context parameters",
    .test = {
        TEST_CMD("AT+CNCFG"),
        "+CNCFG: (0-3),(0-4),150,127,127,(0-3)"},
    .read = {READ_CMD("AT+CNCFG"), "+CNCFG: %d,%d,\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",%d"},
    .write = {WRITE_CMD("AT+CNCFG"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CNACT = {
    .name = "AT+CNACT",
    .description = "APP Network Active - Control PDP context activation",
    .test = {
        TEST_CMD("AT+CNACT"),
        "+CNACT: (0-3),(0-2)"},
    .read = {
        READ_CMD("AT+CNACT"),
        "+CNACT: %d,%d,\"%[^\"]\"" // Format for each context
    },
    .write = {WRITE_CMD("AT+CNACT"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMCONF = {
    .name = "AT+SMCONF",
    .description = "MQTT Configuration - Set MQTT parameters including broker URL, credentials, and session options",
    .test = {TEST_CMD("AT+SMCONF"), "OK"},
    .read = {READ_CMD("AT+SMCONF"), "+SMCONF: \"%[^\"]\",\"%[^\"]\""},
    .write = {WRITE_CMD("AT+SMCONF"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMCONN = {
    .name = "AT+SMCONN",
    .description = "MQTT Connect - Establish connection to configured MQTT broker",
    .test = {0},  // No test command
    .read = {0},  // No read command
    .write = {0}, // No write command
    .execute = {
        EXECUTE_CMD("AT+SMCONN"),
        "OK"}};

const at_cmd_t AT_SMPUB = {
    .name = "AT+SMPUB",
    .description = "MQTT Publish - Publish message to specified topic with QoS and retain settings",
    .test = {
        TEST_CMD("AT+SMPUB"),
        "+SMPUB: 128,(0-1024),(0-2),(0-1)"},
    .read = {0}, // No read command
    .write = {
        WRITE_CMD("AT+SMPUB"),
        ">" // Special case - expects > prompt then message content
    },
    .execute = {0}};

const at_cmd_t AT_SMSTATE = {
    .name = "AT+SMSTATE",
    .description = "MQTT State Check - Query current MQTT connection status",
    .test = {
        TEST_CMD("AT+SMSTATE"),
        "+SMSTATE: (0-2)"},
    .read = {READ_CMD("AT+SMSTATE"), "+SMSTATE: %d"},
    .write = {0},
    .execute = {0}};

const at_cmd_t AT_CEREG = {
    .name = "AT+CEREG",
    .description = "EPS Network Registration Status",
    .test = {
        TEST_CMD("AT+CEREG"),
        "+CEREG: (0-2,4)"},
    .read = {READ_CMD("AT+CEREG"), "+CEREG: %d,%d[,[\"%[^\"]\"],[\"%[^\"]\"],[\"%[^\"]\"],%d][,,[,[%[^]],[%[^]]]]]"},
    .write = {WRITE_CMD("AT+CEREG"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMDISC = {
    .name = "AT+SMDISC",
    .description = "MQTT Disconnect - Terminate the MQTT broker connection",
    .test = {0},  // No test command
    .read = {0},  // No read command
    .write = {0}, // No write command
    .execute = {
        EXECUTE_CMD("AT+SMDISC"),
        "OK"}};

// const at_cmd_t AT_SMSUB = {
//     .name = "AT+SMSUB",
//     .description = "MQTT Subscribe - Subscribe to specified MQTT topic with QoS level",
//     .test = {0},
//     .read = {0},
//     .write = {WRITE_CMD("AT+SMSUB"), "OK"},
//     .execute = {0}};

// const at_cmd_t AT_SMPUB = {
//     .name = "AT+SMPUB",
//     .description = "MQTT Publish - Publish message to specified topic with QoS and retain settings",
//     .test = {0},
//     .read = {0},
//     .write = {WRITE_CMD("AT+SMPUB"), ">"},
//     .execute = {0}};

// const at_cmd_t AT_SMUNSUB = {
//     .name = "AT+SMUNSUB",
//     .description = "MQTT Unsubscribe - Unsubscribe from previously subscribed MQTT topic",
//     .test = {TEST_CMD("AT+SMUNSUB"), "OK"},
//     .read = {0},
//     .write = {WRITE_CMD("AT+SMUNSUB"), "OK"},
//     .execute = {0}};

// TODO - Implement this if its found relevant later to check  transport layer connection
//  const at_cmd_t AT_CASTATE = {
//      .name = "AT+CASTATE",
//      .description = "Query TCP/UDP Connection Status - Check current connection status",
//      .test = {0},
//      .read = {READ_CMD("AT+CASTATE"), "+CASTATE: %d,%d"},
//      .write = {0},
//      .execute = {0}};

// ------------------------- THESE COMMANDS MAY BE USEFUL LATER -------------------------//
// --------------------------------------------------------------------------------------//
// const at_cmd_t AT_CNMP = {
//     .name = "AT+CNMP",
//     .description = "Preferred Mode Selection - Select network mode (GSM/LTE)",
//     .test = {TEST_CMD("AT+CNMP"), "+CNMP: (2,13,38,51)"},
//     .read = {READ_CMD("AT+CNMP"), "+CNMP: %d"},
//     .write = {WRITE_CMD("AT+CNMP"), "OK"},
//     .execute = {0}};

// const at_cmd_t AT_CMNB = {
//     .name = "AT+CMNB",
//     .description = "Preferred Selection between CAT-M and NB-IoT",
//     .test = {TEST_CMD("AT+CMNB"), "+CMNB: (1-3)"},
//     .read = {READ_CMD("AT+CMNB"), "+CMNB: %d"},
//     .write = {WRITE_CMD("AT+CMNB"), "OK"},
//     .execute = {0}};

// const at_cmd_t AT_CPSI = {
//     .name = "AT+CPSI",
//     .description = "Inquiring UE System Information",
//     .test = {TEST_CMD("AT+CPSI"), "OK"},
//     .read = {READ_CMD("AT+CPSI"), "+CPSI: %[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s"},
//     .write = {0},
//     .execute = {0}};

// const at_cmd_t AT_CRESET = {
//     .name = "AT+CRESET",
//     .description = "Reset Module",
//     .test = {TEST_CMD("AT+CRESET"), "OK"},
//     .read = {0},
//     .write = {0},
//     .execute = {EXECUTE_CMD("AT+CRESET"), "OK"}};

// const at_cmd_t AT_CGSN = {
//     .name = "AT+CGSN",
//     .description = "Request Product Serial Number (IMEI)",
//     .test = {TEST_CMD("AT+CGSN"), "OK"},
//     .read = {0},
//     .write = {0},
//     .execute = {EXECUTE_CMD("AT+CGSN"), "%15s"}};

// const at_cmd_t AT_CGMI = {
//     .name = "AT+CGMI",
//     .description = "Request Manufacturer Identification",
//     .test = {TEST_CMD("AT+CGMI"), "OK"},
//     .read = {0},
//     .write = {0},
//     .execute = {EXECUTE_CMD("AT+CGMI"), "%s"}};

// const at_cmd_t AT_CGMM = {
//     .name = "AT+CGMM",
//     .description = "Request Model Identification",
//     .test = {TEST_CMD("AT+CGMM"), "OK"},
//     .read = {0},
//     .write = {0},
//     .execute = {EXECUTE_CMD("AT+CGMM"), "%s"}};

// const at_cmd_t AT_CBANDCFG = {
//     .name = "AT+CBANDCFG",
//     .description = "Configure CAT-M or NB-IOT Band",
//     .test = {TEST_CMD("AT+CBANDCFG"), "+CBANDCFG: (CAT-M,NB-IOT),(list of supported bands)"},
//     .read = {READ_CMD("AT+CBANDCFG"), "+CBANDCFG: \"%[^\"]\",\"%[^\"]\""},
//     .write = {WRITE_CMD("AT+CBANDCFG"), "OK"},
//     .execute = {0}};

// const at_cmd_t AT_CPSMS = {
//     .name = "AT+CPSMS",
//     .description = "Power Saving Mode Setting",
//     .test = {TEST_CMD("AT+CPSMS"), "+CPSMS: (0,1),,,(%8s),(%8s)"},
//     .read = {READ_CMD("AT+CPSMS"), "+CPSMS: %d,,,\"%[^\"]\",\"%[^\"]\""},
//     .write = {WRITE_CMD("AT+CPSMS"), "OK"},
//     .execute = {0}};