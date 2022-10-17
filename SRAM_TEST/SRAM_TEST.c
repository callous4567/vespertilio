#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

// Pins 
static const int SPI_RX_PIN = 12;// Yellow 
static const int SPI_CSN_PIN = 13; // White 
static const int SPI_SCK_PIN = 14; // Blue
static const int SPI_TX_PIN =  15; // Green 
static const int BAUDRATE = 62500000; // Hz 

// Define all the codes associated with reading/writing/etc. Note this is specific for the 48LM01/48L512 model. 
static const uint8_t WREN = 0b00000110; // write enable will reset after a succesful write- no need to reset constantly. 
static const uint8_t WRDI = 0b00000100; 

static const uint8_t WRITE = 0b00000010; // you need a write enable sent first, with cs(0)/cs(1) too.
static const uint8_t READ = 0b00000011;

// Define status register and nonvolatile space
static const uint8_t WRSR = 0b00000001; // you need a write enable sent first, with cs(0)/cs(1) too.
static const uint8_t RDSR = 0b00000101;
static const uint8_t WRNUR = 0b11000010; // the nonvolatile space is only 16 bytes- use it sparingly. 
static const uint8_t RDNUR = 0b11000011;

// Hibernation command: pass this then leave the device
static const uint8_t HIBERNATE = 0b10111001; // use this to power down to power-save: no PFET will be included. 

// Define the number of pages, bytes, pagesize, etc. Specific to the 48LM01 here. 
static const int MAX_BYTE = 131072; 
static const int MAX_PAGE = 1024;
static const int PAGE_SIZE_BYTE = 128;
static const int PAGE_SIZE_BITS = 1024;

// Print as binary the individual byte a 
void toBinary(uint8_t a)
{
    uint8_t i;

    for(i=0x80;i!=0;i>>=1)
        printf("%c",(a&i)?'1':'0'); 
}

// Activate the CS pin (pull low)
static inline void cs_select() { 
    gpio_put(SPI_CSN_PIN, 0); 
}

// Deactivate the CS pin (pull high)
static inline void cs_deselect() {
    gpio_put(SPI_CSN_PIN, 1); 
}

// Set the default register settings, i.e. 0b01000000
static void write_default_register() {
    uint8_t buf[1]; 
    buf[0] = WREN;
    cs_select();
    spi_write_blocking(spi1, buf, 1); // WREN command 
    cs_deselect();
    uint8_t buffer[2];
    buffer[0] = WRSR;
    buffer[1] = 0b01000000;
    cs_select();
    spi_write_blocking(spi1, buffer, 2); // then WRSR command 
    cs_deselect();
}

// Read the default register settings and print it via serial UART. 
static void read_default_register() {
    uint8_t buf[1];
    buf[0] = RDSR;
    cs_select();
    spi_write_blocking(spi1, buf, 1);
    spi_read_blocking(spi1, 1, buf, 1);
    cs_deselect();
    printf("The current read register...\r\n");
    toBinary(buf[0]);
    printf("\r\n");
}

// Write a single byte to a register 
static void write_byte(int address, uint8_t byte) {
    uint8_t buf[1];
    buf[0] = WREN;
    cs_select();
    spi_write_blocking(spi1, buf, 1);
    cs_deselect();
    uint8_t bytebuffer[4];
    bytebuffer[0] = WRITE;
    bytebuffer[1] = 0b1111111 | address >> 16;
    bytebuffer[2] = address >> 8;
    bytebuffer[3] = address;
    cs_select();
    spi_write_blocking(spi1, bytebuffer, 4);
    cs_deselect();
}

// Read a single byte from the register 
static uint8_t read_byte(int address) {
    uint8_t bytebuffer[4];
    bytebuffer[0] = READ;
    bytebuffer[1] = 0b1111111 | (address >> 16);
    bytebuffer[2] = address >> 8;
    bytebuffer[3] = address; 
    uint8_t databuffer[1];
    cs_select();
    spi_write_blocking(spi1, bytebuffer, 4);
    spi_read_blocking(spi1, 1, databuffer, 1);
    cs_deselect();
    return databuffer[0];
}

// Initialize the main 
int main() {

    // Sleep a bit for SRAM to stabilise
    sleep_ms(1000);
    stdio_init_all();

    // Sleep a bit for SRAM to stabilise
    sleep_ms(1000);
    stdio_init_all();

    // Initialize our SPI bus 
    spi_init(spi1, BAUDRATE);
    spi_set_format(spi1, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

    // Setup pins 
    gpio_init(SPI_CSN_PIN);
    gpio_set_dir(SPI_CSN_PIN, true);

    // Set pin funcionality to the SPI bus 
    gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI); 
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI); 
    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);

    // Write the default register, then read it back.
    write_default_register();
    read_default_register();

    // Test something out
    write_byte(128, 0b11110000);
    uint8_t byteread = read_byte(128);
    printf("When we tried to read the byte back ... ");
    toBinary(byteread);
    printf("\r\n");

    // Done 
    printf("Finito Bonito!\r\n");
}
