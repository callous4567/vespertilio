// Header Guard
#ifndef FLASHLOG
#define FLASHLOG

#include <stdbool.h>
#include <stdint.h>
#include "../ext_rtc/ext_rtc.h"
#include "pico/mutex.h"

#define USE_FLASHLOG false // Set to false if you do not want to use the flashlog.
#define RESET_LOG_POSITION false // set to false if you do not want to get the cache from the previous session CURRENT_PAGE/CURRENT_SESSION (the entire flash area is reset.)

#if (USE_FLASHLOG) 
#define custom_printf flashprintf
#else 
#define custom_printf printf
#endif 

typedef struct {

    int32_t CURRENT_SECTOR;
    int32_t CURRENT_PAGE;
    char* LOG_BUF; 
    char* logbuf;
    ext_rtc_t* EXT_RTC; // inherited by everything else if flashlog is used 
    mutex_t* mutex; // inherited by everything run on core1, in order of priority: flashlog -> EXT_RTC -> VEML -> BME 

} flashlog_t; 

extern flashlog_t* flashlog; 
extern const int32_t FLASH_LOG_SIZE;

/*
Use logbuf for snprintfs/etc/logstrings.
*/
void init_flashlog(void); // run to initialize the flashlog 
void write_to_log(char* log); // write log element to flashlog (should only be used internally by flashprintf/flashpanic)
void flashprintf(const char *format, ...); // equivalent to printf but with log 
void flashpanic(const char *format, ...); // equivalent to panic but with log 
void deinit_flashlog(void); // run at end of everything else to deinit the flashlog
void flashlog_seridump(void); // dump flashlog to serial 
void testflashlog(void); // misc 

#endif // FLASHLOG