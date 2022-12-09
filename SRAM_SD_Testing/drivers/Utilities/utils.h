// Header Guard
#ifndef UTILS_H
#define UTILS_H

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
This file holds various utilities that don't necessarily fit elsewhere.
*/
static const int ADC_PIN = 26;
static const int ADC_SAMPLE_RATE = 192000;
static const int ADC_TRANSFER_SIZE = 512; // 256 transfers equals 512 bytes equals 16 pages. 

// Initialize the ADC taking from default pin
void setup_adc(void); 

// Print as binary the uint8_t value a 
void toBinary(uint8_t a); 

// Print as binary the uint16_t value a
void toBinary_16(uint16_t a);

#endif /* IS62_SRAM_H */