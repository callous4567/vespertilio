#ifndef RECORDING_STHREAD
#define RECORDING_STHREAD 

extern "C" {
    #include "../mSD/mSD.h"
    #include "../ext_rtc/ext_rtc.h"
    #include "../veml/i2c_driver.h"
    #include "../Utilities/pinout.h"
}

/*

Note that for the environmental file, we have the TIME_BME_VEML format 
^_^

*/
// run the sequence. initialize this at the time the recordings should start. I recommend starting the recordings 20-30 minutes beforehand to allow all hardware to equalize/self-heat.
void run_wav_bme_sequence_single();
void test_read();
void test_write();

#endif // RECORDING_STHREAD 