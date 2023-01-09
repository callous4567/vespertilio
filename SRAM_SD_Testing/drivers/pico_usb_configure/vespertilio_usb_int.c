#include "vespertilio_usb_int.h"

// constants for handshaking 
static const int32_t handshake_interval = 100; // milliseconds
static const int32_t handshake_max_time = 30; // seconds 
static const int32_t handshake_number = handshake_max_time*1000/handshake_interval;

// The Raspberry Pi Vendor ID 
static const int32_t VID = 0x2E8A;

// The Pi Pico SDK CDC UART Product ID 
static const int32_t PID = 0x000A;

// flush stdin stdout 
static inline void flushbuf(void) {

    // works fine 
    fflush(stdout);

    // can't fflush(stdin) due to undefined behaviour. just force out all chars.
    for (int i = 0; i < 50; i++) {
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

    busy_wait_ms(5); // wait for host to read + send confirm true int32

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
    int32_t configuration_size_in_int32 = 15;
    int32_t* configuration_buffer = (int32_t*)malloc(configuration_size_in_int32*sizeof(int32_t));

    /*

    // These are all the variables we need to retrieve from the host, in order of interpretation. 

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
    8) int32_t DOTW;
    9) int32_t DOTM;
    10) int32_t MONTH;
    11) int32_t YEAR;
    12) int32_t ALARM_MIN;
    13) int32_t ALARM_HOUR;

    // INDEPENDENT CONFIGURATION BOOL/INT32_T
    14) int32_t CONFIG_SUCCESS // 1 if successful, 0 if else.

    */
    


    busy_wait_ms(10);
    printf("Ready to accept...\r\n"); // tell the host we're good to go... 
    busy_wait_ms(10);
    flushbuf();
    busy_wait_ms(10);

    int32_t read_amount = fread(configuration_buffer, 4, configuration_size_in_int32, stdin);
    if (read_amount!=configuration_size_in_int32) { // we did not read the entire configuration buffer/read failed.
        *(configuration_buffer+14) = (int32_t)0;
    } else {
        *(configuration_buffer+14) = (int32_t)1;
    }

    return configuration_buffer;

}

/*
Full USB-to-FLASH configuration routine. 
*/
bool configure_flash(void) {

    // configure buffers 
    char stdin_buf[500]; // stdin buffer 500 bytes 
    char stdout_buf[500]; // stdout buffer 500 bytes
    setvbuf(stdin, stdin_buf, _IOLBF, sizeof(stdin_buf));
    setvbuf(stdout, stdout_buf, _IOLBF, sizeof(stdout_buf));


    if (handshake()) { // do the handshake. 

        if (request_configuration()) { // get the "true" from the host signifying it is sending data over.

            int32_t* configuration_buffer = download_configuration(); // try to download the configuration 

            if (*(configuration_buffer+14)==(int32_t)1) { // configuration success- we received the correct amount of int32_t's
                
                flushbuf();
                busy_wait_ms(10);
                printf("SEIKO!\r\n");
                busy_wait_ms(10);
                flushbuf();

                free(configuration_buffer);

                return true;

            } else { // configuration failed- we did not receive the amount of int32_t's

                flushbuf();
                busy_wait_ms(10);
                printf("Configuration failed...\r\n");
                busy_wait_ms(10);
                flushbuf();

                free(configuration_buffer);

                return false; 

            }

        }

    }

}