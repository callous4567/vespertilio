#ifndef EXT_IO_REGS
#define EXT_IO_REGS

#include "../Utilities/pinout.h"

// Note the presence of the interrupt pin for this particular RTC
extern const int IO_BAUD;

// And the i2c, defined inside pinout.h
extern i2c_inst_t* IO_I2C;

// The slave address for I2C, equal to (0b1=Read,0b0=Write)|(0b1101000=Slave)
extern const uint8_t IO_SLAVE_READ_ADDRESS, IO_SLAVE_WRITE_ADDRESS;

/*
Shed load of registers 
*/
extern const uint8_t IO_INPUT,
IO_OUTPUT,
IO_POLARITY,
IO_CONFIGURE;

#endif /* EXT_IO_REGS */