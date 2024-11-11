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

// Enable verbose error reporting (CMEE)

// Get GPRS attached status

// Get operator info

// Get APN
// Set APN

// Get PDP context status (CNACT)
// Set (activate OR deactivate) PDP context (CNACT)

// Get MQTT configuration parameters (SMCONF)
// Set MQTT configuration parameters (SMCONF)

// Connect to MQTT broker
// Disconnect from MQTT broker

// Publish to MQTT broker
// Subscribe to MQTT broker

// --- Now for the static fxns like 'send at command' and 'parse response' etc that are used by the above commands --- //
