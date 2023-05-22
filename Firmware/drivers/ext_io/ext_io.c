#include "ext_io.h"
#include "ext_io_registers.h"
#include "../Utilities/utils.h"
#include "hardware/gpio.h"
#include "malloc.h"

// write the provided value to the command register on the IO
void io_write_command_byte(
    ext_io_t* EXT_IO,
    uint8_t COMMAND,
    uint8_t VALUE
) {
    uint8_t buf[2];
    buf[1] = COMMAND;
    buf[2] = VALUE;

    i2c_write_blocking(
        EXT_IO->hw_inst,
        IO_SLAVE_WRITE_ADDRESS,
        buf,
        2,
        false
    );

}

// read the provided command register on the IO
uint8_t io_read_command_byte(
    ext_io_t* EXT_IO,
    uint8_t COMMAND
) {

    i2c_write_blocking(
        EXT_IO->hw_inst,
        IO_SLAVE_WRITE_ADDRESS,
        &COMMAND,
        1,
        true
    );

    uint8_t val;

    i2c_read_blocking(
        EXT_IO->hw_inst,
        IO_SLAVE_READ_ADDRESS,
        &val,
        1,
        false
    );

    return val;

}