// This file contains fxns used to command and interact with the sim7080g - these are built ontop of fxns that send, read, and parse AT commands
#pragma once

#include "sim7080g_types.h"
#include "sim7080g_at_cmds.h"

/**
 *
 * FXNs needed to interact with the sim7080g, but not generally exposed to user (for now)
 * THESE WILL ONLY DO ONE THING FOR ONE COMMAND - NO COMPLEX SEQUENCES - THOSE WILL BE IN THE MAIN DRIVE
 * For example the 'mqtt publish' will ONLY perform the publish, it will not first check if connected or anything like that
 *
 */

// Cycle CFUN - soft reset (CFUN)

// Test AT communication with device (AT)

// Disable command Echo (ATE0)

// Enable verbose error reporting (CMEE)

// Check SIM status (CPIN)
esp_err_t sim7080g_check_sim_status(const sim7080g_handle_t *sim7080g_handle);

// Check signal quality (CSQ)

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
