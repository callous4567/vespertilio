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

/*

NOTE TO SELF IMPORTANT!!!

Super cap only handles 3v3 
The RTC does not stop charging it at all
It will charge until VCC - 0.7V 
Consequently, with 4V5 batteries, that's 3.8V (0.5V above rating)
Example^^^
I recommend disabling trickle charge unless using rechargeable batteries (which will cap out around 4V-4.2V.) 
Next PCB version, note our default setting (2k series resistor)
Throw in a voltage divider circuit on our 
*/
int main() {
    stdio_init_all(); // initialize STDIO for debugging + USB configuration
    //digi_enable(); // initialize digital assembly for RTC/etc pullups by default 
    ana_disable(); // ensure analogue is disabled 
    debug_init_LED(); // initialize debug LED 
    
    int32_t success = usb_configurate(); // attempt USB configuration... returns either (0) for panic/failed, (1) for success, or (2) for handshake failed...

    switch (success) 
    {

        case 2: { // handshake was not attempted and we are free to run data acquisition code.

            printf("No handshake! Continuing on to regular runtime ^_^ \r\n");
            debug_flash_LED(1, 10000);

            // get the configuration buffer loaded from flash and also grab a hold of the (constant) independent variables
            flash_read_to_configuration_buffer_external(); // configure default from flash + get constant independent variables

            // iterate over the alarms...
            for (int i = 1; i <= NUMBER_OF_SESSIONS; i++) {

                set_dependent_variables(i); // set the dependent variables for this alarm 

                rtc_setsleep_WHICH_ALARM_ONEBASED(configuration_buffer_external, CONFIGURATION_BUFFER_INDEPENDENT_VALUES, i); // sleep until we're ready. this will disable the digital assembly appropriately before sleeping (we need pullups.)

                ana_enable();

                debug_flash_LED(10, 100);

                dpot_dual_t* DPOT = init_dpot(); // set up gains 
                dpot_set_gain(DPOT, 20);
                deinit_dpot(DPOT);

                // default_variables();  

                run_wav_bme_sequence_single(); // run the BME sequence (which will self-free) 

                busy_wait_ms(1000); // give a second for things to clean up on the mSD/etc.

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

    
    
    
}
