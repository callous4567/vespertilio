#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "utils.h"

// Set up the clock divisor: check documentation to convince yourself we did this right. 
static void adc_clock_divisor(void) {
    int divider = (48000000 - ADC_SAMPLE_RATE)/ADC_SAMPLE_RATE;
    adc_set_clkdiv(divider);
}

// Initialize the ADC taking from default pin
void setup_adc(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_PIN - 26); // select input from appropriate input
    adc_clock_divisor();
    adc_fifo_setup(true, true, 1, false, false);
}

// Print as binary the individual 8-bit byte a 
void toBinary(uint8_t a) {
    uint8_t i;

    for(i=0x80;i!=0;i>>=1)
        printf("%c",(a&i)?'1':'0'); 
    printf("\r\n");
}

// Print as binary the individual 16-bit byte a 
void toBinary_16(uint16_t a) {
    uint16_t i;

    for (int i = 0; i < 16; i++) {
        printf("%d", (a & 0x8000) >> 15);
        a <<= 1;
    }
    printf("\r\n");
}




