#include "recording.h"
#include "../mSD/mSD.h"
#include "../ext_rtc/ext_rtc.h"
#include "../Utilities/utils.h"
#include "../Utilities/external_config.h"
#include "pico/multicore.h"
#include "../bme280/bme280_spi.h"

/*
Some important information regarding .wav files
Data should be stored as int16_t data
Since uint16_t is unsigned, but 16-bit PCM requires signed data.
So int16_t.
*/

// Set up the clock divisor: check documentation to convince yourself we did this right. 
static void adc_clock_divisor(void) {
    int divider = (48000000 - ADC_SAMPLE_RATE)/ADC_SAMPLE_RATE;
    adc_set_clkdiv(divider);
}

// set up ADC pins/etc + run in free-running-mode 
static void setup_adc(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_PIN - 26); // select input from appropriate input
    adc_clock_divisor();
    adc_fifo_setup(true, true, 1, false, false);
    adc_run(true);
}

// set up the dma channels appropriately, setting the initial write address for SRAM_BUFA and ADC_BUFB appropriately. note that input should be malloc pointers. 
static void init_dma(recording_multicore_struct_t* multicore_struct) {

    // Configure DMA channel from ADC to buffer A
    *multicore_struct->ADC_BUFA_CHAN = dma_claim_unused_channel(true);
    *multicore_struct->ADC_BUFA_CONF = dma_channel_get_default_config(*multicore_struct->ADC_BUFA_CHAN);
    channel_config_set_transfer_data_size(multicore_struct->ADC_BUFA_CONF, DMA_SIZE_16);
    channel_config_set_read_increment(multicore_struct->ADC_BUFA_CONF, false);
    channel_config_set_write_increment(multicore_struct->ADC_BUFA_CONF, true);
    channel_config_set_dreq(multicore_struct->ADC_BUFA_CONF, DREQ_ADC);
    dma_channel_configure(
        *multicore_struct->ADC_BUFA_CHAN,
        multicore_struct->ADC_BUFA_CONF,
        multicore_struct->BUF_A,
        &adc_hw->fifo,
        ADC_BUF_SIZE, 
        false
    );

    // Configure DMA channel from ADC to buffer B
    *multicore_struct->ADC_BUFB_CHAN = dma_claim_unused_channel(true);
    *multicore_struct->ADC_BUFB_CONF = dma_channel_get_default_config(*multicore_struct->ADC_BUFB_CHAN);
    channel_config_set_transfer_data_size(multicore_struct->ADC_BUFB_CONF, DMA_SIZE_16);
    channel_config_set_read_increment(multicore_struct->ADC_BUFB_CONF, false);
    channel_config_set_write_increment(multicore_struct->ADC_BUFB_CONF, true);
    channel_config_set_dreq(multicore_struct->ADC_BUFB_CONF, DREQ_ADC);
    dma_channel_configure(
        *multicore_struct->ADC_BUFB_CHAN,
        multicore_struct->ADC_BUFB_CONF,
        multicore_struct->BUF_B,
        &adc_hw->fifo,
        ADC_BUF_SIZE, 
        false
    );

}

// Write BUF_? to the SD card appropriately (one call will handle it all.)
static void write_BUF__SD(
    int16_t *BUF_,
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

// Wait until ADC_TO_BUF__ is done and then reset the write address 
static void WAIT_ADC_to_BUF__(
    int16_t *BUF__,
    int *ADC_BUF__CHAN
) {
    dma_channel_wait_for_finish_blocking(*ADC_BUF__CHAN);
    dma_channel_set_write_addr(*ADC_BUF__CHAN, BUF__, false);
}

/*
Init ADC to BUF.
Note that this does not wait or reset the write address.
The correct procedure is to:
- Call ADC_to_BUF
- Run any code you want to run (that takes less time than ADC_to_BUF takes to populate BUF)
- Call WAIT_ADC_to_BUF, which will wait + then reset write address, before continuing. 
*/
static void ADC_to_BUF__(
    int16_t *BUF__,
    int *ADC_BUF__CHAN
) {
    adc_fifo_drain();
    dma_channel_start(*ADC_BUF__CHAN);
}

// Test-run a recording
static void core1_test_cycles() {

    // Grab the pointer for the functions/etc. 
    recording_multicore_struct_t* multicore_struct = (recording_multicore_struct_t*)multicore_fifo_pop_blocking();

    // will keep going paced by core 0 until a reset is called by core 0.
    while (1) {

        // wait for core 0 to finish populating buffer A. 
        multicore_fifo_pop_blocking();

        // now do ADC->BUF_B process. this isn't blocking 
        (*multicore_struct->adctobuf_ptr)(
            multicore_struct->BUF_B,
            multicore_struct->ADC_BUFB_CHAN
            );

        // while ADC->BUF_B runs, execute the code for waiting + resetting the pointer
        (*multicore_struct->waittobuf_ptr)(
            multicore_struct->BUF_B,
            multicore_struct->ADC_BUFB_CHAN
        );

        // ADC->BUF_B is done. tell core0 it is done.
        multicore_fifo_push_blocking((uint32_t)1);

        // first do BUF_B to MSD write 
        (*multicore_struct->writebufSD_ptr)(
            multicore_struct->BUF_B, 
            multicore_struct->BUF_SIZE, 
            multicore_struct->mSD->fp_audio, 
            multicore_struct->mSD->bw
            );

        // all done! wait on core0 to tell us we can start populating buffer B.

    }

}

// write the 44-byte WAV header required for the .wave format (do this after opening file and initiating recording.)
static void write_standard_wav_header(recording_multicore_struct_t *multicore_struct) {

    f_write(
        multicore_struct->mSD->fp_audio,
        "RIFF",
        4,
        multicore_struct->mSD->bw
    );

    int32_t an_int_thirtytwo;
    an_int_thirtytwo = 36 + RECORDING_FILE_DATA_SIZE; // the total file size 
    int32_t* an_int_thirtytwo_ptr = &an_int_thirtytwo;

    // the size of the overall file minus 8 bytes http://soundfile.sapp.org/doc/WaveFormat/
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_thirtytwo_ptr,
        4,
        multicore_struct->mSD->bw
    );

    // WAVE and fmt 
    f_write(
        multicore_struct->mSD->fp_audio,
        "WAVEfmt ",
        8,
        multicore_struct->mSD->bw
    );

    an_int_thirtytwo = 16; // the length of the format data above 

    // size of format data above 
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_thirtytwo_ptr,
        4,
        multicore_struct->mSD->bw
    );

    int16_t an_int_sixteen;
    an_int_sixteen = 1; // the type of format (1 is PCM)
    int16_t *an_int_sixteen_ptr = &an_int_sixteen;

    // type of format 
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_sixteen_ptr,
        2,
        multicore_struct->mSD->bw
    );

    an_int_sixteen = 1; // number of channels 

    // the number of channels 
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_sixteen_ptr,
        2,
        multicore_struct->mSD->bw
    );

    an_int_thirtytwo = ADC_SAMPLE_RATE; // the sample rate in hertz 

    // the sample rate in hertz
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_thirtytwo_ptr,
        4,
        multicore_struct->mSD->bw
    );

    an_int_thirtytwo = RECORDING_FILE_DATA_RATE_BYTES;

    // the file data rate
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_thirtytwo_ptr,
        4,
        multicore_struct->mSD->bw
    );

    an_int_sixteen = 2; // bitspersample * channels div byte 

    // bits per sample * channels / 8
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_sixteen_ptr,
        2,
        multicore_struct->mSD->bw
    );

    an_int_sixteen = 16; // bits per sample 

    // bits per sample 
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_sixteen_ptr,
        2,
        multicore_struct->mSD->bw
    );

    // data chunk header     
    f_write(
        multicore_struct->mSD->fp_audio,
        "data",
        4,
        multicore_struct->mSD->bw
    );

    an_int_thirtytwo = RECORDING_FILE_DATA_SIZE;

    // size of data section
    f_write(
        multicore_struct->mSD->fp_audio,
        an_int_thirtytwo_ptr,
        4,
        multicore_struct->mSD->bw
    );

    // That's the wav header done! :D 
}

// Generate a multicore struct for recording purely audio data
static recording_multicore_struct_t* audiostruct_generate(void) {

    // pointer up. 
    recording_multicore_struct_t* multicore_struct = (recording_multicore_struct_t*)malloc(sizeof(recording_multicore_struct_t));
    
    // Set up pointers of the multicore_struct
    multicore_struct->mSD = (mSD_struct_t*)malloc(sizeof(mSD_struct_t));
    multicore_struct->BUF_SIZE = (int*)malloc(sizeof(int));
    multicore_struct->BUF_A = (int16_t*)malloc(ADC_BUF_SIZE*sizeof(int16_t)); 
    multicore_struct->BUF_B = (int16_t*)malloc(ADC_BUF_SIZE*sizeof(int16_t)); 
    multicore_struct->ADC_BUFA_CHAN = (int*)malloc(sizeof(int)); 
    multicore_struct->ADC_BUFB_CHAN = (int*)malloc(sizeof(int)); 
    multicore_struct->ADC_BUFA_CONF = (dma_channel_config*)malloc(sizeof(dma_channel_config)); 
    multicore_struct->ADC_BUFB_CONF = (dma_channel_config*)malloc(sizeof(dma_channel_config)); 
    multicore_struct->BUF_A_BW = (UINT*)malloc(sizeof(UINT));
    multicore_struct->BUF_B_BW = (UINT*)malloc(sizeof(UINT));

    // Allocate all the pointers (except mSD)
    *multicore_struct->BUF_SIZE = ADC_BUF_SIZE;
    multicore_struct->writebufSD_ptr = &write_BUF__SD;
    multicore_struct->waittobuf_ptr = &WAIT_ADC_to_BUF__;
    multicore_struct->adctobuf_ptr = &ADC_to_BUF__;
    init_dma(multicore_struct);

    // Now allocate mSD pointers where appropriate 
    multicore_struct->mSD->pSD = sd_get_by_num(0); 
    multicore_struct->mSD->fp_audio = (FIL*)malloc(sizeof(FIL));
    multicore_struct->mSD->bw = (UINT*)malloc(sizeof(UINT));
    multicore_struct->mSD->fp_audio_filename = (char*)malloc(26); // 22 bytes for the time fullstring, then 4 bytes for .wav 

    // Set up pointers for the BME too (and init it.)
    if (USE_BME==true) {

        // Datastring/timestring/init 
        multicore_struct->BME_DATASTRING = (char*)malloc(20); // 20 bytes for the BME data 
        multicore_struct->BME_AND_TIME_STRING = (char*)malloc(45); // 20 bytes + 22 RTC bytes + 2 byte spacer + 1 byte newline 
    
        // The microSD pointers too.
        multicore_struct->mSD->fp_env = (FIL*)malloc(sizeof(FIL));
        multicore_struct->mSD->bw_env = (UINT*)malloc(sizeof(UINT));
        multicore_struct->mSD->fp_env_filename = (char*)malloc(30); // 22 bytes for the time fullstring, 4 for .env, 4 for .txt 

    }

    // Finally let's do the RTC 
    multicore_struct->EXT_RTC = (ext_rtc_t*)malloc(sizeof(ext_rtc_t));
    multicore_struct->EXT_RTC = rtc_debug();

    return multicore_struct;
}


// initialize the wav file for the current time taken from the RTC and open it for writing (writing the header.) 
static void init_wav_file(recording_multicore_struct_t* multicore_struct) {

    // Read the current time + get string 
    rtc_read_string_time(multicore_struct->EXT_RTC);

    // Generate a string with the time at the front and .wav on the end: fullstring is maximum of 22 bytes, .wav is 4 bytes. 
    snprintf(
        multicore_struct->mSD->fp_audio_filename,
        26,
        "%s.wav",
        multicore_struct->EXT_RTC->fullstring
    );

    // Check if file exists 
    FRESULT FR;
    FILINFO FNO;

    // Check if the file exists 
    bool exists = SD_IS_EXIST(multicore_struct->mSD->fp_audio_filename);
    if (exists) { // delete 
        f_unlink(multicore_struct->mSD->fp_audio_filename);
        printf("The file %s exists- deleting.\r\n", multicore_struct->mSD->fp_audio_filename);
    } 
    
    // Open the file with write access (FA_WRITE) with the read/write pointer at the start (FA_OPEN_ALWAYS)
    FRESULT fr = f_open(multicore_struct->mSD->fp_audio, multicore_struct->mSD->fp_audio_filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", multicore_struct->mSD->fp_audio_filename, FRESULT_str(fr), fr);

    // Write the wav header/etc 
    write_standard_wav_header(multicore_struct);

}

// initialize the BME text file for the current time taken from the RTC and open it for writing. Note that this assumes you have already gotten the EXT_RTC fullstring (you should have, for init_wav.)
static void init_bme_file(recording_multicore_struct_t* multicore_struct) {

    // Generate a string with the time at the front and .wav on the end: fullstring is maximum of 22 bytes, .env is 4 bytes, .txt is 4 bytes, making 30
    snprintf(
        multicore_struct->mSD->fp_env_filename,
        30,
        "%s.env.txt",
        multicore_struct->EXT_RTC->fullstring
    );

    // Check if file exists 
    FRESULT FR;
    FILINFO FNO;

    // Check if the file exists 
    bool exists = SD_IS_EXIST(multicore_struct->mSD->fp_env_filename);
    if (exists) { // delete 
        f_unlink(multicore_struct->mSD->fp_env_filename);
        printf("The file %s exists- deleting.\r\n", multicore_struct->mSD->fp_env_filename);
    } 
    
    // Open the file with write access (FA_WRITE) with the read/write pointer at the start (FA_OPEN_ALWAYS)
    FRESULT fr = f_open(multicore_struct->mSD->fp_env, multicore_struct->mSD->fp_env_filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", multicore_struct->mSD->fp_env_filename, FRESULT_str(fr), fr);

}

// format/get the BME string
static void inline bmetimestring_generate(recording_multicore_struct_t* multicore_struct) {

    // read the BME datastring (20 bytes max)
    bme_datastring(multicore_struct->BME_DATASTRING);

    // also read the RTC to record time with the BME data (22 bytes max)
    rtc_read_string_time(multicore_struct->EXT_RTC);

    // snprintf what we want.
    snprintf(
        multicore_struct->BME_AND_TIME_STRING,
        45,
        "%s_%s\n", 
        multicore_struct->EXT_RTC->fullstring,
        multicore_struct->BME_DATASTRING
    );
    
}

// write the BME string 
static void inline bmetimestring_puts(recording_multicore_struct_t* multicore_struct) {

    // try to write it 
    int was_it_a_success = f_puts(multicore_struct->BME_AND_TIME_STRING, multicore_struct->mSD->fp_env);
    if (was_it_a_success<0) {
        panic("Failed writing BME data to file!!!");
    }

}

// Do a recording test run, including measurements for the BME280 when set to "true" in bme280_spi.c.
void run_wav_bme_sequence(void) {

    // generate the multicore_struct 
    printf("Generating struct");
    recording_multicore_struct_t* test_struct = audiostruct_generate();

    // mount the SD volume
    printf("Mounting SD card volume.");
    FRESULT fr = f_mount(&test_struct->mSD->pSD->fatfs, test_struct->mSD->pSD->pcName, 1); 
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr); 

    // Set up the ADC and the BME if we are to use it. 
    setup_adc();
    if (USE_BME==true) {
        setup_bme();
    }

    // run for several file cycles
    printf("The number of files is...! %d ... starting recording session.\r\n", RECORDING_NUMBER_OF_FILES);
    for (int j = 0; j < RECORDING_NUMBER_OF_FILES; j++) {

        // initiate the wav file + BME if we are to use it
        init_wav_file(test_struct);
        if (USE_BME==true) {
            init_bme_file(test_struct);
        }

        // launch core 1
        multicore_launch_core1(core1_test_cycles);

        // send over the struct 
        multicore_fifo_push_blocking((uintptr_t)test_struct);
        
        // populate buffer A to start with (will not wait)
        ADC_to_BUF__(
            test_struct->BUF_A,
            test_struct->ADC_BUFA_CHAN
            );

        /*
        
        Now we need to use any spare time to record the BME measurements.
        As an example (extreme) case consider 384 kHz
        One half-cycle is given 5.3 ms to run 

        That'll give us 5.3 ms to...
        - Record BME280 data (which takes 2 ms minimum due to sleep_ms) from device
        - Get the RTC time down for the BME280 data 
        - Format the BME280 data and RTC time into an appropriate string (with a newline \n at the end.)

        We then have to take the scraps off the FatFS_write cycle to...
        - Close the file for the audio 
        - Open the file for the environmental data 
        - Write BME280 data to the SD card as a string
        - Close the file for the environmental data 
        - Open the file for the audio 
        
        The first step can be done in parallel with an ADC->BUFFER DMA transfer 
        The latter has to be done after a BUFFER->SD transfer to avoid parallel writing.

        Taking the scraps for the environmental file is going to be a bottleneck with higher ADC sample rates.


        */

        // generate the bme timestring 
        if (USE_BME==true) {
            bmetimestring_generate(test_struct);
        }

        // execute waiting code
        WAIT_ADC_to_BUF__(
            test_struct->BUF_A,
            test_struct->ADC_BUFA_CHAN
        );
        
        // run cycles 
        for (int i = 0; i < RECORDING_NUMBER_OF_CYCLES; i++) {

            // buffer A is populated. instantly trigger core1 to populate buffer B
            multicore_fifo_push_blocking((uint32_t)1);

            // while that happens on a separate core, write buffer A to the SD 
            write_BUF__SD(
                test_struct->BUF_A,
                test_struct->BUF_SIZE,
                test_struct->mSD->fp_audio,
                test_struct->BUF_A_BW
            );

            // assume the write has been finished + try to write the appropriate string to the BME file. 
            if (USE_BME==true) {
                if (i%BME_RECORD_PERIOD_CYCLES==0) {
                    bmetimestring_puts(test_struct);
                }
            }
            
            // buffer A has been emptied. wait to switch over to populating buffer A again once B is full.
            multicore_fifo_pop_blocking();

            // begin populating buffer A
            ADC_to_BUF__(
                test_struct->BUF_A,
                test_struct->ADC_BUFA_CHAN
            );

            // while buffer A is populated, check to see if we need to record environment data. if we do, then read it to the test_struct buffer. This will double-up for cycle 0 but whatever.
            if (USE_BME==true) {
                if (i%BME_RECORD_PERIOD_CYCLES==0) {

                    // generate the bme timestring
                    bmetimestring_generate(test_struct);

                }
            }
        
            // execute waiting code
            WAIT_ADC_to_BUF__(
                test_struct->BUF_A,
                test_struct->ADC_BUFA_CHAN
            );

        }

        // done
        dma_channel_wait_for_finish_blocking(*test_struct->ADC_BUFA_CHAN);
        multicore_reset_core1();
        
        fr = f_close(test_struct->mSD->fp_audio);
        if (FR_OK != fr) {
            printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }
        if (USE_BME==true) {
            fr = f_close(test_struct->mSD->fp_env);
            if (FR_OK != fr) {
                printf("f_close error environmental: %s (%d)\n", FRESULT_str(fr), fr);
            }
        }

    }

    f_unmount(test_struct->mSD->pSD->pcName);

    printf("Unmounted & Cycles all done baby! Successful recording session.\r\n");


}
