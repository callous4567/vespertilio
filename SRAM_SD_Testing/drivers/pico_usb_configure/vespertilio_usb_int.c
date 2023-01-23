#include "vespertilio_usb_int.h"
#include "../Utilities/utils.h"
#include "../ext_rtc/ext_rtc.h"
#include "tusb.h"
#include "pico/stdio/driver.h"
#include "pico/stdio_usb.h"
#include "pico/stdio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

static const int32_t stdbufsize = 300;
static const int32_t handshake_interval = 100; // milliseconds
static const int32_t handshake_max_time = 30; // seconds 
static const int32_t handshake_number = handshake_max_time*1000/handshake_interval; // number of handshakes 
static const int32_t VID = 0x2E8A; // vendor ID + product ID for the Pico UART USB/etc SDK 
static const int32_t PID = 0x000A; 
static const int32_t CONFIG_FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE; // the offset from XIP_BASE to use for our configuration flash. 
const int32_t CONFIGURATION_BUFFER_INDEPENDENT_VALUES = 4; // 1-based not 0-based: number of values in the desktop JSON we transfer over
static const int32_t CONFIGURATION_BUFFER_MAX_VARIABLES = FLASH_PAGE_SIZE/4; // in int32_t max size of the flash page (256 bytes / int32_t=4 -> 64 variables.)

/*
XIP_BASE is 0x10000000 and extends from here. XIP_BASE is the zeroth byte of the flash.
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024) (pico.h)
#define FLASH_PAGE_SIZE (1u << 8) // program in page sizes (flash.h) ... 256 bytes, i.e. 64 int32_t values. 
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

// // // // These are all the variables we need to retrieve from the host, in order of interpretation, zero-based indices. \\ \\ \\ \\ 

CONFIGURATION_BUFFER_INDEPENDENT_VALUES     pertain to values that are static over the session... 
CONFIGURATION_RTC_INDEPENDENT_VALUES        pertain to values that are dynamic over the session (barring the first 7 values, which defined the initial state of the RTC at configuration.)

CONFIGURATION_BUFFER_INDEPENDENT_VALUES     = 4                                                                                                              // subject to constant change based on features                
CONFIGURATION_RTC_INDEPENDENT_VALUES        = 7 + 1 + 3*NUMBER_OF_ALARMS                                                                                     // where 7 is the RTC config and 1 is NUMBER_OF_ALARMS 
CONFIGURATION_BUFFER_TOTAL_SIZE_BYTES       = FOUR BYTES (INT32_T!!!) * [CONFIGURATION_BUFFER_INDEPENDENT_VALUES + CONFIGURATION_RTC_INDEPENDENT_VALUES + 1] // where 1 has been added to account for CONFIG_SUCCESS
CONFIGURATION_BUFFER_MAX_VARIABLES          = 64                                                                                                             // FLASH_PAGE_SIZE/sizeof(int32_t)

The host should pre-package a total FLASH_PAGE_SIZE=256 bytes=64 int32_t's for us. 
The forward values should be arranged as below. 
The end value, CONFIG_SUCCESS, should be set to unity.

// INDEPENDENT VARIABLES FOR RECORDING SPECIFICS: CONFIGURATION_BUFFER_INDEPENDENT_VALUES of them: set recording parameters.
0)                                                                              int32_t ADC_SAMPLE_RATE = 192000;               
1)                                                                              int32_t RECORDING_LENGTH_SECONDS = 30;           
2)                                                                              int32_t USE_BME = true;                         
3 = CONFIGURATION_BUFFER_INDEPENDENT_VALUES - 1)                                int32_t BME_RECORD_PERIOD_SECONDS = 2;           

// INDEPENDENT TIME VARIABLES- 7 of them. USED BY RTC starting from zero-based index CONFIGURATION_BUFFER_INDEPENDENT_VALUES total of CONFIGURATION_RTC_INDEPENDENT_VALUES
CONFIGURATION_BUFFER_INDEPENDENT_VALUES)                                        int32_t SECOND;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+1)                                      int32_t MINUTE;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+2)                                      int32_t HOUR;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+3)                                      int32_t DOTW; // // 1=SUNDAY (Mon = 2, Tues = 3, etc) Pi Pico SDK has 0 to Sunday, so DOTW-1 = Pi pico DOTW. See Python conversions which set Monday to 2.
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+4)                                      int32_t DOTM;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+5)                                      int32_t MONTH;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+6)                                      int32_t YEAR;

// INDEPENDENT TIME VARIABLES CONTINUED: CUSTOM TIME SCHEDULING FOR THE ALARM! There will be (3N+1) variables here, where the 1 is the "NUMBER_OF_ALARMS." Note that the RECORDING_SESSION_MINUTES must be defined for each alarm. 
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+7)                                      int32_t NUMBER_OF_SESSIONS;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+8)                                      int32_t ALARM_HOUR_1;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+9)                                      int32_t ALARM_MINUTE_1;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+10)                                     int32_t RECORDING_SESSION_MINUTES_1;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+11)                                     int32_t ALARM_HOUR_2;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+12)                                     int32_t ALARM_MINUTE_2;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+13)                                     int32_t RECORDING_SESSION_MINUTES_2;
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t ...
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*WHICH_ALARM_ONEBASED - 2        int32_t ALARM_HOUR_WHICH
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*WHICH_ALARM_ONEBASED - 1        int32_t ALARM_MINUTE_WHICH
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*WHICH_ALARM_ONEBASED            int32_t RECORDING_SESSION_MINUTES_WHICH
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t ...
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*NUMBER_OF_ALARMS - 2)           int32_t ALARM_HOUR_NUMBER_OF_ALARMS;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*NUMBER_OF_ALARMS - 1)           int32_t ALARM_MINUTE_NUMBER_OF_ALARMS;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*NUMBER_OF_ALARMS)               int32_t RECORDING_SESSION_MINUTES_NUMBER_OF_ALARMS;

// Population of zeros
:                                                                               int32_t host sets to zero
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t host sets to zero

// INDEPENDENT CONFIGURATION BOOL/INT32_T
CONFIGURATION_BUFFER_MAX_VARIABLES - 1)                                         int32_t CONFIG_SUCCESS // FROM THE HOST! 1 if successful, 0 if else. The end of the flash page/buffer.

*/


// flush stdin stdout 
static inline void flushbuf(void) {

    // works fine 
    fflush(stdout);

    // can't fflush(stdin) due to undefined behaviour. just force out all chars.
    for (int i = 0; i < stdbufsize; i++) {
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
            break;

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

    busy_wait_ms(1); // wait for host to read + send confirm true int32

    char acknowledged[4];
    fgets(acknowledged, 5, stdin);
    busy_wait_ms(1);

    if (strcmp(acknowledged, "true")==0) { // host "true" matches our "true" + we can continue configuration. double check though with host..
        
        busy_wait_ms(1);
        printf("Thanks.\r\n");
        busy_wait_ms(19);
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

    busy_wait_ms(1000);
    // The configuration buffer: 15 int32_t objects. 
    int32_t* configuration_buffer = (int32_t*)malloc(CONFIGURATION_BUFFER_MAX_VARIABLES * sizeof(int32_t));
    *(configuration_buffer + CONFIGURATION_BUFFER_MAX_VARIABLES - 1) = (int32_t)0; // set default to a "failed transaction" zero

    printf("Ready to accept...\r\n"); // tell the host we're good to go... 

    int32_t read_amount = fread(configuration_buffer, 4, CONFIGURATION_BUFFER_MAX_VARIABLES, stdin); // read a FLASH_PAGE_SIZE of bytes (256 bytes, 64 int32_t's)

    return configuration_buffer;

}

// write the first CONFIGURATION_BUFFER_INDEPENDENT_VALUES of our configuration_buffer to the last page in the flash 
static void write_to_flash(int32_t* configuration_buffer) {


    // pack our configuration buffer (in its entirety) to uint8's for flash. returns a malloc.
    uint8_t* packed_configuration_buffer = pack_int32_uint8(configuration_buffer, CONFIGURATION_BUFFER_MAX_VARIABLES);
    // see https://github.com/raspberrypi/pico-examples/tree/master/flash for examples in pico_examples 

    // create our configuration flash page, which must be in the form of uint8_t 
    uint8_t* flashblock = (uint8_t*)malloc(FLASH_PAGE_SIZE); 
    for (int i = 0; i < FLASH_PAGE_SIZE; i++) { // zero it out 
        *(flashblock + i) = 0; 
    }

    // pack it with our configuration buffer (each value is 4 int32_t, remember.)
    for (int i = 0; i < 4*CONFIGURATION_BUFFER_MAX_VARIABLES; i++) {
        *(flashblock + i) = *(packed_configuration_buffer + i);
    }

    // Disable interrupts https://kevinboone.me/picoflash.html?i=1
    uint32_t ints = save_and_disable_interrupts();

    // erase the sector at our offset
    flash_range_erase(CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE); // erase the sector at our offset

    // write the page
    flash_range_program(CONFIG_FLASH_OFFSET, flashblock, FLASH_PAGE_SIZE); // write the page

    // Enable interrupts
    restore_interrupts (ints); // Enable interrupts

    // free
    free(flashblock);
    free(packed_configuration_buffer);

}

// return the int32_t configuration_buffer in its entirety from the flash (total of 64 int32_t/256 bytes.) Return is a MALLOC OBJECT that needs to be freed at some point.
int32_t* read_from_flash(void) {

    uint8_t* rawbuf = (uint8_t*)malloc(4*CONFIGURATION_BUFFER_MAX_VARIABLES);
    for (int i = 0; i < 4*CONFIGURATION_BUFFER_MAX_VARIABLES; i++) {
        *(rawbuf+i) = *( (uint8_t*)(XIP_BASE + CONFIG_FLASH_OFFSET + i) ); // XIP_BASE is the initial part of the Flash: need to offset to our page.
    }
    int32_t* retbuf = pack_uint8_int32(rawbuf, CONFIGURATION_BUFFER_MAX_VARIABLES);
    free(rawbuf);
    
    return retbuf;

}

/**
 * 
 *  Attempt USB configuration. Returns an int to signify success/status/etc.
 * int 2) Configuration not attempted
 * int 1) Success configuration
 * int 0) Failed configuration
 * 
*/
int usb_configurate(void) {

    // configure buffers 
    char stdin_buf[stdbufsize]; // stdin buffer 256 bytes
    char stdout_buf[stdbufsize]; // stdout buffer 256 bytes
    setvbuf(stdin, stdin_buf, _IOLBF, sizeof(stdin_buf));
    setvbuf(stdout, stdout_buf, _IOLBF, sizeof(stdout_buf));

    if (handshake()) { // if handshake is true (success)

        if (request_configuration()) { // get the "true" from the host signifying it is sending data over.  takes about 30 ms tops after processing. 
            flushbuf();
            int32_t* configuration_buffer = download_configuration(); // try to download the configuration. about 150-200 ms.

            if (*(configuration_buffer+CONFIGURATION_BUFFER_MAX_VARIABLES-1)==(int32_t)1) { // the host has written the correct CONFIG_SUCCESS variable
                
                flushbuf();
                busy_wait_ms(10);
                fwrite(configuration_buffer, 4, CONFIGURATION_BUFFER_MAX_VARIABLES, stdout); // write configuration back to host to verify no corruption
                char* acknowledgement = (char*)malloc(10*sizeof(char)); // get acknowledgement from host that transfer had no corruption
                fgets(acknowledgement, 11, stdin); // waits. you can use fgets to pace the program and wait for the host to confirm.

                if (strcmp(acknowledgement,"Completed.")==0) { // acknowledged true- data isn't corrupted!

                    busy_wait_ms(50);
                    
                    // write to flash the configuration uint8_t*
                    write_to_flash(configuration_buffer);

                    // Next write the RTC configuration 
                    configure_rtc(configuration_buffer, CONFIGURATION_BUFFER_INDEPENDENT_VALUES);

                    // host wants this to verify we truly are done and dusted.
                    printf("Flash written 1.\r\n");

                    // free all malloc 
                    free(configuration_buffer); 
                    free(acknowledgement);

                    // flash lots of times
                    debug_flash_LED(10,1000);

                    return true;

                } else { // host did not verify: configuration failed.

                    free(acknowledgement);
                    free(configuration_buffer);

                    return false;

                }


            } else { // configuration failed- CONFIG_SUCCESS is not equal to unity. 

                free(configuration_buffer);

                return false; 

            }

            free(configuration_buffer);
            printf("Failed\r\n.");
            return false;

        } else {
            
            return false; // host did not request configuration after handshake success 

        }

    } else {

        return 2; // handshake failed/configuration not attempted

    }

}






/* DEPRECATED VERSION WITHOUT TIME SCHEDULING DEPRECATED VERSION WITHOUT TIME SCHEDULING DEPRECATED VERSION WITHOUT TIME SCHEDULING DEPRECATED VERSION WITHOUT TIME SCHEDULING DEPRECATED VERSION WITHOUT TIME SCHEDULING 

// These are all the variables we need to retrieve from the host, in order of interpretation. 
// Total of CONFIGURATION_BUFFER_INDEPENDENT_VALUES significant variables (the top variables.)
// Total of CONFIGURATION_RTC_INDEPENDENT_VALUES significant variables (the bottom variables that set up the RTC.)

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
