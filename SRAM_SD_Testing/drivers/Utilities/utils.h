// Header Guard
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"

// Enable digital assembly or analogue assembly (and disable too)
void digi_enable(void); 
void digi_disable(void);
void ana_enable(void);
void ana_disable(void);
void ana_pull_down(void);

// Print as binary the uint8_t value a 
void toBinary(uint8_t a); 

// Print as binary the uint16_t value a
void toBinary_16(uint16_t a);

// Initialize the LED pin
void debug_init_LED(void);

// Flash the LED pin (x) times 
void debug_flash_LED(int32_t x);


uint8_t* pack_int32_uint8(int32_t* buf, int32_t len);


int32_t* pack_uint8_int32(uint8_t* buf, int32_t len);



#endif /* IS62_SRAM_H */