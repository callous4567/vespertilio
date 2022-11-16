// Header Guard
#ifndef IS62_SRAM_H
#define IS62_SRAM_H

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Active SRAM 
void SRAM_select(void);

// Deactive SRAM
void SRAM_deselect(void); 

/*
Set SRAM SPI to 8-bit mode.
Required before any operations in 8-bit.
*/
void SRAM_SPI8(void); 

/*
Set SRAM SPI to 16-bit mode.
Internally used for sequential read/write.
*/
void SRAM_SPI16(void); 

/* 
Set the mode of the SRAM to BYTE
Must be run before any read/write operations.
CS internal.
*/
void SRAM_byte_mode(void);

/* 
Set the mode of the SRAM to SEQUENTIAL
Must be run before any read/write operations.
CS internal.
*/
void SRAM_sequential_mode(void);

/*
Start an 8-bit sequential read
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_read(int address); 

/*
Start a 16-bit sequential read
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_read16(int address); 

/*
Start an 8-bit sequential write
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_write(int address); 

/*
Start a 16-bit sequential write
CS internal.
On termination requires SRAM_deselect().
*/
void SRAM_start_sequential_write16(int address); 

/*
Read byte at integer address.
*/
uint8_t SRAM_read_byte(int address);

/*
Write byte to integer address.
*/
void SRAM_write_byte(int address, uint8_t byte); 

/*
Zero out the SRAM module.
*/
void SRAM_zeros(void);


/*
Run an SRAM SPI DMA test. 
TODO: Return a true-false if successful. 
At the moment, prints to UART.
*/
void SRAM_DMA_TEST(void); 

#endif /* IS62_SRAM_H */