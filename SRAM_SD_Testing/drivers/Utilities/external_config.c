#include "external_config.h"
#include <malloc.h>
#include "../pico_usb_configure/vespertilio_usb_int.h"

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

// INDEPENDENT VARIABLES (set to some examples.)
int32_t ADC_SAMPLE_RATE = 480000;
int32_t RECORDING_LENGTH_SECONDS = 10; // note that BME files are matched to this recording length, too. 
int32_t RECORDING_SESSION_MINUTES = 5; // this is set from the configuration_buffer_external each and every session
bool USE_BME = false;
int32_t BME_RECORD_PERIOD_SECONDS = 1;
int32_t NUMBER_OF_ALARMS;

// DEPENDENT VARIABLES (calculated from independent, thus undefined as of yet.)
int32_t RECORDING_FILE_DATA_RATE_BYTES;
int32_t RECORDING_FILE_DATA_SIZE; // total data chunk size in bytes is time * bits-per-second / bytes 
int32_t RECORDING_NUMBER_OF_FILES;
int32_t BME_BUFFER_SIZE; // size of the BME data buffer, where applicable 

// HARDWARE/CONSTANT VARIABLES
const int32_t ADC_PIN = 26;

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
    BME_BUFFER_SIZE = 45*((RECORDING_LENGTH_SECONDS/BME_RECORD_PERIOD_SECONDS) + 5); // (bytesize of bmetimestring) * (number of BME datapoints per recording + a tolerance)  

}

/**
 * Sets all permanent constants for this recording day (not session) including the NUMBER_OF_ALARMS
 * Call at start of recording day (not session.)
*/
static void set_constant_independent_variables(void) {

    // Constant independent variables for this program
    NUMBER_OF_ALARMS = *(configuration_buffer_external + CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7);
    ADC_SAMPLE_RATE = *configuration_buffer_external;
    RECORDING_LENGTH_SECONDS = *(configuration_buffer_external+1); // note that BME files are matched to this recording length, too. 
    USE_BME = (bool)*(configuration_buffer_external+2);
    BME_RECORD_PERIOD_SECONDS = *(configuration_buffer_external+3);

}

/**
 * Recover configuration buffer from flash and set it to SRAM configuration_buffer_external
 * Call at start of recording day (not session.)
*/
void flash_read_to_configuration_buffer_external(void) {

    configuration_buffer_external = read_from_flash();
    set_constant_independent_variables();

}