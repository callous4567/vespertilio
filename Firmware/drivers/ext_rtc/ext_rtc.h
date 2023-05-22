// Header Guard
#ifndef EXT_RTC_H
#define EXT_RTC_H

#include "ext_rtc_registers.h"
#include "hardware/i2c.h"
#include "hardware/rtc.h" 
#include "pico/mutex.h"

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
    
    /*
    mutex all i2c operations. 
    If flashlog exists, RTC inherits flashlog mutex. 
    If not, RTC makes its own mutex in default init. 
    VEML/BME280 inherit RTC mutex.
    Note that the flashlog should always create the mutex first before calling ext_rtc initialization if used
    */
    mutex_t* mutex;

} ext_rtc_t;

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