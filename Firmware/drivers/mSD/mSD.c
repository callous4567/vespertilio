#include "mSD.h"

// Stolen from no-OS-FatFS- check if the test_filename exists. 
bool SD_IS_EXIST(const char *test_filename) {
    
    FRESULT FR;
    FILINFO FNO;
    FR = f_stat(test_filename, &FNO);
    switch (FR) {

    case FR_OK:
        return true;
        break;

    case FR_NO_FILE:
        return false;
        break;

    default:
        panic("SD_IS_EXIST error: %s (%d)\n", FRESULT_str(FR), FR);
    }
}

// Run a test producing a file with a single major gineral string!
static void major_gineral_testwrite(void) {
    
    // Configure drive
    busy_wait_ms(1000);

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
    const char *test_filename = "gineral_test.txt";

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
    
    // Test char object 
    const char* test_string = "I am the model of a major modern gineral I've information vegetable animal and mineral!";
    UINT* n_writtenread;
    UINT n_to_writeread = strlen(test_string);

    // Write to the file! 
    printf("Do a test-write of the modern major general!\r\n");

    FRESULT FWRIT;
    FWRIT = f_write(
        &fil,
        test_string,
        n_to_writeread,
        n_writtenread
        );

    switch (FWRIT) {

    case FR_OK:
        printf("Successfully written to the file of the modern major gineral!\r\n");
        break;

    default:
        panic("We failed in writing the modern major gineral!...");
    }

    // Close the file: string test is done.
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    // Open the file up again... 
    fr = f_open(&fil, test_filename, FA_OPEN_ALWAYS | FA_READ);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", test_filename, FRESULT_str(fr), fr);

    //channel_config_set_read_increment(0, false);

    // Create a read buffer 
    UINT readbuffer[n_to_writeread];
    UINT* bytesread;
    f_read(
        &fil,
        readbuffer,
        n_to_writeread,
        bytesread
    );

    printf("...Ahem... %s", readbuffer);

    //channel_config_set_read_increment(0, true);


    // All done. 
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount(pSD->pcName);
    for (;;);

}

/*
just make a malloc + spam writes to it + measure how long each f_write takes, to the size of our sample buffer (which is adc_buf_size_samples uint16's)
*/
void characterize_SD_write_time(int32_t adc_buf_size_samples) {

    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1); 
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr); 
    FRESULT FR;
    FILINFO FNO;
    const char *test_filename = "writespeed_test.txt";
    bool exists = SD_IS_EXIST(test_filename);
    if (exists) {
        f_unlink(test_filename);
        printf("The file %s exists- deleting.\r\n", test_filename);
    } else {
        printf("The file %s doesn't exist! We will create it instead.\r\n", test_filename);
    }
    FIL fil;


    fr = f_open(&fil, test_filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", test_filename, FRESULT_str(fr), fr);

        
    FRESULT FWRIT;

    // the ADC buffer
    uint16_t* adc_buffer = (uint16_t*)malloc(adc_buf_size_samples*sizeof(uint16_t));
    for (int i = 0; i < adc_buf_size_samples; i++) {
        *(adc_buffer+i) = 0;
    }

    // various things
    UINT bytes_to_write = adc_buf_size_samples*sizeof(uint16_t);
    UINT n_written;
    UINT* n_written_ptr = &n_written;

    // get the current time 
    uint64_t start_time = time_us_64();

    // run cycles + get average time per cycle
    int32_t cycle_amount = 1000;


    printf("Running cycles\r\n");
    for (int i = 0; i < cycle_amount; i++) {

        FWRIT = f_write(
            &fil,
            adc_buffer,
            bytes_to_write,
            n_written_ptr
            );
        if (FWRIT != FR_OK) {
            panic("FWRIT WAS NOT FR_OK! Write failed.");
        }

    }

    // get end time 
    uint64_t end_time = time_us_64();

    // Close the file: test is done.
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount(pSD->pcName);

    printf("Cycles are done.\r\n");

    // execution time average per cycle 
    float executionTime = (float)(end_time - start_time) / (cycle_amount*10000*CLOCKS_PER_SEC);
    printf("The time per cycle is %f\r\n", executionTime);

}