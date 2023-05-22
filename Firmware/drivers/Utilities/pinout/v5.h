#include "hardware/i2c.h"
#include "hardware/spi.h"

// PCB version
const extern int32_t PCB_VER;

// ADC pin 
const extern int32_t ADC_PIN;

// define pinout 
const extern int32_t BME_CS_PIN;
const extern int32_t BME_MOSI_PIN;
const extern int32_t BME_MISO_PIN;
const extern int32_t BME_SCK_PIN;
extern spi_inst_t* BME_HW_INST;

// Digital Potentiometer
extern spi_inst_t* DPOT_SPI;
const extern int32_t DPOT_SCK_PIN;
const extern int32_t DPOT_MOSI_PIN;
const extern int32_t DPOT_CSN;
const extern int32_t DPOT_MISO_PIN;

// VEML
const extern int32_t VEML_SDA_PIN;
const extern int32_t VEML_SCK_PIN;
extern i2c_inst_t* VEML_I2C;

// RTC (pins match VEML in this case, and I2C is by default shared in the code.)
const extern int RTC_INT_PIN;
const extern int RTC_SDA_PIN;
const extern int RTC_SCK_PIN;
extern i2c_inst_t* RTC_I2C;

// the pin used to power the pullups used for I2C communication to not only the RTC, but also the VEML light sensing module from Vishay 
const extern int PULLUP_PIN; // pin number 14, GPIO 10, on Version 4 of the PCB debug ver 

// Consts for the enable pins 
const extern int32_t DIGI_ENABLE; // not applicable/used on our debug model- the load switch is broken. but, pull low to enable (opposite to V6.)
const extern int32_t ANA_ENABLE; // pull high to enable 