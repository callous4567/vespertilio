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
#include "IS62_SRAM.h"
#include "../Utilities/utils.h"

spi_inst_t *SRAM_SPI = spi1;

// Activate the CS pin (pull low)
void SRAM_select(sram_t *SRAM) { /* declaration: https://stackoverflow.com/questions/21835664/why-declare-a-c-function-as-static-
    declaring static means other modules do not see it: it's local to this module and saves effort
    on letting other modules see it.
    declaring means that when the file is compiled, SRAM_select(SRAM) will be dumped in the code,
    and then compiled locally, rather than being pulled in some it having been compiled elsewhere.
    declaring void means that this function does not return anything at all. 
    */
    //asm volatile("nop \n nop \n nop"); // assembler-up some nops- delay code by 3 clock cycles.
    gpio_put(SRAM->cs_pin, 0); // pull low/active
    //asm volatile("nop \n nop \n nop"); // https://forums.raspberrypi.com/viewtopic.php?t=332078
}
void SRAM_deselect(sram_t *SRAM) {
    
    //asm volatile("nop \n nop \n nop"); // assembler-up some nops- delay code by 3 clock cycles.
    gpio_put(SRAM->cs_pin, 1); // pull high/active
    //asm volatile("nop \n nop \n nop"); // https://forums.raspberrypi.com/viewtopic.php?t=332078
}

// Test writing/reading the register- use before SRAM_read_default_register(). Default PAGE MODE.
static void SRAM_write_default_register(sram_t *SRAM) {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = SRAM_WRMR;
    buf[1] = 0b10000000;
    SRAM_select(SRAM);
    int written = spi_write_blocking(SRAM->hw_inst, buf, 2);
    SRAM_deselect(SRAM);
    printf("Bytes written to SPI... ");
    printf("%d", written);
    printf("\r\n");
}
static void SRAM_read_default_register(sram_t *SRAM) {
    uint8_t buf[1];
    buf[0] = SRAM_RDMR;
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 1);
    spi_read_blocking(SRAM->hw_inst, 0, buf, 1);
    SRAM_deselect(SRAM);
    printf("The current read register...\r\n");
    toBinary(buf[0]);
}

// Set mode
void SRAM_byte_mode(sram_t *SRAM) {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = SRAM_WRMR;
    buf[1] = 0b00000000;
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 2);
    SRAM_deselect(SRAM);
}
static void SRAM_page_mode(sram_t *SRAM) {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = SRAM_WRMR;
    buf[1] = 0b10000000;
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 2);
    SRAM_deselect(SRAM);
}
void SRAM_sequential_mode(sram_t *SRAM) {
    uint8_t buf[2]; // first the instruction then the new register 
    buf[0] = SRAM_WRMR;
    buf[1] = 0b01000000;
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 2);
    SRAM_deselect(SRAM);
}

// Set the SPI mode to 8 bit or 16 bit as required below
void SRAM_SPI8(sram_t *SRAM) {
    spi_set_format(SRAM->hw_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}
void SRAM_SPI16(sram_t *SRAM) {
    spi_set_format(SRAM->hw_inst, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

// Start sequentials. Note that you need to call SRAM_SPI8(SRAM) at the end of any 16-bit process for future commands (8 bit) to work. When in 16-bit, you have half of SRAM_MAXBYTE/PAGE for indices. 
void SRAM_start_sequential_read(sram_t *SRAM, int address) {
    uint8_t buf[4];
    buf[0] = SRAM_READ;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 4);
}
void SRAM_start_sequential_read16(sram_t *SRAM, int address) {
    uint8_t buf[4];
    buf[0] = SRAM_READ;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 4);
    SRAM_SPI16(SRAM);
}
void SRAM_start_sequential_write(sram_t *SRAM, int address) {
    uint8_t buf[4];
    buf[0] = SRAM_WRITE;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 4);
}
void SRAM_start_sequential_write16(sram_t *SRAM, int address) {
    uint8_t buf[4];
    buf[0] = SRAM_WRITE;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address; 
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 4);
    SRAM_SPI16(SRAM);
}

// Read a certain byte
uint8_t SRAM_read_byte(sram_t *SRAM, int address) {
    uint8_t buf[4];
    uint8_t read_buf[1];
    buf[0] = SRAM_READ;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address;
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 4);
    spi_read_blocking(SRAM->hw_inst, 0x00, read_buf, 1);
    SRAM_deselect(SRAM);
    return read_buf[0];
}
void SRAM_write_byte(sram_t *SRAM, int address, uint8_t byte) {
    uint8_t buf[5];
    buf[0] = SRAM_WRITE;
    buf[1] = 0b000000 | (address >> 16);
    buf[2] = address >> 8;
    buf[3] = address;
    buf[4] = byte;
    SRAM_select(SRAM);
    spi_write_blocking(SRAM->hw_inst, buf, 5);
    SRAM_deselect(SRAM);
}

// Set the entire SRAM unit to ZERO 
void SRAM_zeros(sram_t *SRAM) {
    SRAM_sequential_mode(SRAM);
    int i = 0;
    uint8_t zero_page[512]; // 16 pages 
    while (i<512) {
        zero_page[i] = 0b00000000;
        i += 1;
    }
    i = 0;
    int n_page_writes = (SRAM_MAXPAGE/16) - 1; 
    SRAM_start_sequential_write(SRAM, 0);
    while (i <= n_page_writes) {
        spi_write_blocking(SRAM->hw_inst, zero_page, 512);
        i += 1; 
    }
    SRAM_deselect(SRAM);
}
static void SRAM_ones(sram_t *SRAM) {
    /*
    // Do a sequential test... (it works fine!) 
    printf("Now sequential mode tests! \r\n");
    SRAM_zeros();
    SRAM_byte_mode(SRAM);
    uint8_t TT1 = SRAM_read_byte(SRAM_MAXBYTE-1);
    toBinary(TT1);
    SRAM_ones();
    SRAM_byte_mode(SRAM);
    uint8_t TT2 = SRAM_read_byte(SRAM_MAXBYTE-1);
    toBinary(TT2);
    */
    int i = 0;
    uint8_t one_page[512]; // 16 pages
    while (i<512) {
        one_page[i] = 0b11111111;
        i += 1;
    }
    i = 0;
    int n_page_writes = (SRAM_MAXPAGE/16) - 1; 
    SRAM_sequential_mode(SRAM);
    SRAM_start_sequential_write(SRAM, 0);
    while (i <= n_page_writes) {
        spi_write_blocking(SRAM->hw_inst, one_page, 512);
        i += 1; 
    }
    SRAM_deselect(SRAM);
}
static void SRAM_decimals(sram_t *SRAM) { 
    int i = 0;
    uint8_t buf[1];
    SRAM_sequential_mode(SRAM);
    SRAM_start_sequential_write(SRAM, 0);
    while (i<513) {
        SRAM_write_byte(SRAM, i, i);
        i += 1;
    }
    SRAM_deselect(SRAM);
    /*
    SRAM_decimals();
    SRAM_byte_mode(SRAM);
    int i = 0;
    while (i < 512) {
        toBinary(SRAM_read_byte(i));
        i += 1;
    }
    */
}
/*
static void SRAM_READ_all() {


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
        SRAM_READ_all();
        i += 1;
    }
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("End time for reading test... \r%s      \r\n", datetime_str);


    SRAM_sequential_mode(SRAM);
    uint8_t temp_buffer[512];
    int n_reads = SRAM_MAXBYTE/512 - 1;
    int i = 0;
    SRAM_start_sequential_read(0);
    while (i <= n_reads) {
        spi_read_blocking(SRAM_SPI, 0x00, temp_buffer, 512);
        i += 1;
    }
    SRAM_deselect(SRAM);
}
*/

void SRAM_init(sram_t *SRAM) {

    // Initialize our SPI bus 
    spi_init(SRAM->hw_inst, SRAM->baudrate);
    SRAM_SPI8(SRAM);

    // Setup pins 
    gpio_init(SRAM->cs_pin);
    gpio_set_dir(SRAM->cs_pin, true);

    // Set pin funcionality to the SPI bus 
    gpio_set_function(SRAM->rx_pin, GPIO_FUNC_SPI); // https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__gpio.html
    gpio_set_function(SRAM->sck_pin, GPIO_FUNC_SPI); // j is enumerated- see the documentation 
    gpio_set_function(SRAM->tx_pin, GPIO_FUNC_SPI);
    
    // Reset the SRAM
    SRAM_zeros(SRAM);

}

static spi_hw_t* get_SPI_hw_instance(sram_t *SRAM) {
    if (SRAM->hw_inst==spi1) {
        return spi1_hw;
    } else {
        return spi0_hw;
    }
}

// Run an SRAM DMA test! 
void SRAM_DMA_TEST(sram_t *SRAM) {

    SRAM_init(SRAM);

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
        &get_SPI_hw_instance(SRAM)->dr,
        &adc_hw->fifo,
        ADC_TRANSFER_SIZE, 
        false
    );
    adc_fifo_drain();

    // Start sequential write mode
    SRAM_sequential_mode(SRAM);
    SRAM_start_sequential_write16(SRAM, 0);
    dma_start_channel_mask(1u << ADC_DMA_CHAN);
    adc_run(true);
    dma_channel_wait_for_finish_blocking(ADC_DMA_CHAN);
    SRAM_deselect(SRAM);
    SRAM_SPI8(SRAM);
    adc_run(false);
    adc_fifo_drain(); 

    // Test reading the first few bytes!
    printf("\r\n");
    uint16_t buf[ADC_TRANSFER_SIZE];
    SRAM_start_sequential_read16(SRAM, 0);
    spi_read16_blocking(SRAM->hw_inst, 0x00, buf, ADC_TRANSFER_SIZE);
    SRAM_deselect(SRAM);
    SRAM_SPI8(SRAM);
    for (int k = 0; k < ADC_TRANSFER_SIZE; k++) {
        printf("%u \r\n", buf[k]);
    }

    printf("Successfully ran SRAM_DMA_TEST test without any errors. Proceeding.");
}
