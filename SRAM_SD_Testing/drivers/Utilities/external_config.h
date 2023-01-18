// Header Guard
#ifndef EXT_CONFIG
#define EXT_CONFIG

/* Standard includes. */
#include "hardware/adc.h"
#include "../pico_usb_configure/vespertilio_usb_int.h"

/*
This file includes the includes for utils.h, alongside the various constants/variables we define through configuration.
*/
extern int32_t ADC_SAMPLE_RATE, RECORDING_LENGTH_SECONDS, RECORDING_FILE_DATA_RATE_BYTES, RECORDING_FILE_DATA_SIZE, RECORDING_NUMBER_OF_CYCLES, RECORDING_NUMBER_OF_FILES, BME_RECORD_PERIOD_SECONDS, BME_RECORD_PERIOD_CYCLES;
extern bool USE_BME;

// Define ADC pin and buffer size (non-configurated constant.)
const extern int32_t ADC_PIN, ADC_BUF_SIZE;

// set default configuration from the flash 
void flash_configurate_variables(void);

#endif // EXT_CONFIG