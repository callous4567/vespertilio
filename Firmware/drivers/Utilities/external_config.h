// Header Guard
#ifndef EXT_CONFIG
#define EXT_CONFIG

#include "universal_includes.h"

/*
This file includes the includes for utils.h, alongside the various constants/variables we define through configuration.
*/
extern int32_t ADC_SAMPLE_RATE, RECORDING_LENGTH_SECONDS, RECORDING_NUMBER_OF_FILES, 
RECORDING_FILE_DATA_RATE_BYTES, RECORDING_FILE_DATA_SIZE, ENV_RECORD_PERIOD_SECONDS, 
ENV_BUFFER_SIZE, NUMBER_OF_SESSIONS;
extern const int32_t TIME_VEML_BME_STRINGSIZE;
extern bool USE_ENV;
extern int32_t* configuration_buffer_external;
extern int32_t INTERBLOCK_SLEEP_TIME_US; 

// set configuration_buffer_external from the flash (free configuration_buffer_external later.) set independent unchanging variables. return NUMBER_OF_SESSIONS.
void flash_read_to_configuration_buffer_external(void);

// set the proper dependent variables for the provided WHICH_ALARM_ONEBASED (iterate from 1->NUMBER_OF_SESSIONS and then repeat back to unity.)
void set_dependent_variables(int32_t WHICH_ALARM_ONEBASED);

void default_variables(void);

#endif // EXT_CONFIG