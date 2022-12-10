#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "rtc.h"
#include "hw_config.h"
extern "C" {
#include "drivers/SRAM/IS62_SRAM.h"
}
extern "C" {
#include "drivers/Utilities/utils.h"
}
extern "C" {
#include "drivers/mSD/mSD.h"
}
extern "C" {
#include "drivers/ext_rtc/ext_rtc.h"
}
#include "malloc.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "pico/multicore.h"

// set up the dma channels appropriately, setting the initial write address for SRAM_BUFA and ADC_BUFB appropriately. note that input should be malloc pointers. 
void init_dma(
    int *ADC_BUFA_CHAN,
    dma_channel_config *ADC_BUFA_CONF,
    uint16_t *BUF_A, 
    int *ADC_BUFB_CHAN,
    dma_channel_config *ADC_BUFB_CONF,
    uint16_t *BUF_B,
    int *BUF_SIZE) {

    // Configure DMA channel from ADC to buffer A
    *ADC_BUFA_CHAN = dma_claim_unused_channel(true);
    *ADC_BUFA_CONF = dma_channel_get_default_config(*ADC_BUFA_CHAN);
    channel_config_set_transfer_data_size(ADC_BUFA_CONF, DMA_SIZE_16);
    channel_config_set_read_increment(ADC_BUFA_CONF, false);
    channel_config_set_write_increment(ADC_BUFA_CONF, true);
    channel_config_set_dreq(ADC_BUFA_CONF, DREQ_ADC);
    dma_channel_configure(
        *ADC_BUFA_CHAN,
        ADC_BUFA_CONF,
        BUF_A,
        &adc_hw->fifo,
        *BUF_SIZE, 
        false
    );

    // Configure DMA channel from ADC to buffer B
    *ADC_BUFB_CHAN = dma_claim_unused_channel(true);
    *ADC_BUFB_CONF = dma_channel_get_default_config(*ADC_BUFB_CHAN);
    channel_config_set_transfer_data_size(ADC_BUFB_CONF, DMA_SIZE_16);
    channel_config_set_read_increment(ADC_BUFB_CONF, false);
    channel_config_set_write_increment(ADC_BUFB_CONF, true);
    channel_config_set_dreq(ADC_BUFB_CONF, DREQ_ADC);
    dma_channel_configure(
        *ADC_BUFB_CHAN,
        ADC_BUFB_CONF,
        BUF_B,
        &adc_hw->fifo,
        *BUF_SIZE, 
        false
    );

}

// Write BUF_? to the SD card appropriately (one call will handle it all.)
void write_BUF__SD(
    uint16_t *BUF_,
    int *BUF_SIZE,
    FIL *fp,
    UINT *bw
) {

    // use f_write to write BUF_A. note that BUF_A is uint16_t and hence bytes written must be double the buf size. 
    f_write(
        fp,
        BUF_,
        *BUF_SIZE*2,
        bw
    );

    if (*bw!=(*BUF_SIZE * 2)) {
        panic("The bytes written does not equal double the uint16_t buffer size in write_BUF_A_SD in SRAM_SD_Testing.cpp. FUCK! %u, %u", *bw, 2*(*BUF_SIZE));
    }

}
// Initialize ADC->BUF__ (pass the appropriate buffer and buffer channel.)
void ADC_to_BUF__(
    uint16_t *BUF__,
    int *ADC_BUF__CHAN
) {
    adc_fifo_drain();
    dma_channel_start(*ADC_BUF__CHAN);
    dma_channel_wait_for_finish_blocking(*ADC_BUF__CHAN);
    dma_channel_set_write_addr(*ADC_BUF__CHAN, BUF__, false);
}

// Make a struct to contain all the variables for multicore (seems easiest.)
typedef struct {

    // Pointers to the necessities for core1 to execute. Pass this as argument.
    uint16_t *BUF__;
    int *BUF_SIZE;
    FIL *fp;
    UINT *bw;
    int *ADC_BUF__CHAN;
    void (*writebufSD_ptr)(uint16_t*, int*, FIL*, UINT*);
    void (*adctobuf_ptr)(uint16_t*, int*);

} core1_multicore_pointerstruct; 


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


void core1_test_cycle() {

    // Grab the pointer for the functions/etc. 
    core1_multicore_pointerstruct* CORE1_DATASTRUCT = (core1_multicore_pointerstruct*)multicore_fifo_pop_blocking();
    printf(
        "TEST TEST! Hello?... %d %d", 
        *CORE1_DATASTRUCT->ADC_BUF__CHAN,
        *CORE1_DATASTRUCT->BUF_SIZE
        );

    // will keep going in perpetuity or until reset. paced by core 0. 
    while (1) {

        // wait for the token to continue (will block until it can.)
        multicore_fifo_pop_blocking();

        // first do BUF_B to MSD write 
        (*CORE1_DATASTRUCT->writebufSD_ptr)(
            CORE1_DATASTRUCT->BUF__, 
            CORE1_DATASTRUCT->BUF_SIZE, 
            CORE1_DATASTRUCT->fp, 
            CORE1_DATASTRUCT->bw
            );

        // wait for the token to continue (will block until it can.)
        multicore_fifo_pop_blocking();

        // now do ADC->BUF_B process (blocks until the fifo has something ready to pop I assume.)
        (*CORE1_DATASTRUCT->adctobuf_ptr)(
            CORE1_DATASTRUCT->BUF__,
            CORE1_DATASTRUCT->ADC_BUF__CHAN
            );

        // All done! :D 

    }

}



void test_cycle(void) {

    // Next set up the SD card as usual... 
    SD_pin_configure();
    SD_ON();
    sleep_ms(1000);

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html for information. 

    // Get the SD object 
    sd_card_t *pSD = sd_get_by_num(0);
    
    // Mount the volume corresponding to the object 
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1); 
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr); 

    // Check if file exists 
    FRESULT FR;
    FILINFO FNO;
    
    // Specify filename 
    const char* test_filename = "data_roundrobin_test.txt";

    // Check if the file exists 
    bool exists = SD_IS_EXIST(test_filename);
    if (exists) {
        f_unlink(test_filename);
        printf("The file %s exists- deleting.\r\n", test_filename);
    } else {
        printf("The file %s doesn't exist! We will create it instead.\r\n", test_filename);
    }
    

    // Create file object structure (blank one) for the newly made file 
    FIL fil;

    // See http://elm-chan.org/fsw/ff/doc/open.html 
    fr = f_open(&fil, test_filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", test_filename, FRESULT_str(fr), fr);

    // Set up pointers for malloc buffers LENGTHS (integer!)
    int* BUF_SIZE; // longboi
    BUF_SIZE = (int*)malloc(1*sizeof(int));
    *BUF_SIZE = 2048; // access value numerically by putting a * in front of it to dereference it.

    // Set up the actual buffers! 
    uint16_t* BUF_A; 
    uint16_t* BUF_B;
    BUF_A = (uint16_t*)malloc(*BUF_SIZE*sizeof(uint16_t)); 
    BUF_B = (uint16_t*)malloc(*BUF_SIZE*sizeof(uint16_t)); 

    // Set up ADC Fifo (free-running mode!) (one transfer is one sample is two bytes.) 
    setup_adc();
    adc_run(true);


    // Malloc for the channels 
    int *ADC_BUFA_CHAN = (int*)malloc(sizeof(int)); 
    int *ADC_BUFB_CHAN = (int*)malloc(sizeof(int)); 
    dma_channel_config *ADC_BUFA_CONF = (dma_channel_config*)malloc(sizeof(dma_channel_config)); 
    dma_channel_config *ADC_BUFB_CONF = (dma_channel_config*)malloc(sizeof(dma_channel_config)); 

    init_dma(
        ADC_BUFA_CHAN,  
        ADC_BUFA_CONF, 
        BUF_A, 
        ADC_BUFB_CHAN,  
        ADC_BUFB_CONF, 
        BUF_B,
        BUF_SIZE
        );

    // Saturate buffer A.
    ADC_to_BUF__(BUF_A, ADC_BUFA_CHAN);

    // Just a bytes-written pointer for each dma channel
    UINT* BUF_A_BW; 
    UINT* BUF_B_BW;
    BUF_A_BW = (UINT*)malloc(sizeof(UINT));
    BUF_B_BW = (UINT*)malloc(sizeof(UINT));

    // Set up the handler for core1 
    core1_multicore_pointerstruct *CORE1_DATASTRUCT; 
    CORE1_DATASTRUCT = (core1_multicore_pointerstruct*)malloc(sizeof(core1_multicore_pointerstruct));
    
    CORE1_DATASTRUCT->BUF__=BUF_B;
    CORE1_DATASTRUCT->BUF_SIZE=BUF_SIZE;
    CORE1_DATASTRUCT->fp=&fil;
    CORE1_DATASTRUCT->bw=BUF_B_BW;
    CORE1_DATASTRUCT->ADC_BUF__CHAN=ADC_BUFB_CHAN;

    // See https://stackoverflow.com/questions/840501/how-do-function-pointers-in-c-work 
    CORE1_DATASTRUCT->writebufSD_ptr = &write_BUF__SD;
    CORE1_DATASTRUCT->adctobuf_ptr = &ADC_to_BUF__;
    
    // Alright do a quick little test.
    multicore_launch_core1(core1_test_cycle);
    multicore_fifo_push_blocking((uintptr_t)CORE1_DATASTRUCT);
    multicore_fifo_push_blocking((uint32_t)1);
    multicore_fifo_push_blocking((uint32_t)1);
    for (int i = 0; i < 10; i++) {
        printf("%d\r\n", BUF_B[i]);
    }



    // All done. 
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount(pSD->pcName);
    for (;;);

}


// Run a test of the SD functionality 
int main() {

    

    stdio_init_all();
    ext_rtc_t* EXT_RTC = rtc_debug();
    rtc_sleep_until_alarm(EXT_RTC);
    //time_init();
    //test_cycle();
    //SD_ADC_DMA_testwrite();
    printf("All done! Bye bye!"); 

}
