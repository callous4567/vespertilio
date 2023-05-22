#include "external_config.h"
#include "custoclocks.h"
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
int32_t ADC_SAMPLE_RATE = 192000;
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
int32_t INTERBLOCK_SLEEP_TIME_US; 
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

// calculate the "interblock sleep time" for SD card writes (@480 ksps, 0.5*sample time = fine for processing. Sleep sample time - this.
static void set_interblock_sleep_time(void) {

    // note that one block is 512 bytes, which is 256 samples. The time to thusly sample one block is 256/ADC_SAMPLE_RATE.
    int32_t processing_time_us = (25 * (512 * 1000000 / 480000)) / 100;
    int32_t sample_time_us = (512 * 1000000) / ADC_SAMPLE_RATE;
    int32_t INTERBLOCK_SLEEP_TIME_US = sample_time_us - processing_time_us;

}

// Set dependent variables (run after setting independent variables.) Do this every session within the ensemble.
void set_dependent_variables(int32_t WHICH_ALARM_ONEBASED) {

    // Set the variables that need to be modified 
    RECORDING_FILE_DATA_RATE_BYTES = ADC_SAMPLE_RATE*2;
    RECORDING_FILE_DATA_SIZE = RECORDING_LENGTH_SECONDS * RECORDING_FILE_DATA_RATE_BYTES; // total data chunk size in bytes is time * bits-per-second / bytes 
    RECORDING_SESSION_MINUTES = *(configuration_buffer_external + CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + (3*WHICH_ALARM_ONEBASED));
    RECORDING_NUMBER_OF_FILES = (60*RECORDING_SESSION_MINUTES)/RECORDING_LENGTH_SECONDS;
    ENV_BUFFER_SIZE = TIME_VEML_BME_STRINGSIZE*((RECORDING_LENGTH_SECONDS/ENV_RECORD_PERIOD_SECONDS) + 5); // (bytesize of !env!timestring) * (number of BME datapoints per recording + a tolerance) // bmetimestring = 45, veml is 26, hence TIME_VEML_BME_STRINGSIZE is total if you include another "_" spacer (two bytes). 
    set_interblock_sleep_time();
}

/* 
Set the independent variables for the current ensemble of recordings from the configuration_buffer_external malloc.
Note that this will also call set_optimal_clock() from custoclocks.h. 
This sets the clock appropriately, initializes stdio, *and* initializes the flashlog.
This should be called outside the loop over WHICH_ALARM_ONEBASED.
*/
static void set_constant_independent_variables(void) {

    // Constant independent variables for this program
    ADC_SAMPLE_RATE = *configuration_buffer_external;
    set_optimal_clock();
    NUMBER_OF_SESSIONS = *(configuration_buffer_external + CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7);
    RECORDING_LENGTH_SECONDS = *(configuration_buffer_external+1); // note that BME files are matched to this recording length, too. 
    USE_ENV = (bool)*(configuration_buffer_external+2);
    ENV_RECORD_PERIOD_SECONDS = *(configuration_buffer_external+3);

}

/* 
Set the independent variables for the session for debugging purposes- will also set dependent variables.
Note that this will also call set_optimal_clock() from custoclocks.h. 
This sets the clock appropriately, initializes stdio, *and* initializes the flashlog.
*/
void default_variables(void) {

    ADC_SAMPLE_RATE = 192000;
    set_optimal_clock();
    RECORDING_LENGTH_SECONDS = 300;
    USE_ENV = false;
    ENV_RECORD_PERIOD_SECONDS = 10;
    RECORDING_FILE_DATA_RATE_BYTES = ADC_SAMPLE_RATE*2;
    RECORDING_FILE_DATA_SIZE = RECORDING_LENGTH_SECONDS * RECORDING_FILE_DATA_RATE_BYTES; // total data chunk size in bytes is time * bits-per-second / bytes 
    RECORDING_SESSION_MINUTES = 3000;
    RECORDING_NUMBER_OF_FILES = (60*RECORDING_SESSION_MINUTES)/RECORDING_LENGTH_SECONDS;
    ENV_BUFFER_SIZE = TIME_VEML_BME_STRINGSIZE*((RECORDING_LENGTH_SECONDS/ENV_RECORD_PERIOD_SECONDS) + 5); // (bytesize of bmetimestring) * (number of BME datapoints per recording + a tolerance) 
    NUMBER_OF_SESSIONS = 1;
    set_interblock_sleep_time();
}


// retrieve configuration from flash for this particular session
void flash_read_to_configuration_buffer_external(void) {

    configuration_buffer_external = read_from_flash();
    set_constant_independent_variables();

}