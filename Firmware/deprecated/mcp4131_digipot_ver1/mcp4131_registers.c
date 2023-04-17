#include "mcp4131_registers.h"

// pins 
const int32_t DPOT_SCK_PIN = 18;
const int32_t DPOT_MOSI_PIN = 19;
const int32_t DPOT_CSN_PRIMARY = 17; // first digipot on the cascade 
const int32_t DPOT_CSN_SECONDARY = 21; // second on the cascade (deprecated in ver. 3)
const int32_t DPOT_MISO_PIN = 16;

/*
Quick design note about our cascaded op-amp.

On the primary digipot, the microphone is fed into P0A, 
the wiper is then fed into the inverting input of the primary,
P0B is connected to the output of the first stage of the cascade
as the feedback 

In this case, the gain is equal to -(P0B - W)/(W - P0A)

On the secondary digipot, output from the first stage is into P0A
the wiper to the inverting input of the second stage, 
P0B is connected to the output of the second stage of the cascade
as the feedback

In this case, the gain is equal to -(P0B - W)/(W - P0A)

*/

// the spi instance 
spi_inst_t* DPOT_SPI = spi0;

// baud 
int32_t DPOT_BAUD = 50*1000;

// memory map
const uint16_t DPOT_WIPER = 0x0; // we are using the MCP4131- one wiper.
const uint16_t DPOT_STATUS = 0x05; 

// maximum number of taps on the pot (iterate 0 through 128. 129 is invalid)
const int32_t max_tap = 128; 
