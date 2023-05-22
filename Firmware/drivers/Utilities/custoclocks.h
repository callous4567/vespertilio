#ifndef CUSTOCLOCKS_H
#define CUSTOCLOCKS_H 

#include <stdint.h>
#include "pico/stdlib.h"

extern int8_t CLOCK_DECIMULTIPLE_TO_NORMAL; // the fraction multiplied by ten (i.e. 0.4 -> 4, 0.8 -> 8, relative to 100 MHz)

void set_optimal_clock(void);

void custoset_sys_clk_pll(uint32_t vco_freq, uint post_div1, uint post_div2); 


#endif 