#include "ext_rtc.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"
#include "hardware/rtc.h"
#include "malloc.h"
#include "pico/time.h"

// Print as binary the individual 16-bit byte a 
static void toBinary_16(uint16_t a) {
    uint16_t i;

    for (int i = 0; i < 16; i++) {
        printf("%d", (a & 0x8000) >> 15);
        a <<= 1;
    }
    printf("\r\n");
}

// Print as binary the individual 8-bit byte a 
void toBinary(uint8_t a) {
    uint8_t i;

    for(i=0x80;i!=0;i>>=1)
        printf("%c",(a&i)?'1':'0'); 
    printf("\r\n");
}

int main() {

    stdio_init_all();

    // set up rtc object 
    ext_rtc_t *EXT_RTC = init_RTC_default();
    *EXT_RTC->timebuf = 0;
    *(EXT_RTC->timebuf+1) = 59;
    *(EXT_RTC->timebuf+2) = 22;
    *(EXT_RTC->timebuf+4) = 8;
    *(EXT_RTC->timebuf+5) = 12;
    *(EXT_RTC->timebuf+6) = 22;
    rtc_set_current_time(EXT_RTC);

    // Read time + print in a loop
    rtc_read_time(EXT_RTC);
    for (int i = 0; i < 3600; i++) {
        rtc_read_time(EXT_RTC);
        printf(rtc_read_string_time(EXT_RTC));
        printf("\r\n");
        sleep_ms(1000);
    }

    // Okay. binary coded decimal. let's fucking go.

}