#include "utils.h"
#include "hardware/pwm.h"
#include "pinout.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

/**
 * We will slowly ramp up to 3V3 using PWM duty cycle @ the full 125 MHz (this will introduce ripple into 3V3 of the MAX8510, but it should be fine)
 * According to the datasheet, "• −1.8 V Rated for Low Voltage Gate Drive" for the NTR3A052PZ/D. 
 * Source is 3V3. Set the gate to 1.5V, and you have gate-source of -1.8V (full drive)
 * Vary the duty on the gate from 2.5V down to 1.5V, then jump straight to turning the PWM off and shorting the gate.
*/
void digi_enable(void) {

    // All done. Reset the GPIO, disable PWM, etc etc, and short it.
    gpio_deinit(DIGI_ENABLE);
    gpio_init(DIGI_ENABLE);
    gpio_set_dir(DIGI_ENABLE, GPIO_OUT);
    gpio_put(DIGI_ENABLE, 1);
    sleep_ms(100); // for capacitor inrush 
    
}
void digi_disable(void) {
    gpio_init(DIGI_ENABLE);
    gpio_set_dir(DIGI_ENABLE, GPIO_OUT);
    gpio_put(DIGI_ENABLE, 0);
    gpio_set_drive_strength(DIGI_ENABLE, GPIO_DRIVE_STRENGTH_4MA);
}
void ana_enable(void) {
    gpio_init(ANA_ENABLE);
    gpio_set_dir(ANA_ENABLE, GPIO_OUT);
    gpio_put(ANA_ENABLE, 1); // Ver 1 PCB this should be pulled low instead 
    gpio_set_drive_strength(ANA_ENABLE, GPIO_DRIVE_STRENGTH_4MA);

}
void ana_disable(void) {
    gpio_init(ANA_ENABLE);
    gpio_set_dir(ANA_ENABLE, GPIO_OUT);
    gpio_put(ANA_ENABLE, 0);
    gpio_set_drive_strength(ANA_ENABLE, GPIO_DRIVE_STRENGTH_4MA);
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

// Initialize the LED pin
void debug_init_LED(void) {
    gpio_init(25);
    gpio_set_dir(25, true);
    gpio_put(25, 0);
}

// Flash the LED pin (x) times with a period in MILLISECONDS!!!! 
void debug_flash_LED(int32_t x, int32_t period) {

    for (int i = 0; i < x; i++) {

        gpio_put(25, 1);
        sleep_ms(period);
        gpio_put(25, 0);
        sleep_ms(period);

    }
    
}

/*
pack int32_t array into uint8_t array. return is malloc- you need to free it later. 
len should be the number of int32_t elements involved. 
will only pack first len elements of input buffer.
if an int32_t made up of four masked bytes as 0baaaaaaaabbbbbbbbccccccccdddddddd
it is stored contiguously as uint8_t via a,b,c,d
*/
uint8_t* pack_int32_uint8(int32_t* buf, int32_t len) {

    uint8_t* expanded_buf = (uint8_t*)malloc(len*sizeof(int32_t));
    for (int i = 0; i < len; i++) {
        *(4*i + expanded_buf    ) = (*(buf+i) >> 24)             ;
        *(4*i + expanded_buf + 1) = (*(buf+i) >> 16) & 0b11111111;
        *(4*i + expanded_buf + 2) = (*(buf+i) >> 8 ) & 0b11111111;
        *(4*i + expanded_buf + 3) = (*(buf+i)      ) & 0b11111111;
    }
    return expanded_buf;

}

/*
pack uint8_t array back into int32_t array. return is malloc- you need to free it later.
note that len should be the number of int32_t, not uint8_t 
see expand_uint32 for convention adopted in packing. 
again, will only unpack first len elements of int32.
*/
int32_t* pack_uint8_int32(uint8_t* buf, int32_t len) {

    int32_t* expanded_buf = (int32_t*)malloc(len*sizeof(int32_t));
    for (int i = 0; i < len; i++) {
        *(expanded_buf+i) = (*(4*i + buf) << 24) | (*(4*i + buf + 1) << 16) | (*(4*i + buf + 2) << 8) | (*(4*i + buf + 3));
    }
    return expanded_buf;

}