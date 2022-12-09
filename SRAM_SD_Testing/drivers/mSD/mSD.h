#ifndef MSD_H
#define MSD_H 

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// All the SRAM parameters for the SD CARD for the hardware configuration. We will manually handle the card detect ourselves.  
const int SD_ENABLE = 20; // for mosfet 
const bool SD_CD_USEORNOT = false; // use the CD or not 
const int SD_CD = 21; // for card detect gpio 
const int SD_CD_VALUE = 0; // what is read if card present

// Configure all the pins for the SD 
void SD_pin_configure(void);

// Turn mosfet on for SD
void SD_ON(void);

// Turn mosfet off for SD
void SD_OFF(void);

// Check if file exists on SD 
bool SD_IS_EXIST(const char *test_filename);

// Do the major gineral test write-read 
void major_gineral_testwrite(void); 

#endif // MSD_H 