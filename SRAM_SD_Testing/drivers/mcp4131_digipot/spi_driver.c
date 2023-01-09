#include "spi_driver.h"

// select either potentiometer (pull down)
void dpot_select(dpot_dual_t* DPOT, int32_t which) {
    if (which==(int32_t)1) {
        gpio_put(DPOT->CSN_1, 0);
    } else {
        gpio_set_pulls(DPOT->CSN_2, false, true);
        gpio_put(DPOT->CSN_2, 0);
    }
}

// deselect either potentiometer (pull up)
void dpot_deselect(dpot_dual_t* DPOT, int32_t which) {
    if (which==(int32_t)1) {
        gpio_put(DPOT->CSN_1, 1);
    } else {
        gpio_put(DPOT->CSN_2, 1);
    }
}

// set up CSN pins in default state. need pullups (see PCB revision)
static inline void default_csn(dpot_dual_t* DPOT) {

    // Setup pins for CSN
    gpio_init(DPOT->CSN_1);
    gpio_set_dir(DPOT->CSN_1, true);
    gpio_put(DPOT->CSN_1, 1);
    gpio_init(DPOT->CSN_2);
    gpio_set_dir(DPOT->CSN_2, true);
    gpio_put(DPOT->CSN_2, 1);
    
}

// initialize the dpot object correctly/appropriately + set up SPI
dpot_dual_t* init_dpot(void) {

    // init the object 
    dpot_dual_t* DPOT = (dpot_dual_t*)malloc(sizeof(dpot_dual_t));
    DPOT->SCK_PIN = DPOT_SCK_PIN;
    DPOT->MOSI_PIN = DPOT_MOSI_PIN;
    DPOT->CSN_1 = DPOT_CSN_PRIMARY;
    DPOT->CSN_2 = DPOT_CSN_SECONDARY;
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
void dpot_read_tap(dpot_dual_t* DPOT, int32_t which) {

    // craft our command (wiper address + command bits)
    uint16_t command_dual = (DPOT_WIPER<<12) | 0b0000111111111111; 

    // placeholder for return
    uint16_t return_dual;

    // select appropriate dpot 
    dpot_select(DPOT, which);

    // let's do!
    spi_write16_read16_blocking(
        DPOT->hw_inst,
        &command_dual,
        &return_dual,
        1
    );

    // deselect
    dpot_deselect(DPOT, which);

    // isolate the last 9 bits of the return dual (see DS)
    toBinary_16(return_dual);
    return_dual = 0b0000000111111111 & return_dual;

    // to binary it!
    toBinary_16(return_dual);

    // set the tap
    if (which==(int32_t)1) {
        DPOT->TAP_1 = (int32_t)return_dual;
    } else {
        DPOT->TAP_2 = (int32_t)return_dual;
    }

}

// NOTE!!! THE WIPER IS SCALED FROM TERMINAL B, NOT TERMINAL A!!!
void dpot_write_tap(dpot_dual_t* DPOT, int32_t which, int32_t value) {

    // panic if wiper exceeds max tap
    if (value > max_tap) {
        panic("Your wiper value for the digipot has exceeded max tap bro.");
    }

    // craft our command (wiper address + command bits)
    uint16_t command_dual = (DPOT_WIPER<<12) | 0b0000000000000000; // first the wiper address
    command_dual = command_dual | value; // next the value  

    // select appropriate dpot 
    dpot_select(DPOT, which);

    // let's do!
    spi_write16_blocking(
        DPOT->hw_inst,
        &command_dual,
        1
    );

    // deselect
    dpot_deselect(DPOT, which);

    // set the DPOT tap (we just assume everything worked out fine.)
    if (which==(int32_t)1) {
        DPOT->TAP_1 = value;
    } else {
        DPOT->TAP_2 = value;
    }

}

// get status register of given DPOT 
void dpot_read_status(dpot_dual_t* DPOT, int32_t which) {

    // craft our command (wiper address + command bits)
    uint16_t command_dual = (DPOT_STATUS<<12) | 0b0000111111111111; 

    // placeholder for return
    uint16_t return_dual;

    // select appropriate dpot 
    dpot_select(DPOT, which);

    // let's do!
    spi_write16_read16_blocking(
        DPOT->hw_inst,
        &command_dual,
        &return_dual,
        1
    );

    // deselect
    dpot_deselect(DPOT, which);

    // isolate the last 9 bits of the return dual (see DS)
    return_dual = 0b0000000111111111 & return_dual;

    // to binary it!
    toBinary_16(return_dual);

}

// set a gain for a given resistor. Note that wiper scales from terminal B!!!
void dpot_set_gain(dpot_dual_t* DPOT, int32_t which, int32_t gain) {

    /*
    GAIN FORMULA
    ============

    (MAX_TAP - WIPER)/(WIPER - 0) = MAX/WIPER - 1 = G
    MAX/WIPER = G + 1
    WIPER = MAX/(G+1)

    WIPER ZEROTH IS AT TERMINAL B NOT A!
    */ 

    int32_t wiper_value = max_tap/(gain+1);
    wiper_value = max_tap - wiper_value;
    dpot_write_tap(DPOT, which, wiper_value);

    if (which==(int32_t)1) { 
        DPOT->TAP_1 = wiper_value;
        DPOT->GAIN_1 = gain;
    } else {
        DPOT->TAP_2 = wiper_value;
        DPOT->GAIN_2 = gain;
    }

}

