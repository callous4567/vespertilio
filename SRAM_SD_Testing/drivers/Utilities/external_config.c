#include "external_config.h"
#include <malloc.h>

/*

We have two types of variable

- Independent
- Dependent

The former is defined, i.e., above: ADC_SAMPLE_RATE=192,000
We can redefine these whenever we want

When we do, we need to recalculate the dependent variables, however.

- Recalculate dependent variables after redefining independent variables 

We need to read these independent variables from the configuration flash. 

*/

// INDEPENDENT VARIABLES (retrieve from host)
int32_t ADC_SAMPLE_RATE = 480000;
int32_t RECORDING_LENGTH_SECONDS = 10; // note that BME files are matched to this recording length, too. 
int32_t RECORDING_SESSION_MINUTES = 5; 
bool USE_BME = false;
int32_t BME_RECORD_PERIOD_SECONDS = 1;

/*

// INDEPENDENT TIME VARIABLES (retrieve from host) USED BY RTC 
int32_t SECOND;
int32_t MINUTE;
int32_t HOUR;
int32_t DOTW;
int32_t DOTM;
int32_t MONTH;
int32_t YEAR;
int32_t ALARM_MIN; // by default set alarm second/dotw to 0/1 and have the alarm go off every single day.
int32_t ALARM_HOUR;
*/

// DEPENDENT VARIABLES (calculated from independent) 
int32_t RECORDING_FILE_DATA_RATE_BYTES;
int32_t RECORDING_FILE_DATA_SIZE; // total data chunk size in bytes is time * bits-per-second / bytes 
int32_t RECORDING_NUMBER_OF_CYCLES; // assuming one cycle is two full buffers written + each sample is 2 bytes.
int32_t RECORDING_NUMBER_OF_FILES; // number of seconds total / seconds per recorded file 
int32_t BME_RECORD_PERIOD_CYCLES;

// HARDWARE/CONSTANT VARIABLES
const int32_t ADC_PIN = 26;
const int32_t ADC_BUF_SIZE = 3072; // two buffers of this size IN SAMPLES. 2048 -> 2048*16 = 32768 bits/32kbits. 


// set all the functional variables based on the present non-functional variable configuration: call this after redefining, i.e., ADC_SAMPLE_RATE.
static void set_dependent_variables(void) {

    // Set the variables that need to be modified 
    RECORDING_FILE_DATA_RATE_BYTES = ADC_SAMPLE_RATE*2;
    RECORDING_FILE_DATA_SIZE = RECORDING_LENGTH_SECONDS * RECORDING_FILE_DATA_RATE_BYTES; // total data chunk size in bytes is time * bits-per-second / bytes 
    RECORDING_NUMBER_OF_CYCLES = RECORDING_FILE_DATA_SIZE/(ADC_BUF_SIZE*4); // assuming one cycle is two full buffers written + each sample is 2 bytes.
    RECORDING_NUMBER_OF_FILES = RECORDING_SESSION_MINUTES*60/RECORDING_LENGTH_SECONDS; // number of seconds total / seconds per recorded file 

    // Consts specific to using the BME280. We have configured to update the internal measure every second. Larger period = better audio timing. 
    BME_RECORD_PERIOD_CYCLES = BME_RECORD_PERIOD_SECONDS*ADC_SAMPLE_RATE/(ADC_BUF_SIZE*2);

}

// set independent variables from the provided configuration buffer
static void set_independent_variables(int32_t* configuration_buffer) {

    // INDEPENDENT VARIABLES (retrieve from host)
    ADC_SAMPLE_RATE = *configuration_buffer;
    RECORDING_LENGTH_SECONDS = *(configuration_buffer+1); // note that BME files are matched to this recording length, too. 
    RECORDING_SESSION_MINUTES = *(configuration_buffer+2); 
    USE_BME = (bool)*(configuration_buffer+3);
    BME_RECORD_PERIOD_SECONDS = *(configuration_buffer+4);

}

// configurate from the flash 
void flash_configurate_variables(void) {

    int32_t* configuration_buffer = read_from_flash();
    set_independent_variables(configuration_buffer);
    set_dependent_variables();
    free(configuration_buffer);

}