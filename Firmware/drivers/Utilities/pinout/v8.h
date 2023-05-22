#include "hardware/i2c.h"
#include "hardware/spi.h"

// PCB version 
const extern int32_t PCB_VER;

// BME I2C
const extern int32_t BME_SDA_PIN;
const extern int32_t BME_SCL_PIN;
extern i2c_inst_t* BME_HW_INST;

// VEML
const extern int32_t VEML_SDA_PIN; 
const extern int32_t VEML_SCK_PIN; 
extern i2c_inst_t* VEML_I2C;

// RTC (pins match VEML in this case, and I2C is by default shared in the code.)
const extern int RTC_SDA_PIN;  
const extern int RTC_SCK_PIN; 
const extern int RTC_INT_PIN;
extern i2c_inst_t* RTC_I2C; 

// VEML and RTC power pin 
const extern int PULLUP_PIN; 

// Enables
const extern int32_t DIGI_ENABLE; // pull high to enable 
const extern int32_t ANA_ENABLE; // pull high to enable 

// The external ADC pins
const extern int8_t ADC_SCK_PIN;
const extern int8_t ADC_RX_PIN;
const extern int8_t ADC_CNSVT_PIN;

// External LED pins
const extern int8_t DBG_LED_PIN_0;
const extern int8_t DBG_LED_PIN_1;

// The IO expander pins
const extern int8_t IO_SDA_PIN;
const extern int8_t IO_SCL_PIN;
extern i2c_inst_t* IO_I2C; 
