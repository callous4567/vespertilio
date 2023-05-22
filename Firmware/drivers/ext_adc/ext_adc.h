// Header Guard
#ifndef EXT_ADC_T
#define EXT_ADC_T

/*
Specifically written for the MCP33151D ADC.
*/

#include "../Utilities/pinout.h"

typedef struct {

    // Pins
    int rx; 
    int sck;
    int cnvst;
    
    // Serial 
    int baudrate;
    int clock; 

} ext_adc_t;






#endif /* EXT_ADC_T */