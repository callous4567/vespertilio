#include "recording_singlethread.h"
#include "../mSD/mSD.h"
#include "../ext_rtc/ext_rtc.h"
#include "../Utilities/utils.h"
#include "../Utilities/external_config.h"
#include "pico/multicore.h"
#include "../bme280/bme280_spi.h"
#include "hardware/adc.h"


/**
 * ==================|
 * THREAD SAFETY NOTE|
 * ==================|
 * 
 * See sd_active_wait/sd_active_done
 * Need to be used before any and all file writing procedures (f_open/f_close/f_write/f_write_audiobuf/etc)
 * Otherwise, you will have assertion problems with the SD card.
 * Due to multicore, this is a necessity: it should not cost much in performance, though.
*/
static const int32_t ADC_BLOCKBUF_SIZE = 1024; // two SD card blocks (512 bytes*2.) 
static const int32_t ADC_BLOCKTRANSFER_SIZE = ADC_BLOCKBUF_SIZE/4; // 256 samples -> 512 bytes -> one block 

static void init_dma_buf(recording_multicore_struct_single_t* multicore_struct) {

    // The buffer
    multicore_struct->ADC_BUFA = (int16_t*)malloc(ADC_BLOCKBUF_SIZE);
    // Configure DMA channel from ADC to buffer A
    *multicore_struct->ADC_BUFA_CHAN = dma_claim_unused_channel(true);
    *multicore_struct->ADC_BUFA_CONF = dma_channel_get_default_config(*multicore_struct->ADC_BUFA_CHAN);
    channel_config_set_transfer_data_size(multicore_struct->ADC_BUFA_CONF, DMA_SIZE_16);
    channel_config_set_read_increment(multicore_struct->ADC_BUFA_CONF, false);
    channel_config_set_write_increment(multicore_struct->ADC_BUFA_CONF, true);
    channel_config_set_dreq(multicore_struct->ADC_BUFA_CONF, DREQ_ADC);

    // Apply the configurations (MUST BE DONE AFTER SETTING UP THE CONFIG OBJECTS!!!) https://forums.raspberrypi.com/viewtopic.php?t=335738
    dma_channel_configure(
        *multicore_struct->ADC_BUFA_CHAN,
        multicore_struct->ADC_BUFA_CONF,
        multicore_struct->ADC_BUFA,
        &adc_hw->fifo,
        ADC_BLOCKTRANSFER_SIZE, 
        false
    );

}

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
    adc_run(false);
}

// write the 44-byte WAV header required for the .wave format (do this after opening file and initiating recording.)
static void write_standard_wav_header(recording_multicore_struct_single_t *multicore_struct) {

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

// Generate a multicore struct for recording purely audio data. While this is single-threaded, we will likely want to run this on the second core in the future (so passing this over would be much nicer.) 
static recording_multicore_struct_single_t* audiostruct_generate_single(void) {

    // pointer up. 
    recording_multicore_struct_single_t* multicore_struct = (recording_multicore_struct_single_t*)malloc(sizeof(recording_multicore_struct_single_t));
    
    // Allocate everything for the ADC buffers (start addresses are programmed later, in init_dma_buf)
    multicore_struct->ADC_BUFA_CHAN = (int16_t*)malloc(sizeof(int16_t)); 
    multicore_struct->ADC_BUFA_CONF = (dma_channel_config*)malloc(sizeof(dma_channel_config)); 

    // Now allocate mSD pointers where appropriate
    multicore_struct->mSD = (mSD_struct_t*)malloc(sizeof(mSD_struct_t)); 
    multicore_struct->mSD->pSD = sd_get_by_num(0); 
    multicore_struct->mSD->fp_audio = (FIL*)malloc(sizeof(FIL));
    multicore_struct->mSD->bw = (UINT*)malloc(sizeof(UINT));
    multicore_struct->mSD->fp_audio_filename = (char*)malloc(26); // 22 bytes for the time fullstring, then 4 bytes for .wav 
    multicore_struct->active = (bool*)malloc(sizeof(bool));
    *multicore_struct->active = false;

    // Set up pointers for the BME too (and init it.)
    if (USE_BME==true) {

        // Datastring/timestring/init/fullstring/etc
        multicore_struct->BME_DATASTRING = (char*)malloc(20); // 20 bytes for the BME data 
        multicore_struct->BME_AND_TIME_STRING = (char*)malloc(45); // 20 bytes + 22 RTC bytes + 2 byte spacer + 1 byte newline 
        multicore_struct->BME_STRINGBUFFER = (char*)malloc(BME_BUFFER_SIZE);
        multicore_struct->BME_SHOULD_CONTINUE = (bool*)malloc(sizeof(bool));
        multicore_struct->BME_SLEEPING = (bool*)malloc(sizeof(bool));

        // The microSD pointers too.
        multicore_struct->mSD->fp_env = (FIL*)malloc(sizeof(FIL));
        multicore_struct->mSD->bw_env = (UINT*)malloc(sizeof(UINT));
        multicore_struct->mSD->fp_env_filename = (char*)malloc(30); // 22 bytes for the time fullstring, 4 for .env, 4 for .txt 

    }

    // Finally let's do the RTC 
    multicore_struct->EXT_RTC = (ext_rtc_t*)malloc(sizeof(ext_rtc_t));
    multicore_struct->EXT_RTC = init_RTC_default();

    // Now the DMA chans
    init_dma_buf(multicore_struct);  // init the DMA/BUF configuration

    return multicore_struct;
}

/**
 * Wait for the SD card to no longer be "active" under multicore_struct
 * If it is active, it will wait until it isn't
 * Once it's inactive, it will set active to true and continue
 * Use sd_active_done to set active back to false (convenience function)
 **/
static void inline sd_active_wait(recording_multicore_struct_single_t* multicore_struct) {

    while (*multicore_struct->active) {
        busy_wait_us(1);
    }
    *multicore_struct->active=true; // set to active 

}
static void inline sd_active_done(recording_multicore_struct_single_t* multicore_struct) {
    *multicore_struct->active = false;
}

// initialize the wav file for the current time taken from the RTC and open it for writing (writing the header.) 
static void init_wav_file(recording_multicore_struct_single_t* multicore_struct) {

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
static void init_bme_file(recording_multicore_struct_single_t* multicore_struct) {

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

/* initiate a BME file write, pack the buffer, then write it. will reset the buffer, too. */
static void core1_bme_file(recording_multicore_struct_single_t* multicore_struct) {

    memset(multicore_struct->BME_STRINGBUFFER, 0, BME_BUFFER_SIZE); // reset stringbuf
    int32_t bytes_written = 0;
    while (*multicore_struct->BME_SHOULD_CONTINUE) { // gather data for the correct number

        *multicore_struct->BME_SLEEPING=false;

        // read the BME datastring (20 bytes max)
        bme_datastring(multicore_struct->BME_DATASTRING);

        // also read the RTC to record time with the BME data (22 bytes max)
        rtc_read_string_time(multicore_struct->EXT_RTC);

        // snprintf what we want on to our buffer 
        snprintf(
            multicore_struct->BME_STRINGBUFFER + bytes_written,
            45,
            "%s_%s\n", 
            multicore_struct->EXT_RTC->fullstring,
            multicore_struct->BME_DATASTRING
        );
        bytes_written += 45; // iterate the offset for writing, too. 
        *multicore_struct->mSD->bw_env = bytes_written; // update the number of bytes we have to handle.
        *multicore_struct->BME_SLEEPING=true;
        sleep_ms(BME_RECORD_PERIOD_SECONDS*1000 - 5);

    }

}

/* Process for core1. 
Run this at the start of recordings after you have generated the two appropriate files.
This will create the bme1 file loop and run it indefinitely.
To terminate, just set BME_SHOULD_CONTINUE to false and them shut down core1/reset it.
The file loop will launch the first cycle on BME_SHOULD_CONTINUE=true 
After this, it will do one file until you call BME_SHOULD_CONTINUE=false (then write the file.)
It will then do another file when BME_SHOULD_CONTINUE is set back to true, and so it loops.
*/
static void core1_process(void) {

    recording_multicore_struct_single_t* multicore_struct = (recording_multicore_struct_single_t*)multicore_fifo_pop_blocking();

    setup_bme(); 
    uint32_t pacer;

    while (true) { 
        pacer = (uint32_t)multicore_fifo_pop_blocking(); // wait for the host to say we're good to initialize the file + run
        *multicore_struct->BME_SHOULD_CONTINUE=true;
        core1_bme_file(multicore_struct);
    }

}


static void free_audiostruct(recording_multicore_struct_single_t* multicore_struct) {

    // so it begins...
    free(multicore_struct->mSD->fp_audio);
    free(multicore_struct->mSD->fp_env);
    free(multicore_struct->mSD->bw);
    free(multicore_struct->mSD->bw_env);
    free(multicore_struct->mSD->fp_audio_filename);
    free(multicore_struct->mSD->fp_env_filename);

    // and the active...
    free(multicore_struct->active);
    
    // the RTC
    rtc_free(multicore_struct->EXT_RTC);

    // unclaim the dma channel
    dma_channel_unclaim(*multicore_struct->ADC_BUFA_CHAN);

    // and free... 
    free(multicore_struct->ADC_BUFA);
    free(multicore_struct->ADC_BUFA_CHAN);
    free(multicore_struct->ADC_BUFA_CONF);

    // if the BME exists
    if (USE_BME) {

        free(multicore_struct->BME_DATASTRING);
        free(multicore_struct->BME_AND_TIME_STRING);
        free(multicore_struct->BME_SHOULD_CONTINUE);
        free(multicore_struct->BME_SLEEPING);
        free(multicore_struct->BME_STRINGBUFFER);

    }

    // that should be it... I hope...

}

void run_wav_bme_sequence_single(void) {

    recording_multicore_struct_single_t* test_struct = audiostruct_generate_single();   // generate the multicore_struct 

    printf("Mounting SD card volume.");   // mount the SD volume
    FRESULT fr = f_mount(&test_struct->mSD->pSD->fatfs, test_struct->mSD->pSD->pcName, 1); 
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr); 

    setup_adc(); // Set up the ADC 

    rtc_read_string_time(test_struct->EXT_RTC); // read the rtc time 
    datetime_t* dtime = init_pico_rtc(test_struct->EXT_RTC); // init the pico RTC + configure from the external RTC
    if (USE_BME) { // launch core1 process.
        multicore_launch_core1(core1_process);
        multicore_fifo_push_blocking((uintptr_t)test_struct); // pass over our test_struct 
    }

    printf("The number of files is...! %d ... starting recording session.\r\n", RECORDING_NUMBER_OF_FILES);
    for (int j = 0; j < RECORDING_NUMBER_OF_FILES; j++) { // iterate over number of files 

        printf("File number %d\r\n", j);

        rtc_read_string_time(test_struct->EXT_RTC); // read string time 
        update_pico_rtc(test_struct->EXT_RTC, dtime); // update the pico RTC for file writing

        if (USE_BME) {
            multicore_fifo_push_blocking((uint32_t)1); // pass over an int32 to init a new bme file/etc 
        }

        sd_active_wait(test_struct);
        init_wav_file(test_struct);   // initiate the wave file for audio
        adc_fifo_drain();   // drain fifo
        adc_run(true);  // run ADC 
        dma_channel_set_write_addr(*test_struct->ADC_BUFA_CHAN, test_struct->ADC_BUFA, true);   // trigger DMA to the first half of the buffer immediately 
        f_write_audiobuf( 
            test_struct->mSD->fp_audio,
            test_struct->ADC_BUFA,
            RECORDING_FILE_DATA_SIZE,
            test_struct->mSD->bw,
            *test_struct->ADC_BUFA_CHAN
        ); // and run the ADC file writing
        adc_run(false); // all done: stop the ADC
        
        fr = f_close(test_struct->mSD->fp_audio); // done. write file.
        if (FR_OK != fr) {
            printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }
        sd_active_done(test_struct);

        if (USE_BME) { 
            *test_struct->BME_SHOULD_CONTINUE=false; // stop data gathering. this is set back to true by core1 when we push again.
            while (!*test_struct->BME_SLEEPING) {
                busy_wait_us(100);
            }
            sd_active_wait(test_struct); 
            init_bme_file(test_struct); // init the file: we are golden to go 
            int32_t strings_to_dump = *test_struct->mSD->bw_env/45;
            for (int k = 0; k < strings_to_dump; k++) {
                f_puts(test_struct->BME_STRINGBUFFER + 45*k, test_struct->mSD->fp_env);
            }
            FRESULT fr;
            fr = f_close(test_struct->mSD->fp_env);
            if (FR_OK != fr) {
                printf("f_close error environmental: %s (%d)\n", FRESULT_str(fr), fr);
            }
            sd_active_done(test_struct);
        
        }

    }

    if (USE_BME) {
        busy_wait_ms(1100*BME_RECORD_PERIOD_SECONDS);
        multicore_reset_core1();
    }
    f_unmount(test_struct->mSD->pSD->pcName);
    printf("Unmounted & Cycles all done baby! Successful recording session.\r\n");

    // free the test struct
    free_audiostruct(test_struct);

    // and the time
    free(dtime);

}
