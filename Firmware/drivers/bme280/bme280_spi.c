
/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "bme280_spi.h"
#include "malloc.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "../Utilities/pinout.h"
/* Example code to talk to a bme280 humidity/temperature/pressure sensor.
   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore SPI) cannot be used at 5v.
   You will need to use a level shifter on the SPI lines if you want to run the
   board at 5v.
   Connections on Raspberry Pi Pico board and a generic bme280 board, other
   boards may vary.
   GPIO 16 (pin 21) MISO/spi0_rx-> SDO/SDO on bme280 board
   GPIO 17 (pin 22) Chip select -> CSB/!CS on bme280 board
   GPIO 18 (pin 24) SCK/spi0_sclk -> SCL/SCK on bme280 board
   GPIO 19 (pin 25) MOSI/spi0_tx -> SDA/SDI on bme280 board
   3.3v (pin 36) -> VCC on bme280 board
   GND (pin 38)  -> GND on bme280 board
   Note: SPI devices can have a number of different naming schemes for pins. See
   the Wikipedia page at https://en.wikipedia.org/wiki/Serial_Peripheral_Interface
   for variations.
   This code uses a bunch of register definitions, and some compensation code derived
   from the Bosch datasheet which can be found here.
   https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf
*/

#define READ_BIT 0x80

// define baudrate and spi 
static const int32_t BME_BAUD = 10*1000*1000;

int32_t t_fine;

uint16_t dig_T1;
int16_t dig_T2, dig_T3;
uint16_t dig_P1;
int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
uint8_t dig_H1, dig_H3;
int8_t dig_H6;
int16_t dig_H2, dig_H4, dig_H5;

/* The following compensation functions are required to convert from the raw ADC
data from the chip to something usable. Each chip has a different set of
compensation parameters stored on the chip at point of manufacture, which are
read from the chip at startup and used in these routines.
*/

// compensate the reading from the BME for temperature. Note that there is an offset of 1-2 degrees due to self-heating that isn't corrected for.
static int32_t compensate_temp(int32_t adc_T) {
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t) dig_T1 << 1))) * ((int32_t) dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t) dig_T1)) * ((adc_T >> 4) - ((int32_t) dig_T1))) >> 12) * ((int32_t) dig_T3))
            >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// compensate the reading from the BME for pressure 
static uint32_t compensate_pressure(int32_t adc_P) {
    int32_t var1, var2;
    uint32_t p;
    var1 = (((int32_t) t_fine) >> 1) - (int32_t) 64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t) dig_P6);
    var2 = var2 + ((var1 * ((int32_t) dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t) dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t) dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t) dig_P1)) >> 15);
    if (var1 == 0)
        return 0;

    p = (((uint32_t) (((int32_t) 1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000)
        p = (p << 1) / ((uint32_t) var1);
    else
        p = (p / (uint32_t) var1) * 2;

    var1 = (((int32_t) dig_P9) * ((int32_t) (((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t) (p >> 2)) * ((int32_t) dig_P8)) >> 13;
    p = (uint32_t) ((int32_t) p + ((var1 + var2 + dig_P7) >> 4));

    return p;
}

// compensate the reading from the BME for humidity 
static uint32_t compensate_humidity(int32_t adc_H) {
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t) 76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t) dig_H4) << 20) - (((int32_t) dig_H5) * v_x1_u32r)) +
                   ((int32_t) 16384)) >> 15) * (((((((v_x1_u32r * ((int32_t) dig_H6)) >> 10) * (((v_x1_u32r *
                                                                                                  ((int32_t) dig_H3))
            >> 11) + ((int32_t) 32768))) >> 10) + ((int32_t) 2097152)) *
                                                 ((int32_t) dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t) dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    return (uint32_t) (v_x1_u32r >> 12);
}

static inline void cs_select() {
    gpio_put(BME_CS_PIN, 0);  // Active low
}

static inline void cs_deselect() {
    gpio_put(BME_CS_PIN, 1);
}

// write the register with the provided uint8_t data 
static void write_register(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg & 0x7f;  // remove read bit as this is a write
    buf[1] = data;
    cs_select();
    busy_wait_ms(5);
    spi_write_blocking(spi0, buf, 2);
    cs_deselect();
    busy_wait_ms(5);

}

// read the register reg to the buffer (length len uint8_t bytes.) Note that there is a cost of 3-ish milliseconds here.
static void read_registers(uint8_t reg, uint8_t *buf, uint16_t len) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.
    reg |= READ_BIT;
    cs_select();
    busy_wait_ms(1);
    spi_write_blocking(spi0, &reg, 1);
    busy_wait_ms(1);
    spi_read_blocking(spi0, 0, buf, len);
    cs_deselect();
    busy_wait_ms(1);

}

/* This function reads the manufacturing assigned compensation parameters from the device */
static void read_compensation_parameters() {
    uint8_t buffer[26];

    read_registers(0x88, buffer, 24);

    dig_T1 = buffer[0] | (buffer[1] << 8);
    dig_T2 = buffer[2] | (buffer[3] << 8);
    dig_T3 = buffer[4] | (buffer[5] << 8);

    dig_P1 = buffer[6] | (buffer[7] << 8);
    dig_P2 = buffer[8] | (buffer[9] << 8);
    dig_P3 = buffer[10] | (buffer[11] << 8);
    dig_P4 = buffer[12] | (buffer[13] << 8);
    dig_P5 = buffer[14] | (buffer[15] << 8);
    dig_P6 = buffer[16] | (buffer[17] << 8);
    dig_P7 = buffer[18] | (buffer[19] << 8);
    dig_P8 = buffer[20] | (buffer[21] << 8);
    dig_P9 = buffer[22] | (buffer[23] << 8);

    dig_H1 = buffer[25];

    read_registers(0xE1, buffer, 8);

    dig_H2 = buffer[0] | (buffer[1] << 8);
    dig_H3 = (int8_t) buffer[2];
    dig_H4 = buffer[3] << 4 | (buffer[4] & 0xf);
    dig_H5 = (buffer[5] >> 4) | (buffer[6] << 4);
    dig_H6 = (int8_t) buffer[7];
}

// read raw adc data from bme280 to buffers 
static void bme280_read_raw(int32_t *humidity, int32_t *pressure, int32_t *temperature) {
    uint8_t buffer[8];

    read_registers(0xF7, buffer, 8);
    *pressure = ((uint32_t) buffer[0] << 12) | ((uint32_t) buffer[1] << 4) | (buffer[2] >> 4);
    *temperature = ((uint32_t) buffer[3] << 12) | ((uint32_t) buffer[4] << 4) | (buffer[5] >> 4);
    *humidity = (uint32_t) buffer[6] << 8 | buffer[7];
}

// set the default oversampling parameters 
static inline void set_default_registers(void) {

    // Set the various registers for oversampling 
    write_register(0xF2, 0b00000010); // Humidity oversampling register. Table 20, Chapter 3.4.1. Set to 0x2 for 2x oversample.
    write_register(0xF4, 0b01001011);// Set rest of oversampling modes and run mode to normal (Page 29, Table 22).
    /*
    For the register 0xF4...

    Bit 7,6,5 is the temperature data oversampling, Table 24 Chapter 3.4.3. 010 is x2 oversampling.
    Bit 4,3,2 is the pressure data oversampling, Table 23 Chapter 3.4.2. 010 is x2 oversampling. 
    Bit 1,0 is the sensor mode, Table 25 Chapter 3.3. 11 is normal mode. 

    The consequence is that our register 0xF4 must be set to...

    0b01001011! 
    */

    // Set the default configuration register
    write_register(0xF5, 0b10100000);
    /*
    
    See 5.4.6 Page 30 Tables 26 onward.
    The control register has 8 bits.
    
    Bit 7,6,5 controls inactive duration between measurements. Set to 101 to measure every second. 
    Bit 4,3,2 controls filter settings. Not needed. Disable filter via 000.
    Bit 1 is a "no-care" bit
    Bit 0 decides 4-wire vs. 3-wire. Set to 0 for standard 4-wire SPI mode.

    Consequently, our final register for the default control is given by...
    10100000
    */

}

// init the SPI and CS by default 
static inline void init_default_spi_bme(void) {
    
    spi_init(BME_HW_INST, BME_BAUD);

    gpio_set_function(BME_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(BME_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(BME_MOSI_PIN, GPIO_FUNC_SPI);

    gpio_init(BME_CS_PIN);
    gpio_set_dir(BME_CS_PIN, GPIO_OUT);
    gpio_put(BME_CS_PIN, 1);

    gpio_set_pulls(BME_MISO_PIN, false, false);
    gpio_set_pulls(BME_SCK_PIN, false, false);
    gpio_set_pulls(BME_MOSI_PIN, false, false);

    gpio_set_drive_strength(BME_MISO_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(BME_SCK_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(BME_MOSI_PIN, GPIO_DRIVE_STRENGTH_4MA);


}

// set up the BME
void setup_bme(void) {

    // init SPI 
    init_default_spi_bme();

    // read compensation 
    read_compensation_parameters();
    
    // Set the various registers for oversampling 
    set_default_registers();

}

// update the datastring buffer with a string containing the humidity/pressure/temperature in human-readable form using snprintf. buffer is max 20 bytes. 
void bme_datastring(char* datastring) {

    /*
    the maximum number of chars/bytes written is...
    humidity        :    max[0.0,100.0]%     ->   5 bytes 
    pressure        :    max[0->120,000]pA   ->   6 bytes
    temperature     :    max[-20.0,+60.0]    ->   5 bytes  
    spacer          :    2 per spacer        ->   4 bytes
    hence total of 20 bytes 
    */

    // Get the data 
    int32_t humidity, pressure, temperature;
    bme280_read_raw(&humidity, &pressure, &temperature);
    pressure = compensate_pressure(pressure);
    temperature = compensate_temp(temperature);
    humidity = compensate_humidity(humidity);

    // get the datastring 
    snprintf(
        datastring,
        20,
        "%.1f_%d_%.1f", 
        humidity/1024.0,
        pressure,
        temperature/100.0
    );

}

void test_bme(void) {

    // setup 
    setup_bme();

    // allocate datastring 
    char* datastring = (char*)malloc(20);

    // allocate test values. 
    int32_t humidity, pressure, temperature;

    while (1) {

        // These are the raw numbers from the chip, so we need to run through the
        // compensations to get human understandable numbers
        bme_datastring(datastring);

        printf("The datastring is...!%s\r\n", datastring);

        busy_wait_ms(1000);
    }
}
