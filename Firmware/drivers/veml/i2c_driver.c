#include "../Utilities/utils.h"
#include "i2c_driver.h"

/*
Note that the VEML is powered through the Pico by means of Pin 14 / GPIO 10
The RTC functionality for enable_external_pulls/disable_external_pulls powers the VEML and all I2C pullups/etc

Further note-
You should init it, then deinit it, even if not using it,
That ensures it's turned off (since by default, it's on- deinit/free(VEML) will set config to OFF.

*/
static const int32_t VEML_STRINGSIZE = 29; // 4*5 byte char, four spacers (4*2), then one sens char, makes 29. 
static const int32_t VEML_DEFAULT_SENSITIVITY = 5; // 1280 ms 

// write a single data byte (16 bit) to the VEML at the given command register
static void veml_write(veml_t* VEML, uint8_t command, uint16_t data_byte) {

    uint8_t* buffer = (uint8_t*)malloc(3);
    *(buffer) = command;
    *(buffer+1) = (uint8_t)(data_byte);
    *(buffer+2) = (uint8_t)(data_byte >> 8);
    i2c_write_blocking(VEML->hw_inst, VEML_SLAVE, buffer, 3, false);
    free(buffer);

}

// read a single data byte (16 bit) from the VEML to the given buffer for the following command register
static void veml_read(veml_t* VEML, uint8_t command, uint16_t* read_value) {

    uint8_t* valtemp = (uint8_t*)malloc(2);
    i2c_write_blocking(VEML->hw_inst, VEML_SLAVE, &command, 1, true);
    i2c_read_blocking(VEML->hw_inst, VEML_SLAVE, valtemp, 2, false);
    uint16_t lower = *valtemp;
    uint16_t upper = *(valtemp+1);
    *read_value = (upper << 8) | lower;
    free(valtemp);

}

// write the default configuration *WITH* VEML->sensint integration time 
void veml_default_configuration_write(veml_t* VEML) {

    uint16_t config = 0x00;
    config |= (VEML->sensint << 4);
    veml_write(VEML, VEML_CONF, config);

}

// generate a default VEML
veml_t* init_VEML_default(void) {

    // set up the object 
    veml_t* VEML = (veml_t*)malloc(sizeof(veml_t));
    VEML->sda = VEML_SDA_PIN;
    VEML->scl = VEML_SCK_PIN;
    VEML->hw_inst = VEML_I2C;
    VEML->baudrate = VEML_BAUD;
    VEML->sensint = VEML_DEFAULT_SENSITIVITY;

    /*
    // i2c setup (skip this here, as the RTC has the same init already done.)
    i2c_init(
        VEML->hw_inst, 
        VEML->baudrate
    );
    */

    // Malloc up the buffers
    VEML->colbuf = (uint16_t*)malloc(4);
    VEML->colstring = (char*)malloc(VEML_STRINGSIZE*sizeof(char));

    // write configuration with VEML->sensint sensitivity + other defaults.
    veml_default_configuration_write(VEML);

    // return 
    return VEML;

}

// read RGBW to the buffer *and* generate the appropriate string (29 chars/bytes max)
void veml_read_rgbw(veml_t* VEML) {

    veml_read(VEML, VEML_R, VEML->colbuf); // 5 chars 
    veml_read(VEML, VEML_G, VEML->colbuf+1); // 5 chars 
    veml_read(VEML, VEML_B, VEML->colbuf+2); // 5 chars 
    veml_read(VEML, VEML_W, VEML->colbuf+3); // 5 chars 
    snprintf(
        VEML->colstring,
        VEML_STRINGSIZE,
        "%d_%d_%d_%d_%d",
        *(VEML->colbuf),
        *(VEML->colbuf+1),
        *(VEML->colbuf+2),
        *(VEML->colbuf+3),
        VEML->sensint
    ); // 20 chars + four underscores (_) + one final sensitivity char for 29 chars total 

}

// free the VEML and all associated mallocs and shut it off using config 
void veml_free(veml_t* VEML) {

    veml_write(VEML, VEML_CONF, (uint16_t)0b0000000000000001);
    // i2c_deinit(VEML->hw_inst); // We do not- RTC needs it.
    free(VEML->colbuf);
    free(VEML->colstring);
    free(VEML);

}