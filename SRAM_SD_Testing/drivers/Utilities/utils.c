#include "utils.h"

// Consts for the enable pins 
static const int32_t DIGI_ENABLE = 6; // pull high to enable digital
static const int32_t ANA_ENABLE = 28; // pull low to enable analogue

void digi_enable(void) {
    gpio_init(DIGI_ENABLE);
    gpio_set_dir(DIGI_ENABLE, GPIO_OUT);
    gpio_put(DIGI_ENABLE, 1);
    gpio_set_drive_strength(DIGI_ENABLE, GPIO_DRIVE_STRENGTH_2MA);
}
void digi_disable(void) {
    gpio_put(DIGI_ENABLE, 0);
}
void ana_enable(void) {
    gpio_init(ANA_ENABLE);
    gpio_set_dir(ANA_ENABLE, GPIO_OUT);
    gpio_put(ANA_ENABLE, 0);
    gpio_set_drive_strength(ANA_ENABLE, GPIO_DRIVE_STRENGTH_2MA);
}
void ana_disable(void) {
    gpio_put(ANA_ENABLE, 1);
}

// Set up the clock divisor: check documentation to convince yourself we did this right. 
static void adc_clock_divisor(void) {
    int divider = (48000000 - ADC_SAMPLE_RATE)/ADC_SAMPLE_RATE;
    adc_set_clkdiv(divider);
}

// set up ADC pins/etc + run in free-running-mode 
void setup_adc(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_PIN - 26); // select input from appropriate input
    adc_clock_divisor();
    adc_fifo_setup(true, true, 1, false, false);
    adc_run(true);
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




