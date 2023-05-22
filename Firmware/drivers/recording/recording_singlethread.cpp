#include "recording_singlethread.h"
extern "C" {
    #include "../Utilities/utils.h"
    #include "../Utilities/external_config.h"
    #include "pico/multicore.h"
    #include "../bme280/bme280_spi.h"
    #include "hardware/adc.h"
    #include "multicore_struct.h"
}

/*
TODO:
- Remove the sd_active_wait/etc bit- we don't need thread safety for file write
since we no longer write from core1 
- Clean the hell up with the comments/etc... proper docstringing 
- write the flashlog (from the previous session) to the debug file .txt, for convenience purposes.
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

    // Apply the configurations (MUST BE DONE AFTER SETTING UP THE CONFIG OBJECTS!!!) https://forums.raspberrypi.com/viewtopic.php?t=335TIME_VEML_BME_STRINGSIZE8
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
    multicore_struct->ADC_WHICH_HALF = (int8_t*)malloc(sizeof(int8_t));

    // Now allocate mSD pointers where appropriate
    multicore_struct->mSD = (mSD_struct_t*)malloc(sizeof(mSD_struct_t)); 
    multicore_struct->mSD->pSD = sd_get_by_num(0); 
    multicore_struct->mSD->fp_audio = (FIL*)malloc(sizeof(FIL));
    multicore_struct->mSD->bw = (UINT*)malloc(sizeof(UINT));
    multicore_struct->mSD->fp_audio_filename = (char*)malloc(26); // 22 bytes for the time fullstring, then 4 bytes for .wav 
    multicore_struct->active = (bool*)malloc(sizeof(bool));
    *multicore_struct->active = false;

    // Set up RTC (or inherit it) 
    if (USE_FLASHLOG) { 
        multicore_struct->EXT_RTC = flashlog->EXT_RTC;
    } else {
        multicore_struct->EXT_RTC = init_RTC_default();
    }

    // Set up pointers for the ENV too (and init it.) We do this after the RTC, because the VEML needs the RTC I2C to be running already. 
    if (USE_ENV==true) {

        // Datastring/timestring/init/fullstring/etc
        multicore_struct->BME_DATASTRING = (char*)malloc(20); // 20 bytes for the BME data 
        multicore_struct->ENV_AND_TIME_STRING = (char*)malloc(TIME_VEML_BME_STRINGSIZE); // 22 RTC bytes + 2 byte spacer + 20 bytes BME + 2 byte spacer + 26 bytes VEML + 1 byte newline 
        multicore_struct->ENV_STRINGBUFFER = (char*)malloc(ENV_BUFFER_SIZE);
        multicore_struct->ENV_SHOULD_CONTINUE = (bool*)malloc(sizeof(bool));
        multicore_struct->ENV_SLEEPING = (bool*)malloc(sizeof(bool));

        // Default VEML
        multicore_struct->VEML = init_VEML_default(multicore_struct->EXT_RTC->mutex);

        // The microSD pointers too.
        multicore_struct->mSD->fp_env = (FIL*)malloc(sizeof(FIL));
        multicore_struct->mSD->bw_env = (UINT*)malloc(sizeof(UINT));
        multicore_struct->mSD->fp_env_filename = (char*)malloc(30); // 22 bytes for the time fullstring, 4 for .env, 4 for .txt 

    }

    // Now the DMA chans
    init_dma_buf(multicore_struct);  // init the DMA/BUF configuration

    // Finally also do the Debug file (which we will use for writing down the previous log at the start of the session.)
    multicore_struct->mSD->fp_debug = (FIL*)malloc(sizeof(FIL));
    multicore_struct->mSD->fp_debug_filename = (char*)malloc(26);
    multicore_struct->mSD->bw_debug = (UINT*)malloc(sizeof(UINT));

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

// create a log file to use for a given session (which will store anything we want to log. Just a standard .txt file.)
static void init_debug_file(recording_multicore_struct_single_t* multicore_struct) {

    // Read the current time + get string 
    rtc_read_string_time(multicore_struct->EXT_RTC);

    // Generate a string with the time at the front and .log on the end: fullstring is maximum of 22 bytes, .log is 4 bytes. 
    snprintf(
        multicore_struct->mSD->fp_debug_filename,
        26,
        "%s.log",
        multicore_struct->EXT_RTC->fullstring
    );

    // Check if file exists 
    FRESULT FR;
    FILINFO FNO;

    // Check if the file exists 
    bool exists = SD_IS_EXIST(multicore_struct->mSD->fp_debug_filename);
    if (exists) { // delete 
        f_unlink(multicore_struct->mSD->fp_debug_filename);
        custom_printf("The file %s exists- deleting.\r\n", multicore_struct->mSD->fp_debug_filename);
    } 
    
    // Open the file with write access (FA_WRITE) with the read/write pointer at the start (FA_OPEN_ALWAYS)
    FRESULT fr = f_open(multicore_struct->mSD->fp_debug, multicore_struct->mSD->fp_debug_filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", multicore_struct->mSD->fp_debug_filename, FRESULT_str(fr), fr);

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
    } 
    
    // Open the file with write access (FA_WRITE) with the read/write pointer at the start (FA_OPEN_ALWAYS)
    FRESULT fr = f_open(multicore_struct->mSD->fp_audio, multicore_struct->mSD->fp_audio_filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        panic("f_open(%s) error: %s (%d)\n", multicore_struct->mSD->fp_audio_filename, FRESULT_str(fr), fr);
    }

    // Write the wav header/etc 
    write_standard_wav_header(multicore_struct);

}

// initialize the BME text file for the current time taken from the RTC and open it for writing. Note that this assumes you have already gotten the EXT_RTC fullstring (you should have, for init_wav.)
static void init_env_file(recording_multicore_struct_single_t* multicore_struct) {

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
    } 
    
    // Open the file with write access (FA_WRITE) with the read/write pointer at the start (FA_OPEN_ALWAYS)
    FRESULT fr = f_open(multicore_struct->mSD->fp_env, multicore_struct->mSD->fp_env_filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        panic("f_open(%s) error: %s (%d)\n", multicore_struct->mSD->fp_env_filename, FRESULT_str(fr), fr);
    }

}

// reset stringbuf (holds all measurements for a single recording) and then, subject to RTC timing (no FIFO pacing) record every ENV_RECORD_PERIOD_SECONDS.
static void core1_env_file(recording_multicore_struct_single_t* multicore_struct) {

    memset(multicore_struct->ENV_STRINGBUFFER, 0, ENV_BUFFER_SIZE); // reset stringbuf
    int32_t bytes_written = 0;
    while (*multicore_struct->ENV_SHOULD_CONTINUE) { // gather data over the individual recording every ENV_PERIOD_SECONDS. 

        *multicore_struct->ENV_SLEEPING=false;

        // read the BME datastring (20 bytes max)
        bme_datastring(multicore_struct->BME_DATASTRING);

        // next read the VEML string (26 bytes max)
        veml_read_rgbw(multicore_struct->VEML);

        // also read the RTC to record time with the BME data (22 bytes max)
        rtc_read_string_time(multicore_struct->EXT_RTC);

        // snprintf what we want on to our buffer. 20 + 26 + 22 = 68, plus two underscores (2*2) and a newline (1) makes TIME_VEML_BME_STRINGSIZE. 
        snprintf(
            multicore_struct->ENV_STRINGBUFFER + bytes_written,
            TIME_VEML_BME_STRINGSIZE,
            "%s_%s_%s\n", 
            multicore_struct->EXT_RTC->fullstring,
            multicore_struct->BME_DATASTRING,
            multicore_struct->VEML->colstring
        );
        bytes_written += TIME_VEML_BME_STRINGSIZE; // iterate the offset for writing, too. 
        *multicore_struct->mSD->bw_env = bytes_written; // update the number of bytes we have to handle.
        *multicore_struct->ENV_SLEEPING=true;
        sleep_ms(ENV_RECORD_PERIOD_SECONDS*1000 - 5); // precess by 5 ms to account for the cost of running this bit of the code 

    }

}

// core1 process. called once per SESSION and ran over several recordings without reset, and paced by the FIFO (FIFO push by core0 once per recording.)
static void core1_process(void) {

    recording_multicore_struct_single_t* multicore_struct = (recording_multicore_struct_single_t*)multicore_fifo_pop_blocking();

    // set up the BME for SPI. Note that the VEML is just I2C and we already set that up earlier with the RTC I2C/etc. Note that EXT_RTC mutex is needed.
    setup_bme(multicore_struct->EXT_RTC->mutex); 
    uint32_t pacer;

    while (true) { 
        pacer = (uint32_t)multicore_fifo_pop_blocking(); // wait for the host to say we're good to initialize the file + run
        *multicore_struct->ENV_SHOULD_CONTINUE=true;
        core1_env_file(multicore_struct);
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
    
    // if the BME exists
    if (USE_ENV) {

        free(multicore_struct->BME_DATASTRING);
        free(multicore_struct->ENV_AND_TIME_STRING);
        free(multicore_struct->ENV_SHOULD_CONTINUE);
        free(multicore_struct->ENV_SLEEPING);
        free(multicore_struct->ENV_STRINGBUFFER);
        veml_free(multicore_struct->VEML); // needs the RTC to still exist. 

    }

    // the RTC (if flashlog is not used- else flashlog.h will deinit the flashlog RTC)
    if (!USE_FLASHLOG) {
        rtc_free(multicore_struct->EXT_RTC);
    }

    // unclaim the dma channel
    dma_channel_unclaim(*multicore_struct->ADC_BUFA_CHAN);

    // and free... 
    free(multicore_struct->ADC_BUFA);
    free(multicore_struct->ADC_BUFA_CHAN);
    free(multicore_struct->ADC_BUFA_CONF);
    free(multicore_struct->ADC_WHICH_HALF);

    // then free debug stuff
    free(multicore_struct->mSD->fp_debug);
    free(multicore_struct->mSD->fp_debug_filename);
    free(multicore_struct->mSD->bw_debug);

    // Free the struct overall, too.
    free(multicore_struct);

}

static void env_singlet(recording_multicore_struct_single_t* multicore_struct, datetime_t* dtime) {
    *multicore_struct->ENV_SHOULD_CONTINUE=false; // stop data gathering. this is set back to true by core1 when we push again.
    while (!*multicore_struct->ENV_SLEEPING) { // wait for core1 to stop any activity/go to sleep 
        busy_wait_us(100);
    }
    sd_active_wait(multicore_struct); 
    init_env_file(multicore_struct); // initiate the ENV file to dump our environmental stringbuf to 
    int32_t strings_to_dump = *multicore_struct->mSD->bw_env/TIME_VEML_BME_STRINGSIZE;
    for (int k = 0; k < strings_to_dump; k++) {
        f_puts(multicore_struct->ENV_STRINGBUFFER + TIME_VEML_BME_STRINGSIZE*k, multicore_struct->mSD->fp_env);
    }
    FRESULT fr;
    fr = f_close(multicore_struct->mSD->fp_env);
    if (FR_OK != fr) {
        panic("f_close error environmental: %s (%d)\n", FRESULT_str(fr), fr);
    }
    sd_active_done(multicore_struct);
}

// run this code to do a single recording (as part of a recording sequence) with a multicore_struct already initialized. returns a true on success.
static void recording_singlet(recording_multicore_struct_single_t* multicore_struct, datetime_t* dtime) {

    rtc_read_string_time(multicore_struct->EXT_RTC); // read string time 
    update_pico_rtc(multicore_struct->EXT_RTC, dtime); // update the pico RTC for file writing

    // Initialize core1 to record the environmental file (it paces itself- no need to pace it.)
    if (USE_ENV) {
        multicore_fifo_push_blocking((uint32_t)1); // pass over an int32 to init a new bme file/etc 
    }

    sd_active_wait(multicore_struct);
    init_wav_file(multicore_struct);   // initiate the wave file for audio

    adc_fifo_drain();   // drain fifo
    adc_run(true);  // run ADC 
    dma_channel_set_write_addr(*multicore_struct->ADC_BUFA_CHAN, multicore_struct->ADC_BUFA, true);   // trigger DMA to the first half of the buffer immediately 
    *multicore_struct->ADC_WHICH_HALF = 0; // we're currently writing the zeroth half 
    f_write_audiobuf( 
        multicore_struct->mSD->fp_audio,
        multicore_struct->ADC_BUFA,
        RECORDING_FILE_DATA_SIZE,
        multicore_struct->mSD->bw,
        *multicore_struct->ADC_BUFA_CHAN,
        multicore_struct->ADC_WHICH_HALF
    ); // and run the ADC file writing
    adc_run(false); // all done: stop the ADC

    FRESULT fr;
    fr = f_close(multicore_struct->mSD->fp_audio); // done. finish the audio file. 
    if (FR_OK != fr) {
        panic("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    sd_active_done(multicore_struct);

    // write env buffer 
    if (USE_ENV) { 
        try {
            env_singlet(multicore_struct, dtime);
        } catch (...) {
            custom_printf("Env write failed, first pass.\r\n"); // no point in logging to file at this point, as it will probably fail.

            try {
                // re-try file write 
                env_singlet(multicore_struct, dtime);
            } catch (...) {

                panic("We're done. Env fail second pass.\r\n");

            }

        }

    }

}

// standard sequence: start recording and run for the number of recordings in this session.
void run_wav_bme_sequence_single(void) {

    recording_multicore_struct_single_t* test_struct = audiostruct_generate_single();   // generate the multicore_struct 

    custom_printf("Mounting SD card volume.");   // mount the SD volume
    FRESULT fr = f_mount(&test_struct->mSD->pSD->fatfs, test_struct->mSD->pSD->pcName, 1); 
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr); 

    setup_adc(); // Set up the ADC 

    rtc_read_string_time(test_struct->EXT_RTC); // read the rtc time 
    datetime_t* dtime = init_pico_rtc(test_struct->EXT_RTC); // init the pico RTC + configure from the external RTC
    if (USE_ENV) { // launch core1 process (reset before just in case)
        multicore_reset_core1();
        multicore_launch_core1(core1_process);
        multicore_fifo_push_blocking((uintptr_t)test_struct); // pass over our test_struct 
    }

    for (int j = 0; j < RECORDING_NUMBER_OF_FILES; j++) { // iterate over number of files 
    
        /**
         * Error catching code for failed recordings
         * Note that we should label the "caught" attempt at recording with ERROR at the front
         * We should nest two try-catches such that if a successive fail is made, then we break the program, crash the Pico purposefully
         * That should satisfy the error catchment needs (no two errors in a row, I'd hope.)
        */

        custom_printf("Doing file number %d\r\n", j);

        try {
            recording_singlet(test_struct, dtime);
        } catch (...) {

            try {
                // re-run recording 
                if (USE_ENV) { // launch core1 process (reset before just in case)
                    multicore_reset_core1();
                    multicore_launch_core1(core1_process);
                    multicore_fifo_push_blocking((uintptr_t)test_struct); // pass over our test_struct 
                }
                recording_singlet(test_struct, dtime);
            } catch (...) {

                panic("RECORDING SINGLET FAILED SECOND PASS! Oof %d, j");

            }

        }
    }

    if (USE_ENV) {
        sleep_ms(1100*ENV_RECORD_PERIOD_SECONDS);
        multicore_reset_core1();
    }

    f_unmount(test_struct->mSD->pSD->pcName);
    custom_printf("Unmounted SD card- session done :)\r\n");

    // free the test struct
    free_audiostruct(test_struct);

    // and the time
    free(dtime);

}
