#include <string.h>
#include <stdio.h>
#include "sim7080g_at_commands.h"

// Macros used to expand a command based on its type
// For example, TEST_CMD("AT+COPS") expands to "AT+COPS=?"
#define TEST_CMD(cmd) cmd "=?"
#define READ_CMD(cmd) cmd "?"
#define WRITE_CMD(cmd) cmd "="
#define EXECUTE_CMD(cmd) cmd

const at_cmd_t AT_ECHO_OFF = {
    .name = "ATE0",
    .description = "Echo Off - Disable command echo",
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("ATE0"), "OK"}};

const at_cmd_t AT_CPIN = {
    .name = "AT+CPIN",
    .description = "Enter PIN - Check if SIM card requires a PIN or if it's ready",
    .test = {TEST_CMD("AT+CPIN"), "OK"},
    .read = {READ_CMD("AT+CPIN"), "+CPIN: %s"},
    .write = {WRITE_CMD("AT+CPIN"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CSQ = {
    .name = "AT+CSQ",
    .description = "Signal Quality Report - Get current signal strength (RSSI) and bit error rate (BER)",
    .test = {TEST_CMD("AT+CSQ"), "OK"},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+CSQ"), "+CSQ: %d,%d"}};

const at_cmd_t AT_CGATT = {
    .name = "AT+CGATT",
    .description = "GPRS Service Attach/Detach - Control device attachment to GPRS service",
    .test = {TEST_CMD("AT+CGATT"), "OK"},
    .read = {READ_CMD("AT+CGATT"), "+CGATT: %d"},
    .write = {WRITE_CMD("AT+CGATT"), "OK"},
    .execute = {0}};

const at_cmd_t AT_COPS = {
    .name = "AT+COPS",
    .description = "Operator Selection - Select and register GSM/UMTS/LTE network operator",
    .test = {TEST_CMD("AT+COPS"), "+COPS: (LIST)"},
    .read = {READ_CMD("AT+COPS"), "+COPS: %d,%d,\"%[^\"]\",%d"},
    .write = {WRITE_CMD("AT+COPS"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CGNAPN = {
    .name = "AT+CGNAPN",
    .description = "Get Network APN - Retrieve Access Point Name from network in CAT-M or NB-IOT mode",
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+CGNAPN"), "+CGNAPN: %d,\"%[^\"]\""}};

const at_cmd_t AT_CNCFG = {
    .name = "AT+CNCFG",
    .description = "PDP Context Configuration - Set up PDP (Packet Data Protocol) context parameters",
    .test = {TEST_CMD("AT+CNCFG"), "OK"},
    .read = {READ_CMD("AT+CNCFG"), "+CNCFG: %d,%d,\"%[^\"]\""},
    .write = {WRITE_CMD("AT+CNCFG"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CNACT = {
    .name = "AT+CNACT",
    .description = "App Network Activation - Activate or deactivate PDP context for network connection",
    .test = {TEST_CMD("AT+CNACT"), "OK"},
    .read = {READ_CMD("AT+CNACT"), "+CNACT: %d,%d,\"%[^\"]\""},
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
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+SMCONN"), "OK"}};

const at_cmd_t AT_SMSUB = {
    .name = "AT+SMSUB",
    .description = "MQTT Subscribe - Subscribe to specified MQTT topic with QoS level",
    .test = {0},
    .read = {0},
    .write = {WRITE_CMD("AT+SMSUB"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMPUB = {
    .name = "AT+SMPUB",
    .description = "MQTT Publish - Publish message to specified topic with QoS and retain settings",
    .test = {0},
    .read = {0},
    .write = {WRITE_CMD("AT+SMPUB"), ">"},
    .execute = {0}};

const at_cmd_t AT_SMUNSUB = {
    .name = "AT+SMUNSUB",
    .description = "MQTT Unsubscribe - Unsubscribe from previously subscribed MQTT topic",
    .test = {TEST_CMD("AT+SMUNSUB"), "OK"},
    .read = {0},
    .write = {WRITE_CMD("AT+SMUNSUB"), "OK"},
    .execute = {0}};

const at_cmd_t AT_SMDISC = {
    .name = "AT+SMDISC",
    .description = "MQTT Disconnect - Terminate active MQTT broker connection",
    .test = {0},
    .read = {0},
    .write = {0},
    .execute = {EXECUTE_CMD("AT+SMDISC"), "OK"}};

const at_cmd_t AT_SMSTATE = {
    .name = "AT+SMSTATE",
    .description = "MQTT State Check - Query current MQTT connection status",
    .test = {TEST_CMD("AT+SMSTATE"), "+SMSTATE: (0-2)"},
    .read = {READ_CMD("AT+SMSTATE"), "+SMSTATE: %d"},
    .write = {0},
    .execute = {0}};

const at_cmd_t AT_CMEE = {
    .name = "AT+CMEE",
    .description = "Enable Verbose Error Reporting - Enable detailed error codes in response",
    .test = {TEST_CMD("AT+CMEE"), "OK"},
    .read = {READ_CMD("AT+CMEE"), "+CMEE: %d"},
    .write = {WRITE_CMD("AT+CMEE"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CFUN = {
    .name = "AT+CFUN",
    .description = "Set Phone Functionality - Set phone functionality to minimum, full, or disable",
    .test = {TEST_CMD("AT+CFUN"), "OK"},
    .read = {READ_CMD("AT+CFUN"), "+CFUN: %d"},
    .write = {WRITE_CMD("AT+CFUN"), "OK"},
    .execute = {0}};

const at_cmd_t AT_CEREG = {
    .name = "AT+CEREG",
    .description = "EPS Network Registration Status - Controls and reports network registration and location information",
    .test = {TEST_CMD("AT+CEREG"), "+CEREG: (0-2,4)"},
    .read = {READ_CMD("AT+CEREG"), "+CEREG: %d,%d"}, // Basic format: mode,status
    .write = {WRITE_CMD("AT+CEREG"), "OK"},
    .execute = {0}};

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