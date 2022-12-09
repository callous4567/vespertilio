// Header Guard
#ifndef EXT_RTC_H
#define EXT_RTC_H

#include "ext_rtc_registers.h"

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

} ext_rtc_t;

// read the register given address and save result to provided pointer 
void rtc_register_read(
    ext_rtc_t *EXT_RTC, 
    uint8_t address, 
    uint8_t *result,
    uint8_t len 
    );

// write the register given address 
void rtc_register_write(
    ext_rtc_t *EXT_RTC,
    uint8_t address,
    uint8_t *data,
    uint8_t len
);

// convert uint8_t to BCD uint8_t 
uint8_t toBCD(uint8_t a);

// convert uint8_t int from BCD 
uint8_t fromBCD(uint8_t a);


// return the pointer to a malloc'd RTC default based on our config.
ext_rtc_t* init_RTC_default(void);

// set the current time of the RTC from the internal timebuf 
void rtc_set_current_time(ext_rtc_t *EXT_RTC);

// read the current time from the rtc to the internal timebuf 
void rtc_read_time(ext_rtc_t *EXT_RTC);

// get time formatted as a string instead. MALLOC MUST FREE LATER!
char* rtc_read_string_time(ext_rtc_t *EXT_RTC); 

#endif /* EXT_RTC_H */