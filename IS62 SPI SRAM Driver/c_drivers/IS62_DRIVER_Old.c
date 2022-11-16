#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "hardware/dma.h"

// All the SRAM parameters 
static const int SPI_RX_PIN = 12;// Yellow 
static const int SPI_CSN_PIN = 13; // White 
static const int SPI_SCK_PIN = 14; // Blue
static const int SPI_TX_PIN =  15; // Green 
static const int BAUDRATE = 22000000; // Hz 
spi_inst_t *SPI = spi1; // which SPI 
static const uint8_t READ = 0b00000011; 
static const uint8_t WRITE = 0b00000010;
static const uint8_t RDMR = 0b00000101; 
static const uint8_t WRMR = 0b00000001;
static const int MAX_BYTE = 262144; 
static const int MAX_PAGE = 8192;
static const int PAGE_SIZE_BYTES = 32;
static const int PAGE_SIZE_BITS = 256;

// The ADC parameters. Note that we output ADC samples as 16-bit values (12bit -> 16bit.)
static const int ADC_PIN = 26;
static const int ADC_SAMPLE_RATE = 192000;
static const int ADC_TRANSFER_SIZE = 512; // 256 transfers equals 512 bytes equals 16 pages. 

// Print as binary the individual byte a 
static void toBinary(uint8_t a) {
    uint8_t i;

    for(i=0x80;i!=0;i>>=1)
        printf("%c",(a&i)?'1':'0'); 
    printf("\r\n");
}
static void toBinary_16(uint16_t a) {
    uint16_t i;

    for (int i = 0; i < 16; i++) {
        printf("%d", (a & 0x8000) >> 15);
        a <<= 1;
    }
    printf("\r\n");
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
static inline void cs_deselect() {
    
    //asm volatile("nop \n nop \n nop"); // assembler-up some nops- delay code by 3 clock cycles.
    gpio_put(SPI_CSN_PIN, 1); // pull high/active
    //asm volatile("nop \n nop \n nop"); // https://forums.raspberrypi.com/viewtopic.php?t=332078
}

// Test writing/reading the register- use before read_default_register(). Default PAGE MODE.
static void write_default_register() {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = WRMR;
    buf[1] = 0b10000000;
    cs_select();
    int written = spi_write_blocking(SPI, buf, 2);
    cs_deselect();
    printf("Bytes written to SPI... ");
    printf("%d", written);
    printf("\r\n");
}
static void read_default_register() {
    uint8_t buf[1];
    buf[0] = RDMR;
    cs_select();
    spi_write_blocking(SPI, buf, 1);
    spi_read_blocking(SPI, 0, buf, 1);
    cs_deselect();
    printf("The current read register...\r\n");
    toBinary(buf[0]);
}

// Set mode
static inline void byte_mode() {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = WRMR;
    buf[1] = 0b00000000;
    cs_select();
    spi_write_blocking(SPI, buf, 2);
    cs_deselect();
}
static inline void page_mode() {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = WRMR;
    buf[1] = 0b10000000;
    cs_select();
    spi_write_blocking(SPI, buf, 2);
    cs_deselect();
}
static inline void sequential_mode() {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = WRMR;
    buf[1] = 0b01000000;
    cs_select();
    spi_write_blocking(SPI, buf, 2);
    cs_deselect();
}

// Set the SPI mode to 8 bit or 16 bit as required below
static inline void SPI8() {
    spi_set_format(SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}
static inline void SPI16() {
    spi_set_format(SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

// Start sequentials. Note that you need to call SPI8() at the end of any 16-bit process for future commands (8 bit) to work. When in 16-bit, you have half of MAX_BYTE/PAGE for indices. 
static inline void start_sequential_read(int address) {
    uint8_t buf[4];
    buf[0] = READ;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    cs_select();
    spi_write_blocking(SPI, buf, 4);
}
static inline void start_sequential_read16(int address) {
    uint8_t buf[4];
    buf[0] = READ;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    cs_select();
    spi_write_blocking(SPI, buf, 4);
    SPI16();
}
static inline void start_sequential_write(int address) {
    uint8_t buf[4];
    buf[0] = WRITE;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    cs_select();
    spi_write_blocking(SPI, buf, 4);
}
static inline void start_sequential_write16(int address) {
    uint8_t buf[4];
    buf[0] = WRITE;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    cs_select();
    spi_write_blocking(SPI, buf, 4);
    SPI16();
}

// Read a certain byte
static inline uint8_t read_byte(int address) {
    uint8_t buf[4];
    uint8_t read_buf[1];
    buf[0] = READ;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address;
    cs_select();
    spi_write_blocking(SPI, buf, 4);
    spi_read_blocking(SPI, 0x00, read_buf, 1);
    cs_deselect();
    return read_buf[0];
}
static inline uint8_t write_byte(int address, uint8_t byte) {
    uint8_t buf[5];
    buf[0] = WRITE;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address;
    buf[4] = byte;
    cs_select();
    spi_write_blocking(SPI, buf, 5);
    cs_deselect();
}

// Set the entire SRAM unit to ZERO 
static void SRAM_zeros() {
    sequential_mode();
    int i = 0;
    uint8_t zero_page[512]; // 16 pages 
    while (i<512) {
        zero_page[i] = 0b00000000;
        i += 1;
    }
    i = 0;
    int n_page_writes = (MAX_PAGE/16) - 1; 
    start_sequential_write(0);
    while (i <= n_page_writes) {
        spi_write_blocking(SPI, zero_page, 512);
        i += 1; 
    }
    cs_deselect();
}
static void SRAM_ones() {
    /*
    // Do a sequential test... (it works fine!) 
    printf("Now sequential mode tests! \r\n");
    SRAM_zeros();
    byte_mode();
    uint8_t TT1 = read_byte(MAX_BYTE-1);
    toBinary(TT1);
    SRAM_ones();
    byte_mode();
    uint8_t TT2 = read_byte(MAX_BYTE-1);
    toBinary(TT2);
    */
    int i = 0;
    uint8_t one_page[512]; // 16 pages
    while (i<512) {
        one_page[i] = 0b11111111;
        i += 1;
    }
    i = 0;
    int n_page_writes = (MAX_PAGE/16) - 1; 
    sequential_mode();
    start_sequential_write(0);
    while (i <= n_page_writes) {
        spi_write_blocking(SPI, one_page, 512);
        i += 1; 
    }
    cs_deselect();
}
static void SRAM_decimals() { 
    int i = 0;
    uint8_t buf[1];
    sequential_mode();
    start_sequential_write(0);
    while (i<513) {
        write_byte(i, i);
        i += 1;
    }
    cs_deselect();
    /*
    SRAM_decimals();
    byte_mode();
    int i = 0;
    while (i < 512) {
        toBinary(read_byte(i));
        i += 1;
    }
    */
}
/*
static void READ_all() {


    // Initiate the RTC
    datetime_t t = {
        .year = 2022,
        .month = 10,
        .day = 20,
        .dotw = 4,
        .hour = 17,
        .min = 50,
        .sec = 00
    };
    rtc_init();
    rtc_set_datetime(&t);
    sleep_us(64);

    // Print time 
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("Start time for writing test ... \r%s      \r\n", datetime_str);
    int i = 0;
    while (i < 100) {
        SRAM_ones();
        i += 1;
    }
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("End time for writing test... \r%s      \r\n", datetime_str);

    // Print time 
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("Start time for reading test ... \r%s      \r\n", datetime_str);
    i = 0;
    while (i < 100) {
        READ_all();
        i += 1;
    }
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("End time for reading test... \r%s      \r\n", datetime_str);


    sequential_mode();
    uint8_t temp_buffer[512];
    int n_reads = MAX_BYTE/512 - 1;
    int i = 0;
    start_sequential_read(0);
    while (i <= n_reads) {
        spi_read_blocking(SPI, 0x00, temp_buffer, 512);
        i += 1;
    }
    cs_deselect();
}
*/

// Set up the clock divisor: check documentation to convince yourself we did this right. 
static inline void adc_clock_divisor() {
    int divider = (48000000 - ADC_SAMPLE_RATE)/ADC_SAMPLE_RATE;
    adc_set_clkdiv(divider);
}

// Initialize the ADC taking from default pin
static inline void setup_adc() {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_PIN - 26); // select input from appropriate input
    adc_clock_divisor();
    adc_fifo_setup(true, true, 4, false, false);
}

// Initialize the main 
int main() {

    // Sleep a bit for SRAM to stabilise
    sleep_ms(1000);
    stdio_init_all();

    // Initialize our SPI bus 
    spi_init(SPI, BAUDRATE);
    SPI8();

    // Setup pins 
    gpio_init(SPI_CSN_PIN);
    gpio_set_dir(SPI_CSN_PIN, true);

    // Set pin funcionality to the SPI bus 
    gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI); // https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__gpio.html
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI); // j is enumerated- see the documentation 
    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
    
    // Reset the SRAM
    SRAM_zeros();

    // Set up ADC Fifo (free-running mode!) (one transfer is one sample is two bytes.) 
    setup_adc();

    // Configure DMA channel from ADC to the SPI output FIFO
    int ADC_DMA_CHAN = dma_claim_unused_channel(true);
    dma_channel_config ADC_DMA_CONF = dma_channel_get_default_config(ADC_DMA_CHAN);
    channel_config_set_transfer_data_size(&ADC_DMA_CONF, DMA_SIZE_16);
    channel_config_set_read_increment(&ADC_DMA_CONF, false);
    channel_config_set_write_increment(&ADC_DMA_CONF, false);
    channel_config_set_dreq(&ADC_DMA_CONF, DREQ_ADC);
    dma_channel_configure(
        ADC_DMA_CHAN,
        &ADC_DMA_CONF,
        &spi1_hw->dr,
        &adc_hw->fifo,
        ADC_TRANSFER_SIZE, 
        false
    );
    adc_fifo_drain();

    // Start sequential write mode
    sequential_mode();
    start_sequential_write16(0);
    dma_start_channel_mask(1u << ADC_DMA_CHAN);
    adc_run(true);
    dma_channel_wait_for_finish_blocking(ADC_DMA_CHAN);
    cs_deselect();
    SPI8();
    adc_run(false);
    adc_fifo_drain(); 

    // Test reading the first few bytes!
    printf("\r\n");
    uint16_t buf[ADC_TRANSFER_SIZE];
    start_sequential_read16(0);
    spi_read16_blocking(SPI, 0x00, buf, ADC_TRANSFER_SIZE);
    cs_deselect();
    SPI8();
    for (int k = 0; k < ADC_TRANSFER_SIZE; k++) {
        printf("%u \r\n", buf[k]);
    }

    /*
    This is reliable
    Setting the max number of transfers is reliable
    Writes as 16 bit values (not 12 bit) 
    So far, so good! :D 
    */
}