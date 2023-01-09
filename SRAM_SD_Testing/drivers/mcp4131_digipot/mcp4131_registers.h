#ifndef MCP4131_REGS
#define MCP4131_REGS

#include "hardware/spi.h"

extern const int32_t DPOT_SCK_PIN,
DPOT_MOSI_PIN,
DPOT_CSN_PRIMARY,
DPOT_CSN_SECONDARY,
DPOT_MISO_PIN;

extern spi_inst_t* DPOT_SPI;

extern int32_t DPOT_BAUD;

extern const uint16_t DPOT_WIPER,
DPOT_STATUS; 

extern const int32_t max_tap;

#endif // MCP4131_REGS