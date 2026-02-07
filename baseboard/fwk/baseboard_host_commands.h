/* Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* host command customization configuration */

#ifndef __BASEBOARD_HOST_COMMANDS_H
#define __BASEBOARD_HOST_COMMANDS_H

#define EC_CMD_PRIVACY_SWITCHES_CHECK_MODE 0x3E14

struct ec_response_privacy_switches_check {
	uint8_t microphone;
	uint8_t camera;
} __ec_align1;

/*****************************************************************************/
/*
 * Battery extender control
 */
#define EC_CMD_BATTERY_EXTENDER	0x3E24

struct ec_params_battery_extender {
	uint8_t disable;
	uint8_t trigger_days;
	uint16_t reset_minutes;
	uint8_t cmd;
	uint8_t manual;
} __ec_align1;

struct ec_response_battery_extender {
	uint8_t current_stage;
	uint16_t trigger_days;
	uint16_t reset_minutes;
	uint8_t disable;
	uint64_t trigger_timedelta;
	uint64_t reset_timedelta;
} __ec_align1;

enum battery_extender_cmd {
	BATT_EXTENDER_WRITE_CMD,
	BATT_EXTENDER_READ_CMD,
};

#endif /* __BASEBOARD_HOST_COMMANDS_H */
