#ifndef MCP4131_SPI
#define MCP4131_SPI

#include "mcp4131_registers.h"
#include "hardware/spi.h"

typedef struct {

    // Pins
    int32_t SCK_PIN;
    int32_t MOSI_PIN;
    int32_t CSN;
    int32_t MISO_PIN;
    
    // SPI
    spi_inst_t *hw_inst; 
    int32_t baudrate; 

    // Taps values (to set, or read.)
    int32_t TAP;

    // Gains (individual + total.)
    int32_t GAIN; 

} dpot_dual_t; // THIS IS MALLOC'D!!!


// grab the DPOT object necessary to control gain of our cascade 
dpot_dual_t* init_dpot(void);

// deinit the dpot 
void deinit_dpot(dpot_dual_t* DPOT);

// read the value of the tap to the DPOT 
//void dpot_read_tap(dpot_dual_t* DPOT, int32_t which);

// write the value to the tap
//void dpot_write_tap(dpot_dual_t* DPOT, int32_t which, int32_t value);

// get status register of given DPOT 
//void dpot_read_status(dpot_dual_t* DPOT, int32_t which);

// set the gain on the dpot cascade stage 
void dpot_set_gain(dpot_dual_t* DPOT, int32_t gain);

void dpot_read_status(dpot_dual_t* DPOT);

void dpot_read_tap(dpot_dual_t* DPOT);
#endif // MCP4131_SPI

