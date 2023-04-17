#include "veml_registers.h"
#include "../Utilities/pinout.h"

// pins/i2c same as RTC 
const int32_t VEML_BAUD = 400000;

/*
https://datasheet.lcsc.com/lcsc/1811101814_Vishay-Intertech-VEML6040A3OG_C78465.pdf
VEML6040 DATASHEET Vishay Semiconductors
*/

// configuration register and data registers (all 16-bit.) 
const uint8_t VEML_SLAVE = 0x10;
const uint8_t VEML_CONF = 0x00;
const uint8_t VEML_R = 0x08;
const uint8_t VEML_G = 0x09;
const uint8_t VEML_B = 0x0A;
const uint8_t VEML_W = 0x0B;

/*

The command register is made up of 16 bits

The first 8 bits are 0:7 data byte low
7: 0
6,5,4: IT2,1,0 = integration time. page 8. scales in factors of 2, with 0x000 being 40 milliseconds, up to 0x101 being 1280 milliseconds (40,80,160,320,640,1280)
3: 0 
2: TRIG = proceed a single cycle in manual force mode. we're using auto mode (so set this to 0.)
1: AF = auto/manual force mode (set to 0 for auto mode.)
0: SD = chip shutdown setting  (0,1 enables/disables color sensing. set to 0 to ensure the sensor is turned on, else set to 1 to turn off.)

The next 8 bits are 8:15 data byte high 
These are reserved for something else- who cares (just set to 0x00)

For IT2,1,0 note that...
0x000 = 40   ms Decimal 0
0x001 = 80   ms Decimal 1 
0x010 = 160  ms Decimal 2 
0x011 = 320  ms Decimal 3
0x100 = 640  ms Decimal 4
0x101 = 1280 ms Decimal 5
*/

/*

Note that the data registers are 16-bit
This means some finesse is needed.

To write:
<Start> SLAVE ADDRESS <Ack> COMMAND <Ack> Data Byte Low <Ack> Data Byte High <Ack & Stop>

To read:
<Start> SLAVE ADDRESS <Ack> COMMAND <Ack & Start, i.e. restart> SLAVE ADDRESS <Ack> Data Byte Low <Ack> Data Byte High <Ack & Stop>

The read has a restart after the command (as opposed to the write!!!) Check the datasheet.

*/