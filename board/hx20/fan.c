/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* MCHP MEC fan control module. */

/* This assumes 2-pole fan. For each rotation, 5 edges are measured. */


#include "common.h"
#include "console.h"
#include "fan.h"
#include "gpio.h"
#include "hooks.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "registers.h"
#include "tfdp_chip.h"
#include "util.h"
#include "math_util.h"


#define CPRINTS(format, args...) cprints(CC_THERMAL, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_THERMAL, format, ## args)

/* Maximum tach reading/target value */
#define MAX_TACH 0xffff

/* Tach target value for disable fan */
#define FAN_OFF_TACH 0xffff

/*
 * RPM = (n - 1) * m * f * 60 / poles / TACH
 *   n = number of edges = 9
 *   m = multiplier defined by RANGE = 1 in our case
 *   f = 100K
 *   poles = 2
 */
#define TACH_TO_RPM(tach) ((2*100000*60) / MAX((tach), 1))


#define FAN_PID_I_INV	100
#define FAN_PID_I_MAX	(10*FAN_PID_I_INV)


static int rpm_setting[FAN_CH_COUNT];
static int duty_setting[FAN_CH_COUNT];
static int integral_factor[FAN_CH_COUNT];

static int in_rpm_mode = 1;

void fan_set_enabled(int ch, int enabled)
{
	if (in_rpm_mode) {
		if (enabled) {
			fan_set_rpm_target(ch, rpm_setting[ch]);
			pwm_enable(ch, enabled);
		} else {
			integral_factor[ch] = 0;
		}
	} else {
		if (enabled) {
			pwm_enable(ch, enabled);
			fan_set_duty(ch, duty_setting[ch]);
		} else {
			pwm_enable(ch, enabled);
			fan_set_duty(ch, 0);
		}
	}
}

int fan_get_enabled(int ch)
{
	return pwm_get_enabled(ch);
}

int fan_rpm_to_percent(int fan, int rpm)
{
	int pct, max, min;

	if (!rpm) {
		pct = 0;
	} else {
		min = fans[fan].rpm->rpm_min;
		max = fans[fan].rpm->rpm_max;

		if (rpm < min)
			rpm = min;
		else if (rpm > max)
			rpm = max;

		pct = (rpm - min) / ((max - min) / 100);
		/*CPRINTS(" Fan max min : %d , %d", max, min);*/
	}
	/*CPRINTS(" Fan PCT = %d ", pct);*/
	return pct;
}

void fan_set_duty(int ch, int percent)
{
	if (ch < 0 || ch > MCHP_TACH_ID_MAX || ch > FAN_CH_COUNT)
		return;
	if (percent < 0)
		percent = 0;
	else if (percent > 100)
		percent = 100;

	duty_setting[ch] = percent;

	pwm_set_duty(ch, percent);
}

int fan_get_duty(int ch)
{
	if (ch < 0 || ch > MCHP_TACH_ID_MAX || ch > FAN_CH_COUNT)
		return -1;
	return duty_setting[ch];
}

int fan_get_rpm_mode(int ch)
{
	return in_rpm_mode;
}

void fan_set_rpm_mode(int ch, int rpm_mode)
{
	in_rpm_mode = rpm_mode;
}

int fan_get_rpm_actual(int ch)
{
	if (ch < 0 || ch > MCHP_TACH_ID_MAX || ch > FAN_CH_COUNT)
		return -1;
	if (MCHP_TACH_CTRL_CNT(ch) == 0xffff)
		return 0;
	else
		return TACH_TO_RPM(MCHP_TACH_CTRL_CNT(ch));
}

int fan_get_rpm_target(int ch)
{
	if (ch < 0 || ch > FAN_CH_COUNT)
		return -1;
	return rpm_setting[ch];
}
/**
 * fan gain should not be greater than 1 for stability
 * since fan goes up to 5500rpm and pwm is 0-100%
 * then we can choose something less than
 * 1/(100/5500)
 * This is called at Hhz from the thermal task if active
 */
void fan_set_rpm_target(int ch, int rpm)
{
	int delta;
	int pct = 0;

	if (ch < 0 || ch > MCHP_TACH_ID_MAX || ch > FAN_CH_COUNT)
		return;
	pct = fan_rpm_to_percent(ch, rpm);
	delta = rpm - fan_get_rpm_actual(ch);
	/**
	 * Only integrate in steady state
	 * allow the fan to ramp naturally in response to a step
	 * assuming a time delta of around a second or so otherwise
	 * we will integrate during the fan ramp up/ramp down time
	 * also do not integrate when the fan is off
	 */
	if (rpm == rpm_setting[ch] && rpm > 0)
		integral_factor[ch] += delta;

	rpm_setting[ch] = rpm;
	duty_setting[ch] = pct;


	/*Cap integral factor at FAN_PID_I_MAX, to prevent runaway conditions */
	integral_factor[ch] = MIN(MAX(integral_factor[ch], -FAN_PID_I_MAX),
							FAN_PID_I_MAX);

#if 0
	CPRINTS(" Fan delta : %drpm %dintegral, commanded %d",
			delta, integral_factor[ch],
			pct +
			integral_factor[ch]/FAN_PID_I_INV);
#endif

	if (rpm == 0)
		integral_factor[ch] = 0;
	
	fan_set_duty(ch, pct + integral_factor[ch]/FAN_PID_I_INV);
}

enum fan_status fan_get_status(int ch)
{
	/* TODO */
	if (fan_get_rpm_actual(ch) == 0)
		return FAN_STATUS_STOPPED;
	if (ABS(integral_factor[ch]) >= FAN_PID_I_MAX)
		return FAN_STATUS_FRUSTRATED;
	if (ABS(fan_get_rpm_actual(ch)-fan_get_rpm_target(ch)) > 200)
		return FAN_STATUS_CHANGING;
	return FAN_STATUS_LOCKED;
}

int fan_is_stalled(int ch)
{
	/* Must be enabled with non-zero target to stall */
	if (!fan_get_enabled(ch) || fan_get_rpm_target(ch) == 0)
		return 0;

	/* Check for stall condition */
	return fan_get_status(ch) == FAN_STATUS_STOPPED;
}

void fan_channel_setup(int ch, unsigned int flags)
{
	int i;

	for (i = 0; i < FAN_CH_COUNT; ++i) {
		pwm_slp_en(pwm_channels[i].channel, 0);
		pwm_configure(pwm_channels[i].channel,
			      pwm_channels[i].flags & PWM_CONFIG_ACTIVE_LOW,
			      pwm_channels[i].flags & PWM_CONFIG_ALT_CLOCK);
		pwm_set_duty(i, 0);
		MCHP_PCR_SLP_DIS_DEV(MCHP_PCR_TACH0);
		MCHP_TACH_CTRL(i) = MCHP_TACH_CTRL_MODE_SELECT +
							MCHP_TACH_CTRL_ENABLE +
							MCHP_TACH_CTRL_FILTER_EN +
							MCHP_TACH_CTRL_TACH_EDGES_9;
	}
}


