extern "C" {
#include "drivers/Utilities/utils.h"
#include "drivers/mSD/mSD.h"
#include "drivers/ext_rtc/ext_rtc.h"
#include "drivers/recording/recording.h"
#include "drivers/mcp4131_digipot/spi_driver.h"
#include "drivers/bme280/bme280_spi.h"
#include "drivers/pico_usb_configure/vespertilio_usb_int.h"
}
#include "malloc.h"
#include "pico/multicore.h"

// Run a test of the SD functionality 
int main() {

    digi_enable();
    ana_enable();
    stdio_init_all(); 
    bool flashresult = configure_flash();



    //dpot_dual_t* DPOT = init_dpot();
    //dpot_set_gain(DPOT, 1, 20);
    //dpot_set_gain(DPOT, 2, 20);
    //deinit_dpot(DPOT);
    //test_bme();
    //run_wav_bme_sequence();
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
    


}
