#ifndef PINOUT_CUSTOM
#define PINOUT_CUSTOM

#include "hardware/i2c.h"
#include "hardware/spi.h"

// Current versioning. Note that you should adjust CMakeLists.txt for which one you want to use!) 
extern const int32_t PCB_VER; 

// ADC pin 
extern const int32_t ADC_PIN;

// Digital Potentiometer
extern spi_inst_t* DPOT_SPI;
extern const int32_t DPOT_SCK_PIN;
extern const int32_t DPOT_MOSI_PIN;
extern const int32_t DPOT_CSN;
extern const int32_t DPOT_MISO_PIN;

// VEML
extern const int32_t VEML_SDA_PIN;
extern const int32_t VEML_SCK_PIN;
extern i2c_inst_t* VEML_I2C;

// Same as VEML- code shares the I2C by default anyway. 
extern const int RTC_INT_PIN;
extern const int RTC_SDA_PIN;
extern const int RTC_SCK_PIN;
extern i2c_inst_t* RTC_I2C;

// VEML/RTC pullup pin for powering RTC/VEML 
extern const int PULLUP_PIN; 

// BME280
extern const int32_t BME_CS_PIN;
extern const int32_t BME_MOSI_PIN;
extern const int32_t BME_MISO_PIN;
extern const int32_t BME_SCK_PIN;
extern spi_inst_t* BME_HW_INST;

// Note that for the MicroSD you need to adjust hw_config.c manually- sorry. 


// Consts for the enable pins 
extern const int32_t DIGI_ENABLE, ANA_ENABLE;

#endif // PINOUT_CUSTOM