// Header Guard
#ifndef EXT_RTC_H
#define EXT_RTC_H

#include "ext_rtc_registers.h"
#include "hardware/i2c.h"
#include "hardware/rtc.h" // for the sake of interfacing with the pico RTC 
//#include "pico/stdio_usb.h"
//#include "pico/stdio_uart.h"

typedef struct {

    // Pins
    int sda; 
    int scl;
    int ext_int;
    
    // I2C
    i2c_inst_t *hw_inst; 
    int baudrate; 

    // Time keeping. MALLOC OBJECT! Note that this is in int format (not BCD.)
    uint8_t *timebuf; // 7 entries uint8_t top-down see ext_rtc.c. 

    // Alarm keeping. MALLOC OBJECT! Note that this is in int format (not BCD.)
    uint8_t *alarmbuf; // 4 entries- see datasheet. 

    // A string of the current time
    char* fullstring;

} ext_rtc_t;

// return the pointer to a malloc'd RTC default based on our config.
// ext_rtc_t* init_RTC_default(void);

// set the current time of the RTC from the internal timebuf 
// void rtc_set_current_time(ext_rtc_t *EXT_RTC);

// read the current time from the rtc to the internal timebuf 
// void rtc_read_time(ext_rtc_t *EXT_RTC);

// get time formatted as a string instead. MALLOC MUST FREE LATER!
void rtc_read_string_time(ext_rtc_t *EXT_RTC); 

// helper function to return an example that has an alarm set for 30 seconds after time (for debug testing.) 
ext_rtc_t* rtc_debug(void);

// dormant sleep until alarm. experimental + troublesome- need to test this extensively. 
void rtc_sleep_until_alarm(ext_rtc_t *EXT_RTC);

// write the default status register just to get the RTC running appropriately 
void rtc_default_status(ext_rtc_t* EXT_RTC);

// malloc free the RTC 
void rtc_free(ext_rtc_t* EXT_RTC);

// initiate + update the pi pico intRTC (returns datetime_t object under malloc)
datetime_t* init_pico_rtc(ext_rtc_t* EXT_RTC);

// update the internal RTC of the pico 
void update_pico_rtc(ext_rtc_t* EXT_RTC, datetime_t* dtime);

// prime the RTC using the configuration_buffer starting after CONFIGURATION_BUFFER_SIGNIFICANT_VALUES using values in order as noted 
void configure_rtc(int32_t* configuration_buffer, int32_t CONFIGURATION_BUFFER_SIGNIFICANT_VALUES);


#endif /* EXT_RTC_H */