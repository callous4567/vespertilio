#include "../mSD/mSD.h"
#include "../ext_rtc/ext_rtc.h"



/*
The process is as follows: Left is CORE 0, righjt is CORE 1 

ADC -> BUF_A | BUF_B -> MSD 
BUF_A -> MSD | ADC -> BUF_B 
ADC -> BUF_A | BUF_B -> MSD 
BUF_A -> MSD | ADC -> BUF_B 

Paced by Core 0 the cycles will start as on the right:
- Run BUF_B->MSD 
- Run ADC->BUF_B 
- Reset with while(1) loop 
The second bit (ADC->BUF_B) won't start without the multicore fifo getting an entry to do it, so yeah. 
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

    // BME pointers, too 
    char* BME_DATASTRING;    // bme 20-byte datastring with humidity_pressure_temperature in RH%_pascal_celsius 
    char* BME_AND_TIME_STRING; // 20 byte BME_DATASTRING + the 22 byte EXT_RTC fullstring + a 2-byte "_" spacer + a 1-byte \n newline.
    bool* BME_SHOULD_CONTINUE;
    bool* BME_SLEEPING; 
    char* BME_STRINGBUFFER;
    int32_t* BME_BUFFER_SIZE; 

} recording_multicore_struct_single_t; 


// run the sequence. initialize this at the time the recordings should start. I recommend starting the recordings 20-30 minutes beforehand to allow all hardware to equalize/self-heat.
void run_wav_bme_sequence_single();