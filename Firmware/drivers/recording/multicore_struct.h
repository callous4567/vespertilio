#ifndef FATFS_CUSTOSTRUCT
#define FATFS_CUSTOSTRUCT 

#include "../mSD/mSD.h"
#include "../ext_rtc/ext_rtc.h"
#include "../veml/i2c_driver.h"
#include "../Utilities/pinout.h"
#include "hardware/dma.h"

/*

Note that for the environmental file, we have the TIME_BME_VEML format 
^_^

*/

// Make a struct to contain all the variables for multicore/recording (seems easiest.) All contents are Malloc'd except for the mSD->sd_card_t. Be very judicious about the malloc fact. 
typedef struct {

    // Pointer to the mSD object responsible for this session. mSD->pSD NOT IN MALLOC.
    mSD_struct_t *mSD;
    bool* active;

    // Pointer to the RTC object responsible for this session.
    ext_rtc_t *EXT_RTC;

    // Pointers for the buffers & all the channel information/etc. 
    int16_t* ADC_BUFA; 
    int16_t *ADC_BUFA_CHAN; 
    dma_channel_config *ADC_BUFA_CONF; 
    int8_t* ADC_WHICH_HALF; // tells us which half is CURRENTLY being written to by the DMA from the ADC, 0-based 

    // Environmental pointers too. Note that 
    char* BME_DATASTRING;    // bme 20-byte datastring with humidity_pressure_temperature in RH%_pascal_celsius 
    char* ENV_AND_TIME_STRING; // 20 byte BME_DATASTRING + two byte spacer _ + the 22 byte EXT_RTC fullstring + a 1-byte \n newline. Now with an extra 26 bytes (making 73, when you include a spacer) to include the VEML.
    bool* ENV_SHOULD_CONTINUE;
    bool* ENV_SLEEPING; 
    char* ENV_STRINGBUFFER; // the buffer of size ENV_BUFFER_SIZE that holds *all* the environmental measurements for each recording 
    int32_t* ENV_BUFFER_SIZE; 
    veml_t* VEML; // our VEML object

} recording_multicore_struct_single_t; 

#endif // FATFS_CUSTOSTRUCT 