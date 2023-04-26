#include "../Utilities/utils.h"
#include "i2c_driver.h"
#include <math.h>

/*
Note that the VEML is powered through the Pico by means of Pin 14 / GPIO 10
The RTC functionality for enable_external_pulls/disable_external_pulls powers the VEML and all I2C pullups/etc

Further note-
You should init it, then deinit it, even if not using it,
That ensures it's turned off (since by default, it's on- deinit/free(VEML) will set config to OFF.

*/
static const int32_t VEML_STRINGSIZE = 29; // 4*5 byte char, four spacers (4*2), then one sens char, makes 29. 
static const int32_t VEML_DEFAULT_SENSITIVITY = 0; // 40 ms (40,80,160,320,640,1280 0,1,2,3,4,5.) Mathematically, the conversion is 40 milliseconds * 2^(sensitivity) 

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

// write configuration with provided sensint- will automatically alter VEML->sensint value. 
static void veml_config_write(veml_t* VEML, int8_t sensint) {
    /* set the VEML sensitivity to the provided integer value, whilst also setting the internal VEML->sensint value appropriately.*/
    VEML->sensint = sensint; // set the sensint value in VEML 
    uint16_t config = 0x00;
    config |= (VEML->sensint << 4);
    veml_write(VEML, VEML_CONF, config);
}

// read raw values from veml 
static void veml_read_rgbw_raw(veml_t* VEML) {
    veml_read(VEML, VEML_R, VEML->colbuf); // 5 chars 
    veml_read(VEML, VEML_G, VEML->colbuf+1); // 5 chars 
    veml_read(VEML, VEML_B, VEML->colbuf+2); // 5 chars 
    veml_read(VEML, VEML_W, VEML->colbuf+3); // 5 chars 
}

// convenience- calculate period of recording 
static inline int16_t sens_period(int8_t sensint) {
    return(40*pow(2,sensint));
}

// convenience- sleep two periods of recording 
static inline void sens_sleep(veml_t* VEML) {
    busy_wait_ms(2*sens_period(VEML->sensint));
}

/* quickly step up/down the VEML sensitivity based on the current value of sensitivity. */
static void veml_sens_step(veml_t* VEML) {
    if ((*(VEML->colbuf+3)>58982)&&(VEML->sensint!=0)) { // first check if saturated: if we are, and we're not at minimum exposure, 
        veml_config_write(VEML, VEML->sensint-1); // try to step the exposure down by half 
    }  
    if ((*(VEML->colbuf+3) < 6553)&&(VEML->sensint!=5)) { // next check if we're undersaturated, and we're not at maximum exposure,
        veml_config_write(VEML, VEML->sensint+1); // try to step the exposure up by half 
    } 
}

// set the initial sensitivity: this step will be very slow as it polls the VEML repeatedly, at double the current sensitivity period.
static void veml_sens_step_startup(veml_t* VEML) {

    veml_config_write(VEML, VEML_DEFAULT_SENSITIVITY); // set the default sensitivity
    sens_sleep(VEML); // sleep 
    veml_read_rgbw_raw(VEML); // initial measure
    sens_sleep(VEML); // sleep 
    while (((*(VEML->colbuf+3) < 6553)&&(VEML->sensint!=5))||((*(VEML->colbuf+3)>58982)&&(VEML->sensint!=0))) { // while we're undersaturated w/o maximum exposure, or saturated w/o minimum exposure...
        veml_sens_step(VEML); // step based on current value 
        veml_read_rgbw_raw(VEML); // poll VEML
        sens_sleep(VEML); // sleep and repeat 
        printf("Current VEML %d\r\n", VEML->sensint);
    }

}

// generate a default VEML (this includes the step of calibrating sensitivity.) 
veml_t* init_VEML_default(void) {

    // set up the object (VEML->sensint is set by veml_config_write automatically.)
    veml_t* VEML = (veml_t*)malloc(sizeof(veml_t));
    VEML->sda = VEML_SDA_PIN;
    VEML->scl = VEML_SCK_PIN;
    VEML->hw_inst = VEML_I2C;
    VEML->baudrate = VEML_BAUD;

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
    veml_sens_step_startup(VEML);

    // return 
    return VEML;

}

/* VEML helper for recording. Will read the raw to RGBW buffer, generate the colstring,  whilst simultaneously stepping the exposure where appropriate.*/
void veml_read_rgbw(veml_t* VEML) {
    veml_read_rgbw_raw(VEML);
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
    veml_sens_step(VEML); // step the sensitivity dependent on the current measurement (higher or lower if possible)
}

// free the VEML and all associated mallocs and shut it off using config 
void veml_free(veml_t* VEML) {

    veml_write(VEML, VEML_CONF, (uint16_t)0b0000000000000001);
    // i2c_deinit(VEML->hw_inst); // We do not- RTC needs it.
    free(VEML->colbuf);
    free(VEML->colstring);
    free(VEML);

}
