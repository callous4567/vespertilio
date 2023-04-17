#ifndef VEML_REGS
#define VEML_REGS

#include <stdint.h>

extern const int32_t VEML_BAUD;

// configuration register and data registers (all 16-bit.) 
extern const uint8_t VEML_SLAVE, VEML_CONF, 
VEML_R, VEML_G, VEML_B, VEML_W;


#endif // VEML_REGS