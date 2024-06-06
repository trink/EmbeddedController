/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* F75303 temperature sensor module for Chrome EC */

#ifndef __CROS_EC_F75303_H
#define __CROS_EC_F75303_H
#ifndef F75303_I2C_ADDR_FLAGS
#ifdef BOARD_MUSHU
#define F75303_I2C_ADDR_FLAGS		0x4D
#else
#define F75303_I2C_ADDR_FLAGS		0x4C
#endif
#endif

enum f75303_index {
	F75303_IDX_LOCAL,
	F75303_IDX_REMOTE1,
	F75303_IDX_REMOTE2,
	F75303_IDX_COUNT,
};
/* F75303 register */
#define F75303_TEMP_LOCAL_REGISTER 0x00
#define F75303_TEMP_LOCAL_LOW_REGISTER 0x29
#define F75303_TEMP_REMOTE1_REGISTER 0x01
#define F75303_TEMP_REMOTE1_LOW_REGISTER 0x10
#define F75303_TEMP_REMOTE2_REGISTER 0x23
#define F75303_TEMP_REMOTE2_LOW_REGISTER 0x24

/**
 * Get the last polled value of a sensor.
 *
 * @param idx	Index to read. Idx indicates whether to read die
 *		temperature or external temperature.
 * @param temp	Destination for temperature in K.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */
int f75303_get_val(int idx, int *temp);

/**
 * Get the last polled value of a sensor.
 *
 * @param idx		Index to read, from board's enum f75303_sensor
 *			definition
 *
 * @param temp_k_ptr	Destination for temperature in K.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */
int f75303_get_val_k(int idx, int *temp_k_ptr);

/**
 * Get the last polled value of a sensor.
 *
 * @param idx		Index to read, from board's enum f75303_sensor
 *			definition
 *
 * @param temp_mk_ptr	Destination for temperature in mK.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */
int f75303_get_val_mk(int idx, int *temp_mk_ptr);

/**
 * Set if the underlying polling task will read the sensor
 * or if it will skip, as the rail this sensor is on
 * may sometimes be powered off
 *
 * @param enabled	Set if the sensor should be polled or skipped
 */
void f75303_set_enabled(uint8_t enabled);

#endif  /* __CROS_EC_F75303_H */
