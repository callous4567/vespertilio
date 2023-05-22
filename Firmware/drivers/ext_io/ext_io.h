// Header Guard
#ifndef EXT_IO_H
#define EXT_IO_H

#include "ext_io_registers.h"
#include "hardware/i2c.h"

typedef struct {

    // Pins
    int sda; 
    int scl;
    
    // I2C
    i2c_inst_t *hw_inst; 
    int baudrate; 

} ext_io_t;

void io_write_command_byte(
    ext_io_t* EXT_IO,
    uint8_t COMMAND,
    uint8_t VALUE
);

uint8_t io_read_command_byte(
    ext_io_t* EXT_IO,
    uint8_t COMMAND
);

#endif /* EXT_IO_H */