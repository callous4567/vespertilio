// Header Guard
#ifndef IS62_SRAM_H
#define IS62_SRAM_H

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// Pico includes
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/mutex.h"
#include "pico/sem.h"
#include "pico/types.h"
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

    // Pins
    int rx_pin; // gpio pin not the actual pinout pin 
    int cs_pin;
    int sck_pin;
    int tx_pin;

    // Spi 
    spi_inst_t *hw_inst; // spi instance spi0 or spi1 
    int baudrate; // in hz 

    // DMA (specific to our project.) Claim three channels- SRAM->BUFA, SRAM->BUFB, ADC->SRAM
    int sram_a_chan;
    dma_channel_config sram_a_conf;
    int sram_b_chan;
    dma_channel_config sram_b_conf;
    int adc_sram_chan;
    dma_channel_config adc_sram_conf;

} sram_t;

// Active SRAM 
void SRAM_select(sram_t *SRAM);

// Deactive SRAM
void SRAM_deselect(sram_t *SRAM); 

// init the SRAM pin-wise for our project. 
void SRAM_init(sram_t *SRAM);

// init the SRAM dma configuration for our project. 
void SRAM_dma_init(sram_t *SRAM, uint16_t *BUF_A, uint16_t *BUF_B, int *BUF_SIZE);

// Get the hardware instance corresponding to this particular SRAM's spi
spi_hw_t* get_SPI_hw_instance(sram_t *SRAM);

// set sram spi to 8-bit
void SRAM_SPI8(sram_t *SRAM); 

// set sram spi to 16-bit
void SRAM_SPI16(sram_t *SRAM); 

// set sram to byte mode 
void SRAM_byte_mode(sram_t *SRAM);

// set sram to sequential mode 
void SRAM_sequential_mode(sram_t *SRAM);

// set sram to start an 8-bit sequential read at address. note that you must deselect once complete.
void SRAM_start_sequential_read(sram_t *SRAM, int address); 

// set sram to start a 16-bit sequential read at address. must call SRAM_SPI8() and SRAM_DESELECT() at end. 
void SRAM_start_sequential_read16(sram_t *SRAM, int address); 

// set sram to start an 8-bit sequential write at address. note that you must deselect once complete.
void SRAM_start_sequential_write(sram_t *SRAM, int address); 

// set sram to start a 16-bit sequential write at address. must call SRAM_SPI8() and SRAM_DESELECT() at end. 
void SRAM_start_sequential_write16(sram_t *SRAM, int address); 

// read byte at address 
uint8_t SRAM_read_byte(sram_t *SRAM, int address);

// write byte to address 
void SRAM_write_byte(sram_t *SRAM, int address, uint8_t byte); 

// zero the SRAM 
void SRAM_zeros(sram_t *SRAM);

// run a DMA test 
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