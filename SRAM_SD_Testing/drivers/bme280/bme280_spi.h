#ifndef BME280_DRIVER
#define BME280_DRIVER
/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

// test cycles + print. if this fails, the BME has not been set up.
void test_bme(void);

// set up the bme SPI + read compensation parameters + set the default registers. You must run this before any other BME functions. 
void setup_bme(void);

// update the datastring provided with the bme data
void bme_datastring(char* datastring);

#endif // BME280_DRIVER