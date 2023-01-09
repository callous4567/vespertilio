#include "../mSD/mSD.h"
#include "../ext_rtc/ext_rtc.h"
#include "../Utilities/utils.h"
#include "pico/multicore.h"
#include "../bme280/bme280_spi.h"
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

// Make a struct to contain all the variables for multicore/recording (seems easiest.) All contents are Malloc'd except for the mSD->sd_card_t 
typedef struct {

    // Pointer to the mSD object responsible for this session. mSD->pSD NOT IN MALLOC.
    mSD_struct_t *mSD;

    // Pointer to the RTC object responsible for this session
    ext_rtc_t *EXT_RTC;

    // Pointers to the multicore functions for audio gathering
    void (*writebufSD_ptr)(int16_t*, int*, FIL*, UINT*);
    void (*waittobuf_ptr)(int16_t*, int*);
    void (*adctobuf_ptr)(int16_t*, int*);

    // Pointers for the buffers & all the channel information/etc 
    int* BUF_SIZE;
    uint16_t* BUF_A; 
    uint16_t* BUF_B;
    int *ADC_BUFA_CHAN; 
    int *ADC_BUFB_CHAN; 
    dma_channel_config *ADC_BUFA_CONF;
    dma_channel_config *ADC_BUFB_CONF; 
    UINT* BUF_A_BW; 
    UINT* BUF_B_BW;

    // BME pointers, too 
    char* BME_DATASTRING;    // bme 20-byte datastring with humidity_pressure_temperature in RH%_pascal_celsius 
    char* BME_AND_TIME_STRING; // 20 byte BME_DATASTRING + the 22 byte EXT_RTC fullstring + a 2-byte "_" spacer + a 1-byte \n newline.


} recording_multicore_struct_t; 


// set up the dma channels appropriately, setting the initial write address for SRAM_BUFA and ADC_BUFB appropriately. note that input should be malloc pointers. 
void init_dma(recording_multicore_struct_t* multicore_struct);

// Write BUF_? to the SD card appropriately (one call will handle it all.)
void write_BUF__SD(
    int16_t *BUF_,
    int *BUF_SIZE,
    FIL *fp,
    UINT *bw
); 

// Initialize ADC->BUF__ (pass the appropriate buffer and buffer channel.)
void ADC_to_BUF__(
    int16_t *BUF__,
    int *ADC_BUF__CHAN
);

// run the sequence. initialize this at the time the recordings should start. I recommend starting the recordings 20-30 minutes beforehand to allow all hardware to equalize/self-heat.
void run_wav_bme_sequence();
