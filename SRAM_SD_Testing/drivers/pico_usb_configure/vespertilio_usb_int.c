#include "vespertilio_usb_int.h"
#include "../Utilities/utils.h"
#include "../ext_rtc/ext_rtc.h"
#include "tusb.h"
#include "pico/stdio/driver.h"
#include "pico/stdio_usb.h"
#include "pico/stdio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

static const int32_t handshake_interval = 100; // milliseconds
static const int32_t handshake_max_time = 30; // seconds 
static const int32_t handshake_number = handshake_max_time*1000/handshake_interval; // number of handshakes 
static const int32_t VID = 0x2E8A; // vendor ID + product ID for the Pico UART USB/etc SDK 
static const int32_t PID = 0x000A; 
static const int32_t CONFIG_FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE; // the offset from XIP_BASE to use for our configuration flash. 
static const int32_t CONFIGURATION_BUFFER_SIGNIFICANT_VALUES = 5; // 1-based not 0-based: number of values in the desktop JSON we transfer over
static const int32_t CONFIGURATION_RTC_SIGNIFICANT_VALUES = 9; // 1-based not 0-based: number of RTC values we transfer over 
static const int32_t CONFIGURATION_BUFFER_TOTAL_SIZE = CONFIGURATION_BUFFER_SIGNIFICANT_VALUES + CONFIGURATION_RTC_SIGNIFICANT_VALUES + 1; // +1 for the "success int"

/*
XIP_BASE is 0x10000000 and extends from here. XIP_BASE is the zeroth byte of the flash.
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024) (pico.h)
#define FLASH_PAGE_SIZE (1u << 8) // program in page sizes (flash.h)
#define FLASH_SECTOR_SIZE (1u << 12) // erase sector sizes (flash.h)

So, a valid flash byte can be found at XIP_BASE all the way to (XIP_BASE + PICO_FLASH_SIZE_BYTES.) 
No -1... idk why, see https://github.com/raspberrypi/pico-examples/blob/master/flash/program/flash_program.c

So, our process is...
- Erase the sector located at XIP_BASE + (PICO_FLASH_SIZE_BYTES) - FLASH_SECTOR_SIZE
- Write the page starting at XIP_BASE + (PICO_FLASH_SIZE_BYTES) - FLASH_SECTOR_SIZE

Which we can re-write to
- Erase the sector located at XIP_BASE + CONFIG_FLASH_OFFSET
- Write the page starting at XIP_BASE + CONFIG_FLASH_OFFSET

*/

/*

// These are all the variables we need to retrieve from the host, in order of interpretation. 
// Total of CONFIGURATION_BUFFER_SIGNIFICANT_VALUES significant variables (the top variables.)
// Total of CONFIGURATION_RTC_SIGNIFICANT_VALUES significant variables (the bottom variables.)

// INDEPENDENT VARIABLES (retrieve from host)
0) int32_t ADC_SAMPLE_RATE = 192000;
1) int32_t RECORDING_LENGTH_SECONDS = 30; // note that BME files are matched to this recording length, too. 
2) int32_t RECORDING_SESSION_MINUTES = 1; 
3) int32_t USE_BME = true;
4) int32_t BME_RECORD_PERIOD_SECONDS = 2;

// INDEPENDENT TIME VARIABLES (retrieve from host) USED BY RTC 
5) int32_t SECOND;
6) int32_t MINUTE;
7) int32_t HOUR;
8) int32_t DOTW; // // 1=SUNDAY (Mon = 2, Tues = 3, etc) Pi Pico SDK has 0 to Sunday, so DOTW-1 = Pi pico DOTW
9) int32_t DOTM;
10) int32_t MONTH;
11) int32_t YEAR;
12) int32_t ALARM_MIN;
13) int32_t ALARM_HOUR;

// INDEPENDENT CONFIGURATION BOOL/INT32_T
14) int32_t CONFIG_SUCCESS // 1 if successful, 0 if else.

*/


// flush stdin stdout 
static inline void flushbuf(void) {

    // works fine 
    fflush(stdout);

    // can't fflush(stdin) due to undefined behaviour. just force out all chars.
    for (int i = 0; i < 100; i++) {
        getchar_timeout_us(1);
    }

}

/*
return a true if a tinyusb recognized device is connected, 
else return a false.
if this returns a false, then a USB device is not connected and we can continue on to regular runtime.
*/
static bool handshake(void) {

    bool connected = false;
    for (int i = 0; i < handshake_number; i++) {

        if (!tud_cdc_connected()==true) {
            busy_wait_ms(handshake_interval);
        } else {
            connected = true;
            i = handshake_number;
        }

    }

    return connected;

}

/*
Execute following a handshake. Bool on whether host will send configuration data. Call "download_configuration" immediately after this code.
*/
static bool request_configuration(void) {

    busy_wait_ms(1);

    printf("Configure vespertilio?\r\n"); 

    busy_wait_ms(10); // wait for host to read + send confirm true int32

    char acknowledged[4];
    fgets(acknowledged, 5, stdin);
    busy_wait_ms(1);
    flushbuf();


    if (strcmp(acknowledged, "true")==0) { // host "true" matches our "true" + we can continue configuration. double check though with host..
        
        busy_wait_ms(1);
        printf("Thanks.\r\n");
        busy_wait_ms(10);
        flushbuf();

        return true;

    } else { // doesn't match: host did not give us a "true." Pass a "false" at this point to flag configuration failure. 

        return false;

    }

}

/*
retrieve the configuration array from the host. call this immediately after request_configuration()
*/ 
static int32_t* download_configuration(void) {

    // The configuration buffer: 15 int32_t objects. 
    int32_t* configuration_buffer = (int32_t*)malloc(CONFIGURATION_BUFFER_TOTAL_SIZE*sizeof(int32_t));
    *(configuration_buffer + CONFIGURATION_BUFFER_TOTAL_SIZE - 1) = (int32_t)0; // set default to a "failed transaction" zero

    busy_wait_ms(10);
    printf("Ready to accept...\r\n"); // tell the host we're good to go... 
    busy_wait_ms(20);
    flushbuf();
    busy_wait_ms(50);

    int32_t read_amount = fread(configuration_buffer, 4, CONFIGURATION_BUFFER_TOTAL_SIZE-1, stdin);
    if (read_amount!=CONFIGURATION_BUFFER_TOTAL_SIZE-1) { // we did not read the entire configuration buffer/read failed.
        *(configuration_buffer+14) = (int32_t)0;
    } else {
        *(configuration_buffer+CONFIGURATION_BUFFER_TOTAL_SIZE-1) = (int32_t)1;
    }

    return configuration_buffer;

}

// write the first CONFIGURATION_BUFFER_SIGNIFICANT_VALUES of our configuration_buffer to the last page in the flash 
static void write_to_flash(int32_t* configuration_buffer) {


    // pack our configuration buffer to uint8_t*
    uint8_t* packed_configuration_buffer = pack_int32_uint8(configuration_buffer, CONFIGURATION_BUFFER_SIGNIFICANT_VALUES);
    // see https://github.com/raspberrypi/pico-examples/tree/master/flash for examples in pico_examples 

    // create our configuration flash page, which must be in the form of uint8_t 
    uint8_t* flashblock = (uint8_t*)malloc(FLASH_PAGE_SIZE); 
    for (int i = 0; i < FLASH_PAGE_SIZE; i++) { // zero it out 
        *(flashblock + i) = 0; 
    }

    // pack it with our configuration buffer (each value is 4 int32_t, remember.)
    for (int i = 0; i < 4*CONFIGURATION_BUFFER_SIGNIFICANT_VALUES; i++) {
        *(flashblock + i) = *(packed_configuration_buffer + i);
    }

    // Disable interrupts https://kevinboone.me/picoflash.html?i=1
    uint32_t ints = save_and_disable_interrupts();

    // erase the sector at our offset
    flash_range_erase(CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);

    // write the page
    flash_range_program(CONFIG_FLASH_OFFSET, flashblock, FLASH_PAGE_SIZE);

    // Enable interrupts
    restore_interrupts (ints);

    // free
    free(flashblock);

}

// return the int32_t CONFIGURATION_BUFFER_SIGNIFICANT_VALUES configuration from the flash as a malloc
int32_t* read_from_flash(void) {

    uint8_t* rawbuf = (uint8_t*)malloc(4*CONFIGURATION_BUFFER_SIGNIFICANT_VALUES);
    for (int i = 0; i < 4*CONFIGURATION_BUFFER_SIGNIFICANT_VALUES; i++) {
        *(rawbuf+i) = *( (uint8_t*)(XIP_BASE + CONFIG_FLASH_OFFSET + i) );
    }
    int32_t* retbuf = pack_uint8_int32(rawbuf, CONFIGURATION_BUFFER_SIGNIFICANT_VALUES);
    free(rawbuf);
    
    return retbuf;

}

// prime the RTC using the configuration_buffer starting after CONFIGURATION_BUFFER_SIGNIFICANT_VALUES using values in order as noted 
static void configure_rtc(int32_t* configuration_buffer) {

    // init a default RTC object 
    ext_rtc_t* EXT_RTC = init_RTC_default();

    // set default time 
    *EXT_RTC->timebuf     = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES);
    *(EXT_RTC->timebuf+1) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+1);
    *(EXT_RTC->timebuf+2) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+2);
    *(EXT_RTC->timebuf+3) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+3);
    *(EXT_RTC->timebuf+4) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+4);
    *(EXT_RTC->timebuf+5) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+5);
    *(EXT_RTC->timebuf+6) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+6);
    rtc_set_current_time(EXT_RTC);

    // setup default alarm buffer 
    *EXT_RTC->alarmbuf = 0; // the alarm second is irrelevant to us atm: set to 0 
    *(EXT_RTC->alarmbuf+1) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+7);; // alarm minute 
    *(EXT_RTC->alarmbuf+2) = *(configuration_buffer+CONFIGURATION_BUFFER_SIGNIFICANT_VALUES+8);; // alarm hour 
    *(EXT_RTC->alarmbuf+3) = 1; // the alarm day is irrelevant to use atm: alarm every day
    rtc_set_alarm1(EXT_RTC);

    // set the RTC on appropriately 
    rtc_default_status(EXT_RTC);

    // free the RTC
    rtc_free(EXT_RTC);

}

/*
Full USB configuration routine. 
First plug in the slave pico to the host 
Then open the serial port using the .exe 
The slave does not have to be turned on or off- the host provides the voltage over the USB.
Note that you need to have live batteries inside the slave for this to work- otherwise the RTC will not configure.
After configuration, the slave can survive a maximum time (subject to the capacitor on the RTC) without batteries without sacrificing its time configuration.
*/
bool usb_configurate(void) {

    // configure buffers 
    char stdin_buf[200]; // stdin buffer 500 bytes 
    char stdout_buf[200]; // stdout buffer 500 bytes
    setvbuf(stdin, stdin_buf, _IOLBF, sizeof(stdin_buf));
    setvbuf(stdout, stdout_buf, _IOLBF, sizeof(stdout_buf));

    if (handshake()) { // do the handshake.

        if (request_configuration()) { // get the "true" from the host signifying it is sending data over.  takes about 30 ms tops after processing. 

            int32_t* configuration_buffer = download_configuration(); // try to download the configuration. about 150-200 ms.

            if (*(configuration_buffer+CONFIGURATION_BUFFER_TOTAL_SIZE-1)==(int32_t)1) { // we received the correct number of values 
                
                flushbuf();
                busy_wait_ms(10);
                fwrite(configuration_buffer, 4, CONFIGURATION_BUFFER_TOTAL_SIZE-1, stdout); // write configuration back to host to verify no corruption
                char* acknowledgement = (char*)malloc(10*sizeof(char)); // get acknowledgement from host that transfer had no corruption
                fgets(acknowledgement, 11, stdin); // waits. you can use fgets to pace the program and wait for the host to confirm.

                if (strcmp(acknowledgement,"Completed.")==0) { // acknowledged true- data isn't corrupted!

                    busy_wait_ms(50);
                    
                    // write to flash the configuration uint8_t*
                    write_to_flash(configuration_buffer);

                    // Next write the RTC configuration 
                    configure_rtc(configuration_buffer);

                    // host wants this to verify we truly are done and dusted.
                    printf("Flash written 1.\r\n");

                    // free all malloc 
                    free(configuration_buffer); 
                    free(acknowledgement);

                    // flash lots of times
                    debug_flash_LED(10);

                    return true;

                } else { // host did not verify: configuration failed.

                    free(acknowledgement);
                    free(configuration_buffer);

                    return false;

                }


            } else { // configuration failed- we did not receive the amount of int32_t's/data transfer failed 

                free(configuration_buffer);

                return false; 

            }

            free(configuration_buffer);
            printf("Failed\r\n.");
            return false;
        } 

    }

}