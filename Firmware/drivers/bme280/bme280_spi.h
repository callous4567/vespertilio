#ifndef BME280_DRIVER
#define BME280_DRIVER
/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../Utilities/universal_includes.h"

// test cycles + print. if this fails, the BME has not been set up.
void test_bme(void);

// set up the bme SPI + read compensation parameters + set the default registers. You must run this before any other BME functions. Needs appropriate mutex.
void setup_bme(mutex_t* EXT_RTC_MUTEX);

// update the datastring provided with the bme data
void bme_datastring(char* datastring);

#endif // BME280_DRIVER