// Header Guard
#ifndef IS62_SRAM_H
#define IS62_SRAM_H

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
Some quick notes:
- I have not included any error catches: if you fuck up by calling an address that does not exist on the SRAM, overwriting it, etc, that isn't my problem.
- I am very new to C/C++- in fact this is my first C/C++ project. This code works- I have not checked if the types or anything are right though. I am a Python man, learning C.
- Make an "Issue" if there is something you are not sure about and I will try to help. Again though- I am a Python man, not a C man. 
*/

// Some default values. The struct is there to make it more general for multiple SRAM or etc. 
const int SRAM_SPI_RX_PIN = 12;// Yellow 
const int SRAM_SPI_CS_PIN = 13; // White 
const int SRAM_SPI_SCK_PIN = 14; // Blue
const int SRAM_SPI_TX_PIN =  15; // Green 
const int SRAM_BAUDRATE = 22000000; // Hz 
const uint8_t SRAM_READ = 0b00000011; 
const uint8_t SRAM_WRITE = 0b00000010;
const uint8_t SRAM_RDMR = 0b00000101; 
const uint8_t SRAM_WRMR = 0b00000001;
const int SRAM_MAXBYTE = 262144; 
const int SRAM_MAXPAGE = 8192;
const int SRAM_PAGE_SIZE_BYTES = 32;
const int SRAM_BITS_PER_BYTE = 8;

/*
Struct with the important bits of the SRAM that we may need elsewhere,
including the pin controls,
DMA channel information,
spi instance,
that sort of thing.
all write/read commands are still kept to the SRAM driver separately obviously- just need 
*/
typedef struct {

    // Standard variables for the SRAM 
    spi_inst_t *hw_inst; // spi instance spi0 or spi1 
    int rx_pin; // gpio pin not the actual pinout pin 
    int cs_pin;
    int sck_pin;
    int tx_pin;
    int baudrate; // in hz 
    int maxbyte; 
    int maxpage; 
    int pagesize; // in bytes 
    int bytesize; // the number of bits to a single byte (i.e. 8 here) according to the SRAM module 

} sram_t;

// Active SRAM 
void SRAM_select(sram_t *SRAM);

// Deactive SRAM
void SRAM_deselect(sram_t *SRAM); 

/*
Set SRAM SPI to 8-bit mode.
Required before any operations in 8-bit.
*/
void SRAM_SPI8(sram_t *SRAM); 

/*
Set SRAM SPI to 16-bit mode.
Internally used for sequential read/write.
*/
void SRAM_SPI16(sram_t *SRAM); 

/* 
Set the mode of the SRAM to BYTE
Must be run before any read/write operations.
CS internal.
*/
void SRAM_byte_mode(sram_t *SRAM);

/* 
Set the mode of the SRAM to SEQUENTIAL
Must be run before any read/write operations.
CS internal.
*/
void SRAM_sequential_mode(sram_t *SRAM);

/*
Start an 8-bit sequential read
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_read(sram_t *SRAM, int address); 

/*
Start a 16-bit sequential read
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_read16(sram_t *SRAM, int address); 

/*
Start an 8-bit sequential write
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_write(sram_t *SRAM, int address); 

/*
Start a 16-bit sequential write
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_write16(sram_t *SRAM, int address); 

/*
Read byte at integer address.
*/
uint8_t SRAM_read_byte(sram_t *SRAM, int address);

/*
Write byte to integer address.
*/
void SRAM_write_byte(sram_t *SRAM, int address, uint8_t byte); 

/*
Zero out the SRAM module.
*/
void SRAM_zeros(sram_t *SRAM);

/*
Initialize the SRAM appropriately ready for usage, 
i.e. set up GPIO and etc.
*/
void SRAM_init(sram_t *SRAM);

/*
Run an SRAM SPI DMA test. 
TODO: Return a true-false if successful. 
At the moment, prints to UART.
*/
void SRAM_DMA_TEST(sram_t *SRAM); 

// Write bufflen from *buff starting at address in the SRAM... 8-bit data
void SRAM_start_sequential_write_buffer(sram_t *SRAM, int address, uint8_t *buff, int bufflen);

// Write bufflen from *buff starting at address in the SRAM... 16-bit data 
void SRAM_start_sequential_write16_buffer(sram_t *SRAM, int address, uint16_t *buff, int bufflen);

// Read bufflen to *buff starting at address in the SRAM... 8-bit data
void SRAM_start_sequential_read_buffer(sram_t *SRAM, int address, uint8_t *buff, int bufflen);

// Read bufflen to *buff starting at address in the SRAM... 16-bit data 
void SRAM_start_sequential_read16_buffer(sram_t *SRAM, int address, uint16_t *buff, int bufflen);



#endif /* IS62_SRAM_H */