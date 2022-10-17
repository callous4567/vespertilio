#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

// Pins 
static const int SPI_RX_PIN = 16;// Yellow 
static const int SPI_CSN_PIN = 17; // White 
static const int SPI_SCK_PIN = 18; // Blue
static const int SPI_TX_PIN =  19; // Green 
static const int BAUDRATE = 22000000; // Hz 

// Define all the codes associated with reading/writing/etc. Note this is specific for the 48LM01/48L512 model. 
static const uint8_t READ = 0b00000011; // you need a write enable sent first, with cs(0)/cs(1) too.
static const uint8_t WRITE = 0b00000010;

static const uint8_t RDMR = 0b00000101; // you need a write enable sent first, with cs(0)/cs(1) too.
static const uint8_t WRMR = 0b00000001;

// Define the number of pages, bytes, pagesize, etc. Specific to the 48LM01 here. 
static const int MAX_BYTE = 262144; 
static const int MAX_PAGE = 8192;
static const int PAGE_SIZE_BYTE = 32;
static const int PAGE_SIZE_BITS = 256;

// Print as binary the individual byte a 
void toBinary(uint8_t a)
{
    uint8_t i;

    for(i=0x80;i!=0;i>>=1)
        printf("%c",(a&i)?'1':'0'); 
}

// Activate the CS pin (pull low)
static inline void cs_select() { /* declaration: https://stackoverflow.com/questions/21835664/why-declare-a-c-function-as-static-inline 
    declaring static means other modules do not see it: it's local to this module and saves effort
    on letting other modules see it.
    declaring inline means that when the file is compiled, cs_select() will be dumped inline in the code,
    and then compiled locally, rather than being pulled in some it having been compiled elsewhere.
    declaring void means that this function does not return anything at all. 
    */
    //asm volatile("nop \n nop \n nop"); // assembler-up some nops- delay code by 3 clock cycles.
    gpio_put(SPI_CSN_PIN, 0); // pull low/active
    //asm volatile("nop \n nop \n nop"); // https://forums.raspberrypi.com/viewtopic.php?t=332078
}

// Deactivate the CS pin (pull high)
static inline void cs_deselect() {
    
    //asm volatile("nop \n nop \n nop"); // assembler-up some nops- delay code by 3 clock cycles.
    gpio_put(SPI_CSN_PIN, 1); // pull high/active
    //asm volatile("nop \n nop \n nop"); // https://forums.raspberrypi.com/viewtopic.php?t=332078
}

// Set the default register settings. By default, we are setting to page mode.
static void write_default_register() {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = WRMR;
    buf[1] = 0b10000000;
    cs_select();
    int written = spi_write_blocking(spi0, buf, 2);
    cs_deselect();
    printf("Bytes written to SPI... ");
    printf("%d", written);
    printf("\r\n");
}

// Read the default register settings and print it via serial UART. 
static void read_default_register() {
    uint8_t buf[1];
    buf[0] = RDMR;
    cs_select();
    spi_write_blocking(spi0, buf, 1);
    spi_read_blocking(spi0, 0, buf, 1);
    cs_deselect();
    printf("The current read register...\r\n");
    toBinary(buf[0]);
    printf("\r\n");
}

// Initialize the main 
int main() {

    // Setup clock/etc
    //vreg_set_voltage(VREG_VOLTAGE_1_15);

    // Check clocks 
    const int current_sys = clock_get_hz(clk_sys);
    const int current_peri = clock_get_hz(clk_peri);

    // Sleep a bit for SRAM to stabilise
    sleep_ms(1000);
    stdio_init_all();

    // What is our current clock?
    printf("The current sys clock is... ");
    printf("%d", current_sys);
    printf("\r\n");
    printf("The current peri clock is... ");
    printf("%d", current_peri);
    printf("\r\n");


    // Sleep a bit for SRAM to stabilise
    sleep_ms(1000);
    stdio_init_all();

    // Initialize our SPI bus 
    int baudrate_set = spi_init(spi0, BAUDRATE);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    //hw_write_masked(&spi_get_hw(spi0)->cr0, (1 - 1) << SPI_SSPCR0_SCR_LSB, SPI_SSPCR0_SCR_BITS);
    printf("The SRAM baudrate has been set to... ");
    printf("%d", baudrate_set);
    printf("\r\n");

    // Setup pins 
    gpio_init(SPI_CSN_PIN);
    gpio_set_dir(SPI_CSN_PIN, true);

    // Set pin funcionality to the SPI bus 
    gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI); // https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__gpio.html
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI); // j is enumerated- see the documentation 
    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);

    // Write the default register, then read it back.
    read_default_register();
    write_default_register();
    read_default_register();
    write_default_register();
    read_default_register();


    // Done 
    printf("Success!\r\n");
}