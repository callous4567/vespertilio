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

void rtc_read_string_time(ext_rtc_t *EXT_RTC); 

ext_rtc_t* rtc_debug(void);

ext_rtc_t* init_RTC_default(void);

void rtc_sleep_until_alarm(ext_rtc_t *EXT_RTC);

void rtc_default_status(ext_rtc_t* EXT_RTC);

void rtc_free(ext_rtc_t* EXT_RTC);

datetime_t* init_pico_rtc(ext_rtc_t* EXT_RTC);

void update_pico_rtc(ext_rtc_t* EXT_RTC, datetime_t* dtime);

void configure_rtc(int32_t* configuration_buffer, int32_t CONFIGURATION_BUFFER_INDEPENDENT_VALUES);

void rtc_setsleep_WHICH_ALARM_ONEBASED(int32_t* configuration_buffer, int32_t CONFIGURATION_BUFFER_INDEPENDENT_VALUES, int32_t WHICH_ALARM_ONEBASED);

void alarmtest(void);

#endif /* EXT_RTC_H */