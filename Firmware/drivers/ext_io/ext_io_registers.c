#include "ext_io_registers.h"

// Note the presence of the interrupt pin for this particular RTC
const int IO_BAUD = 400000;

// The address of the RTC. 0x68. The I2C bus handles the addition of the read or write bits (1 and 0.) 
const uint8_t IO_SLAVE_READ_ADDRESS = 0b10000011;
const uint8_t IO_SLAVE_WRITE_ADDRESS = 0b10000010;

// Control register command bytes 
const uint8_t IO_INPUT = 0x00; // read only- the actual value on pin (0/1)
const uint8_t IO_OUTPUT = 0x01; // read/write- setting for IO output log level (0/1)
const uint8_t IO_POLARITY = 0x02; // read/write- not applicable- default set to 0x00!!!
const uint8_t IO_CONFIGURE = 0x03;  // read/write- directionality. 0 is output, 1 is input

/* === REGISTER DESCRIPTIONS ===
Example byte from ANY of the above command registers:
    0b7654321
Bit 7,6,5,4 are unused for 4-bit expander !!!SHOULD BE SET TO ONES!!!
Bit 4,3,2,1 are IO pins 3,2,1,0 in order according to the pinout 

So, to set IO3 to an output with logic level 1, you would simply 
execute the IO_CONFIGURE command and write:
    0b11110111
Following this, you would then execute the IO_OUTPUT command and
then write:
    0b11111000

Note that if a pin is configured as an input, but you change output,
sweet fuck all happens- same for changing the output for a pin
configured as an input.
*/

// See pinout.h and etc for versioning on the pin configuration/setting IO hw_inst
