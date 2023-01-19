extern "C" {
#include "drivers/Utilities/utils.h"
#include "drivers/Utilities/external_config.h"
#include "drivers/mSD/mSD.h"
#include "drivers/ext_rtc/ext_rtc.h"
#include "drivers/recording/recording.h"
#include "drivers/recording/recording_singlethread.h"
#include "drivers/mcp4131_digipot/spi_driver.h"
#include "drivers/bme280/bme280_spi.h"
#include "drivers/pico_usb_configure/vespertilio_usb_int.h"
}


/*
Todo list:
- deconstruct the FatFS write 
- add custom method for writing that we can use explicitly for our ADC writing/etc
- profit

*/
// Run a test of the SD functionality 
int main() {

    digi_enable();
    ana_enable();
    debug_init_LED();
    stdio_init_all(); 
    //bool success = usb_configurate();
    
    flash_configurate_variables(); // configure default from flash.
    dpot_dual_t* DPOT = init_dpot();
    dpot_set_gain(DPOT, 1, 30);
    dpot_set_gain(DPOT, 2, 20);
    deinit_dpot(DPOT);
    run_wav_bme_sequence_single();
    


    /*
    //bool success = usb_configurate();
    dpot_dual_t* DPOT = init_dpot();
    dpot_set_gain(DPOT, 1, 40);
    dpot_set_gain(DPOT, 2, 20);
    deinit_dpot(DPOT);
    //test_bme();
    run_wav_bme_sequence(); //_single();
    */

    //ext_rtc_t* EXT_RTC = rtc_debug();
    //rtc_sleep_until_alarm(EXT_RTC);

    /*
    ext_rtc_t* EXT_RTC = rtc_debug();
    for (int i = 0; i < 1000; i++) {
        rtc_read_time(EXT_RTC);
        rtc_read_string_time(EXT_RTC);
        printf(EXT_RTC->fullstring);
        printf(" is the time... \r\n");
        sleep_ms(1000);
    }
    */
    
    

    
    //time_init();
    //test_cycle();
    //SD_ADC_DMA_testwrite();


    /*
    
    New scheme (which reduces 2-core to 1-core) but relies on a race condition, which will require cycle delay to be considered.
    part 1: ADC runs DMA1, hits 80% of a full cycle (busy_wait_us or something similar) then f_write is triggered. DMA2 is chained to go from DMA1 to the same buffer.
    part 2: ADC runs DMA2, hits 80% of a full cycle (busy_wait_us or something similar) then f_write is triggered. DMA1 is chained to go from DMA2 to the same buffer.
    

    */

}
