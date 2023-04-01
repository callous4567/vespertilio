#include "spi_driver.h"
#include "hardware/gpio.h"
#include "../Utilities/utils.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/**
 *  IMPORTANT TODO NOTE 
 * This driver needs some optimization
 * We need to clean up our <-> and struct for the potentiometer
 * Note that we are now using just a single potentiometer (instead of 2) and just on the inverting amplifier (singular.)
 * 
 * 
*/

// select either potentiometer (pull down)
static void dpot_select(dpot_dual_t* DPOT) {
    gpio_put(DPOT->CSN, 0);
}

// deselect either potentiometer (pull up)
static void dpot_deselect(dpot_dual_t* DPOT) {
    gpio_put(DPOT->CSN, 1);
}

// set up CSN pins in default state. need pullups (see PCB revision)
static inline void default_csn(dpot_dual_t* DPOT) {

    // Setup pins for CSN
    gpio_init(DPOT->CSN);
    gpio_set_dir(DPOT->CSN, true);
    gpio_put(DPOT->CSN, 1);
    
}

// initialize the dpot object correctly/appropriately + set up SPI
dpot_dual_t* init_dpot(void) {

    // init the object 
    dpot_dual_t* DPOT = (dpot_dual_t*)malloc(sizeof(dpot_dual_t));
    DPOT->SCK_PIN = DPOT_SCK_PIN;
    DPOT->MOSI_PIN = DPOT_MOSI_PIN;
    DPOT->CSN = DPOT_CSN;
    DPOT->MISO_PIN = DPOT_MISO_PIN;
    DPOT->hw_inst = DPOT_SPI;
    DPOT->baudrate = DPOT_BAUD;

    // disable all pulls appropriately (we are using externals)
    default_csn(DPOT);

    // set functions 
    gpio_set_function(DPOT->SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(DPOT->MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(DPOT->MISO_PIN, GPIO_FUNC_SPI);

    // init spi. we are using 16-bit-mode since we only do read/write commands- no increments.
    spi_init(DPOT->hw_inst, DPOT->baudrate);
    spi_set_format(DPOT->hw_inst, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // return
    return DPOT;

}

// deinitialize the dpot object correctly/appropriately + set up SPI
void deinit_dpot(dpot_dual_t* DPOT) {

    // set functions 
    gpio_set_function(DPOT->SCK_PIN, GPIO_FUNC_NULL);
    gpio_set_function(DPOT->MOSI_PIN, GPIO_FUNC_NULL);
    gpio_set_function(DPOT->MISO_PIN, GPIO_FUNC_NULL);

    // deinit spi. 
    spi_deinit(DPOT->hw_inst);

    // free 
    free(DPOT);
    
}

/*
some tidbits on register
16-bit commands are made up of:
4 bits of memory address
2 command bits
then 10 bits of data (i.e. data read or written)
the command bits are 00 for writing, 11 for reading. 
you need to read/write simultaneously. 
note that in writes, the trailing 10-bits of data must be high, i.e. 0bXXXXXX ... 0b1111111111
*/
void dpot_read_tap(dpot_dual_t* DPOT) {

    // craft our command (wiper address + command bits)
    uint16_t command_dual = (DPOT_WIPER<<12) | 0b0000111111111111; 

    // placeholder for return
    uint16_t return_dual;

    // select appropriate dpot 
    dpot_select(DPOT);

    // let's do!
    spi_write16_read16_blocking(
        DPOT->hw_inst,
        &command_dual,
        &return_dual,
        1
    );

    // deselect
    dpot_deselect(DPOT);

    // isolate the last 9 bits of the return dual (see DS)
    //toBinary_16(return_dual);
    return_dual = 0b0000000111111111 & return_dual;

    // to binary it!
    //toBinary_16(return_dual);
    printf("Tap reading %d\r\n", return_dual);

    // set the tap
    DPOT->TAP = (int32_t)return_dual;


}

// NOTE!!! THE WIPER IS SCALED FROM TERMINAL B, NOT TERMINAL A!!!
static void dpot_write_tap(dpot_dual_t* DPOT, int32_t value) {

    // panic if wiper exceeds max tap
    if (value > max_tap) {
        panic("Your wiper value for the digipot has exceeded max tap bro.");
    }

    // craft our command (wiper address + command bits)
    uint16_t command_dual = (DPOT_WIPER<<12) | 0b0000000000000000; // first the wiper address
    command_dual = command_dual | value; // next the value  

    // select appropriate dpot 
    dpot_select(DPOT);

    // let's do!
    spi_write16_blocking(
        DPOT->hw_inst,
        &command_dual,
        1
    );

    // deselect
    dpot_deselect(DPOT);

    // set the DPOT tap (we just assume everything worked out fine.)
    DPOT->TAP = value;

    // double check
    // dpot_read_tap(DPOT);

}

// get status register of given DPOT 
void dpot_read_status(dpot_dual_t* DPOT) {

    // craft our command (wiper address + command bits) : PAGE 47 OF THE DATASHEET https://www.mouser.co.uk/datasheet/2/268/MCHPS02708_1-2520528.pdf
    // first 4 bits are the address in memory, the next two are the command bits (00 is write and 11 is read) and the last 12 bits are the data bits, all set to unity/null for a read
    uint16_t command_dual = (DPOT_STATUS<<12) | 0b0000111111111111; 

    // placeholder for return
    uint16_t return_dual;

    // select appropriate dpot 
    dpot_select(DPOT);

    // let's do!
    spi_write16_read16_blocking(
        DPOT->hw_inst,
        &command_dual,
        &return_dual,
        1
    );

    // deselect
    dpot_deselect(DPOT);

    // isolate the last 9 bits of the return dual (see DS)
    return_dual = 0b0000000111111111 & return_dual;

    // to binary it!
    toBinary_16(return_dual);

}

// set a gain for a given resistor. Note that wiper scales from terminal B!!!
void dpot_set_gain(dpot_dual_t* DPOT, int32_t gain) {

    /*
    GAIN FORMULA (for the inverting amplifier, with the MCP4131 set up as a varistor between P0A and the wiper, with 1k as the non-feedback-resistor)
    ============

    wiper_value scales from 0 at terminal B to max_tap - 1 at terminal A 
    (max_tap - wiper_value) = (wiper_distance_from_A)
    (wiper_distance_from_A / max_tap) = (R_FEEDBACK/50K)
    50K*(wiper_distance_from_A / max_tap) = R_FEEDBACK 
    (50K/1K)*(wiper_distance_from_A / max_tap) = R_FEEDBACK/1K = GAIN = 50(wiper_distance_from_A / max_tap)
    GAIN * MAX_TAP / 50 = max_tap - wiper_value
    wiper_value = max_tap(1 - gain/50)
    */ 

    int32_t wiper_value = (max_tap * (1000 - (1000*gain/50))) / 1000;
    dpot_write_tap(DPOT, wiper_value);

    DPOT->TAP = wiper_value;
    DPOT->GAIN = gain;


}

