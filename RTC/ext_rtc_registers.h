#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

// Note the presence of the interrupt pin for this particular RTC
extern const int RTC_INT_PIN,
RTC_SDA_PIN,
RTC_SCK_PIN,
RTC_BAUD;

// And the i2c 
extern i2c_inst_t* RTC_I2C;

// The slave address for I2C, equal to (0b1=Read,0b0=Write)|(0b1101000=Slave)
extern const uint8_t RTC_SLAVE_ADDRESS;

/*
Shed load of registers 
*/
extern const uint8_t RTC_SECONDS, 
RTC_MINUTES, 
RTC_HOURS, 
RTC_DOTW,
RTC_DOTM,
RTC_MONTHS,
RTC_YEARS,
RTC_ALARM_1_SECONDS,
RTC_ALARM_1_MINUTES,
RTC_ALARM_1_HOURS,
RTC_ALARM_1_DAYDATES,
RTC_ALARM_2_MINUTES,
RTC_ALARM_2_HOURS,
RTC_ALARM_2_DAYDATES,
RTC_CONTROL,
RTC_STATUS,
RTC_TRICKLE;
