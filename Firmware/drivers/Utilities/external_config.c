#include "external_config.h"
#include <malloc.h>
#include "../pico_usb_configure/vespertilio_usb_int.h"
#include "pinout.h"

/*

We have two types of variable:

- Independent, i.e. ripped from the flash 
- Dependent, i.e. set from the above. 

When we update the independent, we need to recalculate the dependent variables.

- Recalculate dependent variables after redefining independent variables 

We need to read these independent variables from the configuration flash. 

*/

// the flash buffer, read into SRAM 
int32_t* configuration_buffer_external; // malloc

// FOUR INDEPENDENT VARIABLES NON-TIME-RELATED!
int32_t ADC_SAMPLE_RATE = 384000;
int32_t RECORDING_LENGTH_SECONDS = 30; // note that BME files are matched to this recording length, too. 
bool USE_ENV = true; // note that we fudge to require the BME and the VEML at once to use Environmental. Do a fix later (make the BME or VEML return zeros if not present to the .txt)
int32_t ENV_RECORD_PERIOD_SECONDS = 5;

// The !current! session settings + number of alarms. 
int32_t RECORDING_SESSION_MINUTES = 5; // this is set from the configuration_buffer_external each and every session
int32_t NUMBER_OF_SESSIONS;

// DEPENDENT VARIABLES (calculated from independent, thus undefined as of yet.)
int32_t RECORDING_FILE_DATA_RATE_BYTES;
int32_t RECORDING_FILE_DATA_SIZE; // total data chunk size in bytes is time * bits-per-second / bytes 
int32_t RECORDING_NUMBER_OF_FILES;
const int32_t TIME_VEML_BME_STRINGSIZE = 76;
/*
const int32_t TIME_VEML_BME_STRINGSIZE
22 BYTES EXT_RTC FULLSTIRNG 
2 BYTES _
20 BYTES BME_DATASTRING 
2 BYTES _
29 BYTES VEML_DATASTRING 
1 BYTES /n NEWLINE 
76 BYTES TOTAL 
*/

int32_t ENV_BUFFER_SIZE; // size of the ENV data buffer, where applicable 

/**
 * Set all the dependent variables. 
 * Call this at the start of every recording session, as RECORDING_SESSION_MINUTES changes.
 * 
*/
void set_dependent_variables(int32_t WHICH_ALARM_ONEBASED) {

    // Set the variables that need to be modified 
    RECORDING_FILE_DATA_RATE_BYTES = ADC_SAMPLE_RATE*2;
    RECORDING_FILE_DATA_SIZE = RECORDING_LENGTH_SECONDS * RECORDING_FILE_DATA_RATE_BYTES; // total data chunk size in bytes is time * bits-per-second / bytes 
    RECORDING_SESSION_MINUTES = *(configuration_buffer_external + CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + (3*WHICH_ALARM_ONEBASED));
    RECORDING_NUMBER_OF_FILES = (60*RECORDING_SESSION_MINUTES)/RECORDING_LENGTH_SECONDS;
    ENV_BUFFER_SIZE = TIME_VEML_BME_STRINGSIZE*((RECORDING_LENGTH_SECONDS/ENV_RECORD_PERIOD_SECONDS) + 5); // (bytesize of !env!timestring) * (number of BME datapoints per recording + a tolerance) // bmetimestring = 45, veml is 26, hence TIME_VEML_BME_STRINGSIZE is total if you include another "_" spacer (two bytes). 

}

/**
 * Sets all permanent constants for this recording day (not session) including the NUMBER_OF_SESSIONS
 * Call at start of recording day (not session.)
*/
static void set_constant_independent_variables(void) {

    // Constant independent variables for this program
    NUMBER_OF_SESSIONS = *(configuration_buffer_external + CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7);
    ADC_SAMPLE_RATE = *configuration_buffer_external;
    RECORDING_LENGTH_SECONDS = *(configuration_buffer_external+1); // note that BME files are matched to this recording length, too. 
    USE_ENV = (bool)*(configuration_buffer_external+2);
    ENV_RECORD_PERIOD_SECONDS = *(configuration_buffer_external+3);

}

// for debugging purposes. Important note: microSD will fill VERY FUCKING FAST @ 192kHz/384kHz/480kHz.
void default_variables(void) {

    ADC_SAMPLE_RATE = 384000;
    RECORDING_LENGTH_SECONDS = 30;
    USE_ENV = true;
    ENV_RECORD_PERIOD_SECONDS = 2.5;
    RECORDING_FILE_DATA_RATE_BYTES = ADC_SAMPLE_RATE*2;
    RECORDING_FILE_DATA_SIZE = RECORDING_LENGTH_SECONDS * RECORDING_FILE_DATA_RATE_BYTES; // total data chunk size in bytes is time * bits-per-second / bytes 
    RECORDING_SESSION_MINUTES = 9000;
    RECORDING_NUMBER_OF_FILES = (60*RECORDING_SESSION_MINUTES)/RECORDING_LENGTH_SECONDS;
    ENV_BUFFER_SIZE = TIME_VEML_BME_STRINGSIZE*((RECORDING_LENGTH_SECONDS/ENV_RECORD_PERIOD_SECONDS) + 5); // (bytesize of bmetimestring) * (number of BME datapoints per recording + a tolerance)  

}


/**
 * Recover configuration buffer from flash and set it to SRAM configuration_buffer_external
 * Call at start of recording day (not session.)
*/
void flash_read_to_configuration_buffer_external(void) {

    configuration_buffer_external = read_from_flash();
    set_constant_independent_variables();

}