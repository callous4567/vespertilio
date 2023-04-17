extern "C" {
#include "drivers/Utilities/utils.h"
#include "drivers/Utilities/external_config.h"
#include "drivers/mSD/mSD.h"
#include "drivers/ext_rtc/ext_rtc.h"
#include "drivers/mcp4131_digipot/spi_driver.h"
#include "drivers/bme280/bme280_spi.h"
#include "drivers/pico_usb_configure/vespertilio_usb_int.h"
#include "drivers/veml/i2c_driver.h"
#include "drivers/Utilities/pinout.h"
}
#include "hardware/vreg.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "drivers/recording/recording_singlethread.h"


//#define PLL 200000

/*

#define PLL 180000

    vreg_set_voltage(VREG_VOLTAGE_1_10);
    set_sys_clock_khz(PLL, true);
    clock_configure(
            clk_peri,
            0,                                                // No glitchless mux
            CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
            PLL*1000,                               // Input frequency
            PLL*1000                                // Output (must be same as no divider)
        );

    digi_enable(); // initialize digital assembly for RTC/etc pullups by default 
    ana_disable(); // ensure analogue is disabled 
    debug_init_LED(); // initialize debug LED 
    stdio_init_all(); // initialize STDIO for debugging + USB configuration

    characterize_SD_write_time(4096);

*/

// Run a test of the SD functionality 

int main() {

    busy_wait_ms(1000);
    stdio_init_all(); // initialize STDIO for debugging + USB configuration
    digi_enable(); // initialize digital assembly for RTC/etc pullups by default 
    ana_enable(); // ensure analogue is disabled 
    //ext_rtc_t* EXT_RTC = init_RTC_default();
    //rtc_sleep_until_alarm(EXT_RTC);
    
    debug_init_LED(); // initialize debug LED 
    dpot_dual_t* DPOT = init_dpot(); // set up gains 
    dpot_set_gain(DPOT, 20);
    deinit_dpot(DPOT);
    default_variables();  
    printf("Starting test recording session!\r\n");
    run_wav_bme_sequence_single(); // run the BME sequence (which will self-free) 
    

    /*
    int32_t success = usb_configurate(); // attempt USB configuration... returns either (0) for panic/failed, (1) for success, or (2) for handshake failed...

    switch (success) 
    {

        case 2: { // handshake was not attempted and we are free to run data acquisition code.

            printf("No handshake! Continuing on to regular runtime ^_^ \r\n");
            debug_flash_LED(1, 10000);

            // get the configuration buffer loaded from flash and also grab a hold of the (constant) independent variables
            flash_read_to_configuration_buffer_external(); // configure default from flash + get constant independent variables

            // iterate over the alarms...
            for (int i = 1; i <= NUMBER_OF_ALARMS; i++) {

                set_dependent_variables(i); // set the dependent variables for this alarm 

                rtc_setsleep_WHICH_ALARM_ONEBASED(configuration_buffer_external, CONFIGURATION_BUFFER_INDEPENDENT_VALUES, i); // sleep until we're ready. this will disable the digital assembly appropriately before sleeping (we need pullups.)

                ana_enable();

                debug_flash_LED(10, 100);

                dpot_dual_t* DPOT = init_dpot(); // set up gains 
                dpot_set_gain(DPOT, 20);
                deinit_dpot(DPOT);

                run_wav_bme_sequence_single(); // run the BME sequence (which will self-free) 

                busy_wait_ms(100000); // give a second for things to clean up on the mSD/etc.

                ana_disable();

            }

            // we're finished with the set alarms. enter standard dormancy.


        }

        break;

        case 1: { // just wait an hour and do nothing. configuration was success.

            busy_wait_ms(1000*3600);

        }

        break;

        case 0: { // same as for true, except in this case the configuration failed/

            debug_flash_LED(1000, 100);
            busy_wait_ms(1000*3600); 

        }

        break;

        default: {// AGGRESSIVE FLASHING 

            while (true) {
                
                debug_flash_LED(1000,100); // VERY AGGRESSIVE FLASHING BABY 

            }

        }

        break;

    }

    */
    
    
}
