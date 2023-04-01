#ifndef VEML_REGS
#define VEML_REGS

#include "hardware/i2c.h"

extern const int32_t VEML_INT_PIN, VEML_SDA_PIN, VEML_SCK_PIN, VEML_BAUD;
extern i2c_inst_t* VEML_I2C;

// configuration register and data registers (all 16-bit.) 
extern const uint8_t VEML_SLAVE, VEML_CONF, 
VEML_R, VEML_G, VEML_B, VEML_W;


#endif // VEML_REGS