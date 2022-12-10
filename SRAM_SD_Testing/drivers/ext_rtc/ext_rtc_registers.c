#include "ext_rtc_registers.h"

// Note the presence of the interrupt pin for this particular RTC
const int RTC_INT_PIN = 7;
const int RTC_SDA_PIN = 8;
const int RTC_SCK_PIN = 9;
const int RTC_BAUD = 400000;
i2c_inst_t* RTC_I2C = i2c0;

// The address of the RTC. 0x68. The I2C bus handles the addition of the read or write bits (1 and 0.) 
const uint8_t RTC_SLAVE_ADDRESS = 0b1101000;

/*
Shed load of registers 
*/
const uint8_t RTC_SECONDS = 0x00;            
const uint8_t RTC_MINUTES = 0x01;        
const uint8_t RTC_HOURS = 0x02;             
const uint8_t RTC_DOTW = 0x03;               
const uint8_t RTC_DOTM = 0x04;              
const uint8_t RTC_MONTHS = 0x05;            
const uint8_t RTC_YEARS = 0x06;             
const uint8_t RTC_ALARM_1_SECONDS = 0x07;    
const uint8_t RTC_ALARM_1_MINUTES = 0x08;    
const uint8_t RTC_ALARM_1_HOURS = 0x09;      
const uint8_t RTC_ALARM_1_DAYDATES = 0x0A;   
const uint8_t RTC_ALARM_2_MINUTES = 0x0B;
const uint8_t RTC_ALARM_2_HOURS = 0x0C;
const uint8_t RTC_ALARM_2_DAYDATES = 0x0D;
const uint8_t RTC_CONTROL = 0x0E;
const uint8_t RTC_STATUS = 0x0F;
const uint8_t RTC_TRICKLE = 0x10;


/* DEBUG

7 6 5 GREEN BLUE RED INT SCL SDA 
GREEN RED BLUE GPIO 7 8 9

INT GPIO_7
SCL GPIO_9
SDA GPIO_8

*/

