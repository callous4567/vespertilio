#ifndef VEML_I2C_DRIVER
#define VEML_I2C_DRIVER

#include "veml_registers.h"
#include "../Utilities/pinout.h"


typedef struct {

    // Pins (same as RTC)
    int sda; 
    int scl;
    
    // I2C (same as RTC)
    i2c_inst_t *hw_inst; 
    int baudrate; 

    // Colour data 
    uint16_t *colbuf; // 4 entries, 16-bit.

    // A string of the colour buffer (resolution 16 bit -> 5 digits per measurement = 5 chars per measurement -> 4 measures = 20 chars plus 6 for 3 underscores = 26 characters)
    char* colstring;

    // [0,5] the sensitivity 40-80-160-320-640-1280 milliseconds 
    int8_t sensint; 

} veml_t; // THIS IS MALLOC'D!!!

// get the default VEML object initialized + configured up 
veml_t* init_VEML_default(void);

// configure the provided VEML with the integration time (decimal 0 through 5, 40-80-160-320-640-1280 milliseconds.)
void veml_default_configuration_write(veml_t* VEML);

// read RGBW to colstring 
void veml_read_rgbw(veml_t* VEML);

// free the object + turn the VEML off 
void veml_free(veml_t* VEML);



#endif // VEML_I2C_DRIVER

