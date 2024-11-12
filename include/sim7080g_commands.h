// This file contains fxns used to command and interact with the sim7080g - these are built ontop of fxns that send, read, and parse AT commands
#pragma once

#include "sim7080g_types.h"
#include "sim7080g_at_cmds.h"
#include "sim7080g_at_cmd_responses.h"

/**
 *
 * FXNs needed to interact with the sim7080g, but not generally exposed to user (for now)
 * THESE WILL ONLY DO ONE THING FOR ONE COMMAND - NO COMPLEX SEQUENCES - THOSE WILL BE IN THE MAIN DRIVE
 * For example the 'mqtt publish' will ONLY perform the publish, it will not first check if connected or anything like that
 *
 */

// Test AT communication with device (AT)
esp_err_t sim7080g_test_at(const sim7080g_handle_t *handle, at_test_status_t *test_status_out);

// Check SIM status (CPIN)
esp_err_t sim7080g_check_sim_status(const sim7080g_handle_t *handle, cpin_status_t *sim_status_out);

// Cycle CFUN - soft reset (CFUN)
esp_err_t sim7080g_set_functionality(const sim7080g_handle_t *handle, cfun_functionality_t fun_level);

// Check signal quality (CSQ)
esp_err_t sim7080g_check_signal_quality(const sim7080g_handle_t *handle, int8_t *rssi_dbm_out, uint8_t *ber_out);

// Disable command Echo (ATE0)
esp_err_t sim7080g_set_echo_mode(const sim7080g_handle_t *handle, ate_mode_t mode);

// Enable verbose error reporting (CMEE)
esp_err_t sim7080g_set_error_report_mode(const sim7080g_handle_t *handle, cmee_mode_t mode);

// Set PDP context ID (1), PDP type (IP), and APN [Define PDP context] (CGDCONT)
esp_err_t sim7080g_define_pdp_context(const sim7080g_handle_t *handle,
                                      uint8_t cid,
                                      cgdcont_pdp_type_t pdp_type,
                                      const char *apn);

// Get GPRS attached status
esp_err_t sim7080g_get_gprs_attachment(const sim7080g_handle_t *handle, cgatt_state_t *state_out);

// Set GPRS attached status
esp_err_t sim7080g_set_gprs_attachment(const sim7080g_handle_t *handle, cgatt_state_t state);

// Get operator info
esp_err_t sim7080g_get_operator_info(const sim7080g_handle_t *handle, cops_parsed_response_t *operator_info);

// Get APN
esp_err_t sim7080g_get_network_apn(const sim7080g_handle_t *handle,
                                   cgnapn_parsed_response_t *apn_info);
// Set APN

// Set APN using CNCFG
esp_err_t sim7080g_set_pdp_config(const sim7080g_handle_t *handle, const cncfg_context_config_t *config);

// Get APN using CNCFG
esp_err_t sim7080g_get_pdp_config(const sim7080g_handle_t *handle, cncfg_parsed_response_t *config_info);

// Get PDP context status (CNACT)
esp_err_t sim7080g_get_network_status(const sim7080g_handle_t *handle, cnact_parsed_response_t *status_info);

// Set (activate OR deactivate) PDP context (CNACT)
esp_err_t sim7080g_activate_network(const sim7080g_handle_t *handle, uint8_t pdp_idx, cnact_action_t action);

// Get MQTT configuration parameters (SMCONF)
esp_err_t sim7080g_get_mqtt_config(const sim7080g_handle_t *handle, smconf_config_t *config);

// Set MQTT configuration parameters (SMCONF)
esp_err_t sim7080g_set_mqtt_param(const sim7080g_handle_t *handle,
                                  smconf_param_t param,
                                  const char *value);
// Connect to MQTT broker
// Disconnect from MQTT broker

// Publish to MQTT broker
// Subscribe to MQTT broker

// --- Now for the static fxns like 'send at command' and 'parse response' etc that are used by the above commands --- //
