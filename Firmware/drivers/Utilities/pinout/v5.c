#include "../pinout.h"

// PCB version 
const int32_t PCB_VER = 5;

// ADC pin 
const int32_t ADC_PIN = 26;

// define pinout 
const int32_t BME_CS_PIN = 5;
const int32_t BME_MOSI_PIN = 3;
const int32_t BME_MISO_PIN = 4;
const int32_t BME_SCK_PIN = 2;
spi_inst_t* BME_HW_INST = spi0;

// Digital Potentiometer
spi_inst_t* DPOT_SPI = spi0; 
const int32_t DPOT_SCK_PIN = 18;
const int32_t DPOT_MOSI_PIN = 19;
const int32_t DPOT_CSN = 17; 
const int32_t DPOT_MISO_PIN = 16;

// VEML
const int32_t VEML_SDA_PIN = 8;
const int32_t VEML_SCK_PIN = 9;
i2c_inst_t* VEML_I2C = i2c0;

// RTC (pins match VEML in this case, and I2C is by default shared in the code.)
const int RTC_INT_PIN = 7; 
const int RTC_SDA_PIN = 8; 
const int RTC_SCK_PIN = 9; 
i2c_inst_t* RTC_I2C = i2c0; 

// the pin used to power the pullups used for I2C communication to not only the RTC, but also the VEML light sensing module from Vishay 
const int PULLUP_PIN = 10; // pin number 14, GPIO 10, on Version 4 of the PCB debug ver 

// Consts for the enable pins 
const int32_t DIGI_ENABLE = 6; // not applicable/used on our debug model- the load switch is broken. but, pull low to enable (opposite to V6.)
const int32_t ANA_ENABLE = 22; // pull high to enable 