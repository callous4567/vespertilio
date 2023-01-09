// Header Guard
#ifndef UTILS_H
#define UTILS_H

// See external_config.h for includes + configurated constants. 
#include "external_config.h"

// Enable digital assembly or analogue assembly (and disable too)
void digi_enable(void); 
void digi_disable(void);
void ana_enable(void);
void ana_disable(void);
void ana_pull_down(void);

// Initialize the ADC taking from default pin
void setup_adc(void); 

// Print as binary the uint8_t value a 
void toBinary(uint8_t a); 

// Print as binary the uint16_t value a
void toBinary_16(uint16_t a);

#endif /* IS62_SRAM_H */