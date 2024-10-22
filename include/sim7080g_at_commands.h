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
    at_cmd_info_t test;
    at_cmd_info_t read;
    at_cmd_info_t write;
    at_cmd_info_t execute;
} at_cmd_t;

extern const at_cmd_t AT_CPIN;

/// @brief Signal Quality Report - Get the current signal strength and bit error rate
/// @details This command returns the received signal strength indication (RSSI) and channel bit error rate (BER) from the ME.
/// @return On success:
///   - +CSQ: <rssi>,<ber>
///   - OK
/// @return On failure:
///   - +CME ERROR: <err>
/// @param rssi
///   - 0: -115 dBm or less
///   - 1: -111 dBm
///   - 2-30: -110 to -54 dBm
///   - 31: -52 dBm or greater
///   - 99: Not known or not detectable
/// @param ber (in percent)
///   - 0-7: As RXQUAL values in the table in GSM 05.08 subclause 7.2.4
///   - 99: Not known or not detectable
/// @note This setting is not saved (NO_SAVE)
extern const at_cmd_t AT_CSQ;

/// @brief Attach or Detach from GPRS Service
/// @details This command is used to attach the MT to, or detach the MT from, the GPRS service.
/// @param state
///   - 0: Detached
///   - 1: Attached
/// @return On success:
///   - OK
/// @return On failure:
///   - +CME ERROR: <err>
/// @note The read command returns the current GPRS service state.
/// @note Maximum response time: 75 seconds
/// @note This setting is not saved (NO_SAVE)
extern const at_cmd_t AT_CGATT;

/// @brief Operator Selection - Select a network operator
/// @details This command forces an attempt to select and register the GSM/UMTS/LTE network operator.
/// @param mode
///   - 0: Automatic mode; <oper> field is ignored
///   - 1: Manual; <oper> field shall be present, and <AcT> optionally
///   - 2: Deregister from network
///   - 3: Set only <format> (for read command +COPS?)
///   - 4: Manual/automatic; if manual selection fails, automatic mode is entered
/// @param format
///   - 0: Long format alphanumeric <oper>
///   - 1: Short format alphanumeric <oper>
///   - 2: Numeric <oper>
/// @param oper Operator in format as per <format>
/// @param act
///   - 0: GSM
///   - 1: GSM Compact
///   - 3: GSM EGPRS
///   - 7: LTE M1 A GB
///   - 9: LTE NB S1
/// @return On success:
///   - OK
/// @return On failure:
///   - +CME ERROR: <err>
/// @note The test command returns available operators and supported modes.
/// @note The read command returns the current mode and the currently selected operator.
/// @note Maximum response time: Test command: 45 seconds, Write command: 120 seconds
/// @note This setting is automatically saved (AUTO_SAVE)
extern const at_cmd_t AT_COPS;

/// @brief Get Network APN in CAT-M or NB-IOT
/// @details This command retrieves the Access Point Name (APN) provided by the network when the device
/// is registered on a CAT-M or NB-IOT network. In GSM networks, the APN will always be NULL.
/// @note The command has no parameters for execution.
/// @return On success:
///   - +CGNAPN: <valid>,<Network_APN>
///   - OK
/// @return On failure:
///   - +CME ERROR: <err>
/// @param valid 0: Network did not send APN parameter to UE (Network_APN is NULL)
///              1: Network sent APN parameter to UE
/// @param Network_APN String type. The APN parameter sent by the network upon successful registration.
///                    Maximum length is defined by the <length> parameter in the test command response.
/// @note In CAT-M or NB-IOT, <Network_APN> is valid if the core network responds with an attach accept
/// message that includes the APN parameter after the UE sends an attach request message.
extern const at_cmd_t AT_CGNAPN;

/// @brief PDP Configure - Configure PDP context parameters
/// @details This command is used to configure parameters for a specified PDP context.
/// @param pdpidx PDP Context Identifier (0-3)
/// @param ip_type Packet Data Protocol type
///   - 0: Dual PDN Stack
///   - 1: Internet Protocol Version 4
///   - 2: Internet Protocol Version 6
///   - 3: NONIP
///   - 4: EX_NONIP
/// @param APN Access Point Name (string, optional)
/// @param username Username for authentication (optional)
/// @param client_password Password for authentication (optional)
/// @param authentication Authentication method (optional)
///   - 0: NONE
///   - 1: PAP
///   - 2: CHAP
///   - 3: PAP or CHAP
/// @return On success:
///   - OK
/// @return On failure:
///   - +CME ERROR: <err>
/// @note The read command returns the current configuration for each PDP context:
///   - +CNCFG: <pdpidx>,<ip_type>,<APN>,<username>,<client_password>,<authentication>
/// @note The test command returns the supported ranges for each parameter:
///   - +CNCFG: (range of supported <pdpidx>s),(range of supported <ip_type>s),<len_APN>,<len_username>,<len_password>,(range of supported <authentication>s)
/// @note This setting is not saved (implied by the absence of a saving mode in the documentation)
extern const at_cmd_t AT_CNCFG;

/// @brief APP Network Active - Activate or deactivate PDP context
/// @details This command is used to activate or deactivate a specified PDP context.
/// @param pdpidx PDP Context Identifier (0-3)
/// @param action
///   - 0: Deactivate
///   - 1: Activate
///   - 2: Auto Activate (will automatically retry if activation fails)
/// @return On success:
///   - OK
/// @return On failure:
///   - +CME ERROR: <err>
/// @note The read command returns the current status and IP address for each PDP context:
///   - +CNACT: <pdpidx>,<status>,<address>
/// @note Status values:
///   - 0: Deactivated
///   - 1: Activated
///   - 2: In operation
/// @note "+APP PDP: <pdpidx>,ACTIVE" will be reported when the network is activated
/// @note "+APP PDP: <pdpidx>,DEACTIVE" will be reported when the network is deactivated
/// @note This setting is not saved (NO_SAVE)
extern const at_cmd_t AT_CNACT;

/// @brief Set MQTT Parameter - Configure various MQTT settings
/// @details This command is used to set various MQTT parameters such as client ID, server URL, keepalive time, etc.
/// @param MQTTParamTag The parameter to be set:
///   - "CLIENTID": Client connection ID (0-128 characters)
///   - "URL": Server URL address (<server domain>,[<tcpPort>])
///   - "KEEPTIME": Hold connect time (0-60-65535 seconds)
///   - "CLEANSS": Session clean (0: Resume based on present session, 1: New session)
///   - "USERNAME": User name
///   - "PASSWORD": Password
///   - "QOS": Quality of Service level (0: At most once, 1: At least once, 2: Exactly once)
///   - "TOPIC": Publish topic name
///   - "MESSAGE": Publish message details
///   - "RETAIN": Retain flag (0: Don't retain, 1: Retain)
///   - "SUBHEX": Subscribe data format (0: Normal, 1: Hexadecimal)
///   - "ASYNCMODE": Asynchronous mode (0: Synchronous, 1: Asynchronous)
/// @param MQTTParamValue The value for the specified parameter
/// @return On success:
///   - OK
/// @return On failure:
///   - ERROR
/// @note The read command returns the current configuration for all parameters
/// @note The test command returns the supported ranges for each parameter
extern const at_cmd_t AT_SMCONF;

/// @brief MQTT Connection - Establish MQTT connection
/// @details This command is used to establish a connection to the MQTT broker using the previously configured parameters.
/// @return On success:
///   - OK
/// @return On failure:
///   - ERROR
extern const at_cmd_t AT_SMCONN;

/// @brief Subscribe Packet - Subscribe to an MQTT topic
/// @details This command is used to subscribe to a specified MQTT topic.
/// @param topic The topic to subscribe to (max length returned by test command)
/// @param qos Quality of Service level (0: At most once, 1: At least once, 2: Exactly once)
/// @return On success:
///   - OK
/// @return On failure:
///   - ERROR
/// @note The test command returns the supported ranges for each parameter
extern const at_cmd_t AT_SMSUB;

/// @brief Send Packet - Publish an MQTT message
/// @details This command is used to publish a message to a specified MQTT topic.
/// @param topic The topic to publish to (max length returned by test command)
/// @param content_length The length of the message content (0-1024)
/// @param qos Quality of Service level (0: At most once, 1: At least once, 2: Exactly once)
/// @param retain Retain flag (0: Don't retain, 1: Retain)
/// @return On success:
///   - OK
/// @return On failure:
///   - ERROR
/// @note After sending the command, enter the message content and press CTRL+Z to send
/// @note The test command returns the supported ranges for each parameter
extern const at_cmd_t AT_SMPUB;

/// @brief Unsubscribe Packet - Unsubscribe from an MQTT topic
/// @details This command is used to unsubscribe from a previously subscribed MQTT topic.
/// @param topic The topic to unsubscribe from (max length returned by test command)
/// @return On success:
///   - OK
/// @return On failure:
///   - ERROR
/// @note The test command returns the maximum length of the topic parameter
extern const at_cmd_t AT_SMUNSUB;

/// @brief Disconnect MQTT - Terminate the MQTT connection
/// @details This command is used to disconnect from the MQTT broker.
/// @return On success:
///   - OK
/// @return On failure:
///   - ERROR
/// @note This is an execution command with no parameters
/// @note No read or test commands are available for this command
/// @note The disconnection is performed immediately upon execution of this command
extern const at_cmd_t AT_SMDISC;

/// @brief Inquire MQTT Connection Status - Check the current MQTT connection state
/// @details This command is used to check the current status of the MQTT connection.
/// @return On success:
///   - +SMSTATE: <status>
///   - OK
/// @note Status values:
///   - 0: MQTT disconnected
///   - 1: MQTT connected
///   - 2: MQTT connected with Session Present flag set
/// @note The test command returns the list of supported status values
extern const at_cmd_t AT_SMSTATE;

// TODO - AT+CGSN - request product serial number ID
// TODO - AT+CGMI - request manf id
// TODO - AT+CGMM - request model id