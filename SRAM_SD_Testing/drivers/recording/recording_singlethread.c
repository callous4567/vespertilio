#include "recording_singlethread.h"
#include "../mSD/mSD.h"
#include "../ext_rtc/ext_rtc.h"
#include "../Utilities/utils.h"
#include "../Utilities/external_config.h"
#include "pico/multicore.h"
#include "../bme280/bme280_spi.h"

/*
Get 6144 sample buffer for the malloc (takes about 10.2 ms to write fully w/ f_write)
16 ms @ 384 kHz ADC sampling 
32 @ 192
64 @ 96
(approx.)
Need to have two DMA channels to chain. 

In the case of anything at 192 kHz or below, just start f_write about 3 milliseconds or so before the cycle ends (note that this will depend on frequency as to where you set the second half of the linear buffer to start.) Gives time for the f_write to burn the first few blocks. We can also reduce baudrate for this to reduce timing requirements.
We do not worry about races here- the SD card will absolutely outpace the ADC, so yeah- easy. The 3 ms is only if you keep the baud at max: if you slow the baud, making it more like 5-10 would work great. Here we are less stringent about the SD card we use- performance isn't necessarily hindered by speed (though should ensure its reasonable.)

In the case of 384 kHz sampling, we need to be more stingent about things. Maximimize baudrate as much as possible. We will be pushing the full 10.2ms write cycle against a 16 ms record cycle. If we pushed to 500 kHz, that's 10.2 ms against 12.28 ms. 
We need to start the f_write within 5.8 ms of the end, obviously, but should aim for maybe 4-5 ms: this ensures that f_write has set off fully and can write without being outraced.
We should note that if the SD card is not capable of doing this in 10 ms or less, it should not be used at 384 kHz. That's not technically a limit- we could do it with a 15 ms setup- the timing would just be subject to the whims of the card, which make things more difficult. 

Chain two DMA channels
Calculate the size of the second start of the buffer (and number of writes necessary, etc, for second half of chain) as (TIME_BEFORE_END/TOTAL_TIME_OF_BUFFER) * BUFFER_SIZE_IN_SAMPLES where TIME_BEFORE_END = 3 ms for most cases. 
Something like that

Need to account for timing to allow extra function calls, i.e. BME280 data collection, that sort of thing. One BME call takes about 3-4 milliseconds which is a timing nightmare for 384 kHz if you only single-core it. 
In such a case, I would recommend that multicore is used. 
*/

/*
initialize the DMA channels and also the buffers we will be using 
*/

static const int32_t BUFAB_ADCBUF_MULTIPLIER = 3; // multiplier of ADC_BUF_SIZE from the multithreaded arrangement, to let us use the same external configuration/etc: changes the size of the total buffer, thus number of cycles 
static const int32_t ADC_BUFB_TIME = 3; // in milliseconds, the length of buffer B, the second half of BUFAB. This might need to be increased for slower microSD cards.


static void init_dma_buf(recording_multicore_struct_single_t* multicore_struct) {

    // malloc up the total buffer 
    int16_t ADC_BUFAB_SIZE = BUFAB_ADCBUF_MULTIPLIER*ADC_BUF_SIZE; // total buffer size BUFAB, in multiple of the multithreaded buffer size, in unit of samples (uint16_t)
    multicore_struct->ADC_BUFA_START_ADDRESS = (int16_t*)malloc(ADC_BUFAB_SIZE*sizeof(int16_t));

    // Set buffer sizes/etc for both buffers + set the offset on BUFB
    int32_t ADC_BUFB_SIZE = ADC_BUFB_TIME * ADC_SAMPLE_RATE / 1000; // sample offset from the end (number of samples BUFB holds) 
    int32_t ADC_BUFA_SIZE = ADC_BUFAB_SIZE - ADC_BUFB_SIZE; // offset from the start (number of samples BUFA holds) for BUFB, alongside the size of BUFA.
    multicore_struct->ADC_BUFB_START_ADDRESS = multicore_struct->ADC_BUFA_START_ADDRESS + ADC_BUFA_SIZE;

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
        multicore_struct->ADC_BUFA_START_ADDRESS,
        &adc_hw->fifo,
        ADC_BUFA_SIZE, 
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
        multicore_struct->ADC_BUFB_START_ADDRESS,
        &adc_hw->fifo,
        ADC_BUFB_SIZE, 
        false
    );

    // Also configure the chain: BUFB will trigger BUFA which will trigger BUFB with no delays, ideally.
    channel_config_set_chain_to(multicore_struct->ADC_BUFA_CONF, *multicore_struct->ADC_BUFB_CHAN); 
    channel_config_set_chain_to(multicore_struct->ADC_BUFB_CONF, *multicore_struct->ADC_BUFA_CHAN);


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
    adc_run(true);
}

// Write an int16_t BUF_ of size BUF_SIZE to the provided file object- just a helper. Verifies that the correct n0 of bytes are written. 
static void write_BUF__SD(
    int16_t *BUF_,
    int BUF_SIZE,
    FIL *fp,
    UINT *bw
) {

    // use f_write to write BUF_A. note that BUF_A is uint16_t and hence bytes written must be double the buf size. 
    f_write(
        fp,
        BUF_,
        BUF_SIZE*2,
        bw
    );

    if (*bw!=(BUF_SIZE*2)) {
        panic("The bytes written does not equal double the uint16_t buffer size in write_BUF_A_SD in SRAM_SD_Testing.cpp. FUCK! %u, %u", *bw, 2*BUF_SIZE);
    }

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
    multicore_struct->ADC_BUFB_CHAN = (int16_t*)malloc(sizeof(int16_t)); 
    multicore_struct->ADC_BUFB_CONF = (dma_channel_config*)malloc(sizeof(dma_channel_config)); 

    // Now allocate mSD pointers where appropriate
    multicore_struct->mSD = (mSD_struct_t*)malloc(sizeof(mSD_struct_t)); 
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
static void init_wav_file(recording_multicore_struct_single_t* multicore_struct) {

    // Read the current time + get string 
    rtc_read_time(multicore_struct->EXT_RTC);
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

// format/get the BME string
static void inline bmetimestring_generate(recording_multicore_struct_single_t* multicore_struct) {

    // read the BME datastring (20 bytes max)
    bme_datastring(multicore_struct->BME_DATASTRING);

    // also read the RTC to record time with the BME data (22 bytes max)
    rtc_read_time(multicore_struct->EXT_RTC);
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
static void inline bmetimestring_puts(recording_multicore_struct_single_t* multicore_struct) {

    // try to write it 
    int was_it_a_success = f_puts(multicore_struct->BME_AND_TIME_STRING, multicore_struct->mSD->fp_env);
    if (was_it_a_success<0) {
        panic("Failed writing BME data to file!!!");
    }

}

void run_wav_bme_sequence_single(void) {

    // generate the multicore_struct 
    printf("Generating struct");
    recording_multicore_struct_single_t* test_struct = audiostruct_generate_single();

    // mount the SD volume
    printf("Mounting SD card volume.");
    FRESULT fr = f_mount(&test_struct->mSD->pSD->fatfs, test_struct->mSD->pSD->pcName, 1); 
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr); 

    setup_adc(); // Set up the ADC and the BME if we are to use it. 
    if (USE_BME==true) { // set up the BME if we're using it
        setup_bme();
    }

    printf("The number of files is...! %d ... starting recording session.\r\n", RECORDING_NUMBER_OF_FILES);
    for (int j = 0; j < RECORDING_NUMBER_OF_FILES; j++) {

        init_wav_file(test_struct);   // initiate the wav file + BME if we are to use it
        if (USE_BME==true) {
            init_bme_file(test_struct);
        }
        
        RECORDING_NUMBER_OF_CYCLES = RECORDING_FILE_DATA_SIZE/(ADC_BUF_SIZE*BUFAB_ADCBUF_MULTIPLIER*2); // assuming one cycle is a total of three ADC_BUF_SIZE sample buffers in int16_t's, then multiply by 2 for bytes 
        BME_RECORD_PERIOD_CYCLES = BME_RECORD_PERIOD_SECONDS*ADC_SAMPLE_RATE/(ADC_BUF_SIZE*BUFAB_ADCBUF_MULTIPLIER); // BME period in seconds * cycles per second 
        
        adc_fifo_drain();   // drain fifo 
        dma_channel_start(*test_struct->ADC_BUFA_CHAN);   // trigger BUFA, which will chain to BUFB automatically. This cycle will only end when we forcibly disable the DMA channels at the end of the cycles.

        for (int i = 0; i < RECORDING_NUMBER_OF_CYCLES; i++) {

            dma_channel_set_write_addr(*test_struct->ADC_BUFB_CHAN, test_struct->ADC_BUFB_START_ADDRESS, false); // reset BUFB write address before it starts
            dma_channel_wait_for_finish_blocking(*test_struct->ADC_BUFA_CHAN); // wait for BUFA to finish 
            dma_channel_set_write_addr(*test_struct->ADC_BUFA_CHAN, test_struct->ADC_BUFA_START_ADDRESS, false); // reset BUFA write address after it ends

            write_BUF__SD( // start the SD card write ADC_BUFB_TIME before the cycle ends 
                test_struct->ADC_BUFA_START_ADDRESS,
                BUFAB_ADCBUF_MULTIPLIER*ADC_BUF_SIZE,
                test_struct->mSD->fp_audio,
                test_struct->mSD->bw
            ); 

            if (USE_BME==true) { // the second the audio buffer is written, write the BME 

                if (i%BME_RECORD_PERIOD_CYCLES==0) {

                    bmetimestring_generate(test_struct);
                    bmetimestring_puts(test_struct);

                }

            }

        }

        // done (we just need to wait for the previous f_write to finish up)
        
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