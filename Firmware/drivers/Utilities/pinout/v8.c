#include "v8.h"

// PCB version 
const int32_t PCB_VER = 8;

// BME I2C
const int32_t BME_SDA_PIN = 4;
const int32_t BME_SCL_PIN = 5;
i2c_inst_t* BME_HW_INST = i2c0;

// VEML
const int32_t VEML_SDA_PIN = 6; 
const int32_t VEML_SCK_PIN = 7; 
i2c_inst_t* VEML_I2C = i2c1;

// RTC (pins match VEML in this case, and I2C is by default shared in the code.)
const int RTC_SDA_PIN = 6;  
const int RTC_SCK_PIN = 7; 
const int RTC_INT_PIN = 8;
i2c_inst_t* RTC_I2C = i2c1; 

// VEML and RTC power pin 
const int PULLUP_PIN = 9; 

// Enables
const int32_t DIGI_ENABLE = 11; // pull high to enable 
const int32_t ANA_ENABLE = 28; // pull high to enable 

// The external ADC pins
const int8_t ADC_SCK_PIN = 18;
const int8_t ADC_RX_PIN = 20;
const int8_t ADC_CNSVT_PIN = 21;

// External LED pins
const int8_t DBG_LED_PIN_0 = 16;
const int8_t DBG_LED_PIN_1 = 17;

// The IO expander pins
const int8_t IO_SDA_PIN = 26;
const int8_t IO_SCL_PIN = 27;
i2c_inst_t* IO_I2C = i2c1;
