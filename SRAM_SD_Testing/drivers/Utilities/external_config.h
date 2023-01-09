// Header Guard
#ifndef EXT_CONFIG
#define EXT_CONFIG

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "malloc.h"

/*
This file includes the includes for utils.h, alongside the various constants/variables we define through configuration.
*/
extern int32_t ADC_SAMPLE_RATE, RECORDING_LENGTH_SECONDS, RECORDING_FILE_DATA_RATE_BYTES, RECORDING_FILE_DATA_SIZE, RECORDING_NUMBER_OF_CYCLES, RECORDING_NUMBER_OF_FILES, BME_RECORD_PERIOD_CYCLES;
extern bool USE_BME;

// Define ADC pin and buffer size (non-configurated constant.)
const extern int32_t ADC_PIN, ADC_BUF_SIZE;

// set default configuration for various variables 
void set_default_configuration(void);




#endif // EXT_CONFIG