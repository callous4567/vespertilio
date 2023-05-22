#include "custoclocks.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "external_config.h"
#include "hardware/vreg.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"

/* Bank of VCO frequencies and post dividers, provided by the pico-sdk vcocalc.py file. All tested fine with Sandisk Ultra 64GB SD. */

static const int32_t VCO_100 = 900*MHZ; // 100 MHz- above 384 kHz 
static const int32_t VCO_80 = 960*MHZ; // 80 MHz- above 192 kHz
static const int32_t VCO_50 = 900*MHZ; // 50 MHz- if 40 MHz proves insufficient
static const int32_t VCO_40 = 840*MHZ; // 40 MHz- 0->192 kHz (you can try lower clocks- 20 MHz would work fine for 96 kHz ADC, for example.)

static const int8_t PD1_100 = 3;
static const int8_t PD1_80 = 6;
static const int8_t PD1_50 = 6;
static const int8_t PD1_40 = 7;

static const int8_t PD2_100 = 3;
static const int8_t PD2_80 = 2;
static const int8_t PD2_50 = 3;
static const int8_t PD2_40 = 3;

// Track clock multiplier
int8_t CLOCK_DECIMULTIPLE_TO_NORMAL; // see INTERBLOCK_SLEEP_TIME_US. A scaling factor to scale the base time at "full 100 MHz" to lower clock speeds operation-wise.

// set the system, peri, ref, and USB clocks, using provided vco/posts
void custoset_sys_clk_pll(uint32_t vco_freq, uint post_div1, uint post_div2) 
{

	clock_configure(clk_sys,
					CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
					CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
					48 * MHZ,
					48 * MHZ);

	pll_init(pll_sys, 1, vco_freq, post_div1, post_div2);
	uint32_t freq = vco_freq / (post_div1 * post_div2);

	// Configure clocks
	// CLK_REF = XOSC (12MHz) / 1 = 12MHz
	clock_configure(clk_ref,
					CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
					0, // No aux mux
					12 * MHZ,
					12 * MHZ);

	// CLK SYS = PLL SYS (125MHz) / 1 = 125MHz
	clock_configure(clk_sys,
					CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
					CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
					freq, freq);

    //clock_configure(clk_peri,
    //                0, // Only AUX mux on ADC
    //                CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
    //                60 * MHZ,
    //                60 * MHZ);
    
    clock_configure(
            clk_peri,
            0,                                                // No glitchless mux
            CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
            freq,                               // Input frequency
            freq                               // Output (must be same as no divider)
        );
    
}

/*
TODO:
BASE CLOCK UNCHANGED (125 MHz)
SLEEP CYCLES RELATIVE TO BASE TIMING (find the timing, pad it by ~10-20%, then scale it for different sample rate timings- see INTERBLOCK_SLEEP_TIME_US)
NO NEED TO SCALE CLOCKS BECAUSE IF YOU DO, THE BASE TIMING CHANGES, AND YOUR POWER SAVING ALTERS A LOT.
*/
// Set the optimal clock for the session, initialize the stdio, and also initialize the flashlog. Disabled functionality, for now. 
void set_optimal_clock(void) {

    
    /*
    if (ADC_SAMPLE_RATE >= 384000) {
        
        custoset_sys_clk_pll(
            VCO_100,
            PD1_100,
            PD2_100
        );
        vreg_set_voltage(
            VREG_VOLTAGE_DEFAULT  
        );
        CLOCK_DECIMULTIPLE_TO_NORMAL = 10;

    } else if (ADC_SAMPLE_RATE > 192001) {
        
        custoset_sys_clk_pll(
            VCO_80,
            PD1_80,
            PD2_80
        );
        vreg_set_voltage(
            VREG_VOLTAGE_1_00   
        );
        CLOCK_DECIMULTIPLE_TO_NORMAL = 8;

    } else {
        
        custoset_sys_clk_pll(
            VCO_40,
            PD1_40,
            PD2_40
        );
        vreg_set_voltage(
            VREG_VOLTAGE_0_90    
        );
        CLOCK_DECIMULTIPLE_TO_NORMAL = 4;
    }
    */

    uint32_t sys = clock_get_hz(clk_sys);
    uint32_t peri = clock_get_hz(clk_peri);
    uint32_t ref = clock_get_hz(clk_ref);
    uint32_t usb = clock_get_hz(clk_usb);

    stdio_init_all();
    sleep_ms(100);

    if (USE_FLASHLOG) {
        init_flashlog(); // init the flashlog 
    }

    custom_printf(
        "Set the optimal clock for this session: sample rate %d. SYS PERI REF USB %d %d %d %d\r\n",
        ADC_SAMPLE_RATE,
        sys,
        peri,
        ref,
        usb
    );

}