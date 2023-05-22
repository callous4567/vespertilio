#ifndef EXT_RTC_REGS
#define EXT_RTC_REGS

#include "../Utilities/pinout.h"

// Note the presence of the interrupt pin for this particular RTC
extern const int RTC_BAUD;

// And the i2c, defined inside pinout.h
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

#endif /* EXT_RTC_REGS */