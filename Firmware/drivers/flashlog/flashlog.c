#include "../Utilities/utils.h"
#include "tusb.h"
#include "pico/stdio/driver.h"
#include "pico/stdio_usb.h"
#include "pico/stdio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "flashlog.h"
#include <stdarg.h>
#include <unistd.h>

// TODO:
// - Make sure any/all try-catch loops are OUTSIDE the actual recording session (and in main.)
// - This will necessitate keeping track of the progress in a session (i.e. number of recordings to go) and then adjusting the session length appropriately, then re-running the recording session with that amount 
// - This is because we will init/deinit flashlog FROM INSIDE MAIN because the flashpanic calls deinit flashlog (and thus the try-catch will catch the panic, and we need to be able to try-catch.) After the catch, reinit the flashlog + try to record the remaining minutes.

// INTERNAL VARIABLES
static const int32_t CYCLIC_BUFF_SECTORS = 16; // Number of sectors for logging. One sector is 4 kB (which is 16*256 byte FLASH_PAGE_SIZE)
static const int32_t LOG_BUFF_OFFSET = PICO_FLASH_SIZE_BYTES - (CYCLIC_BUFF_SECTORS+2)*FLASH_SECTOR_SIZE; // offset from XIP_BASE max.
static const int32_t LOG_CONFIG_OFFSET = PICO_FLASH_SIZE_BYTES - 2*FLASH_SECTOR_SIZE; // offset from XIP_BASE max for the configuration of the flashlog (i.e. a pair of int8_t's telling us the previous sector and page... yeah, overkill.)
static const int32_t FLASHLOG_MUTEX_TIMEOUT_MS = 1000; // mutex timeout in ms 
const int32_t FLASH_LOG_SIZE = 0.75*FLASH_PAGE_SIZE;
bool PANIC = false;

// TODO
// - flashlog to debug file on microSD needs to be implemented + run at start of every single recording ensemble (capturing the previous ensemble)
// - run this in main somehow rather than elsewhere. 

// IMPORTANT NOTE!!! THE FLASHLOG WILL NOT WORK IF THE EXTERNAL RTC IS NON-FUNCTIONAL, AND IN DEBUGGING THIS SHOULD BE YOUR FIRST PORT OF CALL! 

// WORKING VARIABLES (encapsulated)
flashlog_t* flashlog;

/*
The current flash configuration exists as follows:

START -> PI PICO BASE CODE (DO NOT EDIT) -> PROGRAM CODE -> ... -> XIP_BASE + LOG_BUFF_OFFSET (CYCLIC_BUFF_SECTORS) -> FLASHLOG_CONFIG_SECTOR -> XIP_BASE + CONFIG_FLASH_OFFSET (CONFIGURATION BUFFER SECTOR FOR vespertilio_usb_int) -> END 

int8_t flashlog->CURRENT_SECTOR; // the current sector we're working in, 0-ordered. [0,CYCLIC_BUFF_SECTORS-1].
int8_t flashlog->CURRENT_PAGE; // the current page we're working in, 0-ordered. [0,15]. 16 pages per sector. "The last written page."

Notes:
- You must start an erase on a flash sector boundary (4 kB)
- Each sector has 16 pages (256 B.) 
- You must/can write one page, minumum.
- See https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/ for details.

*/

/*

GUIDE ON HOW TO USE THIS TO WRITE GOOD LOGS! :D :D :D 
- Either provide an external char*, which may be used by other programs (i.e. FatFS)
- Or, use flashlog->logbuf for your char*, formatting it with snprintf externally, then passing it to write_to_log.
- Note that flashlog->LOG_BUF is the INTERNAL BUFFER USED BY THIS BIT OF CODE for formatting purposes with EXT_RTC. Do not touch it. It's static, anyway.
- Note that our logger here uses EXT_RTC as provided- you do not need to give it a timestamp. 

GUIDE ON HOW TO USE THE ACTUAL CODE! :D :D :D 
- flashlog initializes its own RTC, and should only be initialized after clocks have stabilized following an under/overclock (see custoclocks.c/external_config.c)
- flashlog RTC has priority if it exists- else the default RTC in recording_singlethread has priority
- flashlog mutex has priority- EXT_RTC inherits it (*or makes its own of flashlog isn't used.) VEML/BME both inherit mutex from EXT_RTC. 
- the mutex is needed due to the interrupt nature of flash program writing. 
- mutex is released + reset for a flashpanic- recovery necessitates deinit flashlog, deinit everything (EVERYTHING), then restart code appropriately. 

*/

// reset the entire flashlog to 0x00 
static void reset_flash(void) {
    printf("Resetting flashlog\r\n");
    memset(flashlog->LOG_BUF, '\0', FLASH_PAGE_SIZE);
    for (int page; page < (CYCLIC_BUFF_SECTORS + 1)*16; page++) {
        flash_range_program(
            LOG_BUFF_OFFSET + page*FLASH_PAGE_SIZE,
            flashlog->LOG_BUF,
            FLASH_PAGE_SIZE
        );
    }
}

// cache the current page/sector to pick up where we left off in logging last session
static void cache_flash(void) {

    int32_t session_info[64];
    session_info[0] = flashlog->CURRENT_PAGE;
    session_info[1] = flashlog->CURRENT_SECTOR;
    uint8_t* packed = pack_int32_uint8(session_info, 64);
    
    // Disable interrupts https://kevinboone.me/picoflash.html?i=1
    mutex_enter_timeout_ms(flashlog->mutex, FLASHLOG_MUTEX_TIMEOUT_MS);
    uint32_t ints = save_and_disable_interrupts();

    // program the log 
    flash_range_program(
        LOG_CONFIG_OFFSET, 
        packed, 
        FLASH_PAGE_SIZE); // write the config

    // Enable interrupts
    restore_interrupts (ints); // Enable interrupts
    mutex_exit(flashlog->mutex);

    // Free packed
    free(packed);

}

// read the cache from the last session to retrieve current page/sector 
static void read_cache(void) {

    uint8_t* cache = (uint8_t*)(XIP_BASE + LOG_CONFIG_OFFSET);
    int32_t* cache_int = pack_uint8_int32(cache, 2);
    flashlog -> CURRENT_PAGE = *(cache_int);
    flashlog -> CURRENT_SECTOR = *(cache_int + 1);
    free(cache_int);

}

// run this before any logging. Makes a FLASH_PAGE_SIZE malloc for us to write logs to before write_to_log puts them in flash.
void init_flashlog(void) {

    // mallocs 
    flashlog = (flashlog_t*)malloc(sizeof(flashlog_t));
    flashlog->logbuf = (char*)malloc(FLASH_LOG_SIZE);
    flashlog->LOG_BUF = (char*)malloc(FLASH_PAGE_SIZE);

    // the mutex, which must be initialized before initializing the EXT_RTC.
    flashlog->mutex = (mutex_t*)malloc(sizeof(mutex_t));
    mutex_init(flashlog->mutex);

    // set variables 
    memset(flashlog->LOG_BUF, 0, FLASH_PAGE_SIZE);
    memset(flashlog->logbuf, 0, (int8_t)0.75*FLASH_PAGE_SIZE);
    flashlog->EXT_RTC = init_RTC_default();
    // Set the CURRENT_SECTOR/CURRENT_PAGE, or retrieve the previous CURRENT_SECTOR/CURRENT_PAGE from the cache
    if (RESET_LOG_POSITION) {
        flashlog->CURRENT_SECTOR = 0;
        flashlog->CURRENT_PAGE = 0;
        reset_flash();
    } else {
        read_cache();
    }

}

// writes the provided char* log to flashlog->LOG_BUF with snprintf with correct RTC time, then dumps it into the flash appropriately. Only runs if flashlog->LOG_ONGOING is false (and sets to true while running.)
void write_to_log(char* log) {

    // set up the flashlog->logbuf + format/etc 
    memset(flashlog->LOG_BUF, '\0', FLASH_PAGE_SIZE);
    rtc_read_string_time(flashlog->EXT_RTC);
    if (!PANIC) {
        snprintf(
            flashlog->LOG_BUF,
            FLASH_PAGE_SIZE,
            "%s |-| %s", 
            flashlog->EXT_RTC->fullstring,
            log
        ); 
    } else {
        snprintf(
            flashlog->LOG_BUF,
            FLASH_PAGE_SIZE,
            "%s |-| PANIC PANIC PANIC %s", 
            flashlog->EXT_RTC->fullstring,
            log
        ); 
    }

    // flashlog mutex 
    bool entered = mutex_enter_timeout_ms(flashlog->mutex, FLASHLOG_MUTEX_TIMEOUT_MS);

    // Disable interrupts https://kevinboone.me/picoflash.html?i=1
    uint32_t ints = save_and_disable_interrupts();

    // check if we're on a sector boundary. If we are, erase the next sector. Also iterate flashlog->CURRENT_PAGE to the immediate-to-write page. Note that we're 0-based.
    if ((flashlog->CURRENT_PAGE%15==0) && (flashlog->CURRENT_PAGE!=0)) {
        flashlog->CURRENT_SECTOR += 1;
        flash_range_erase(LOG_BUFF_OFFSET + FLASH_SECTOR_SIZE*(flashlog->CURRENT_SECTOR % 4), FLASH_SECTOR_SIZE); // %4 ensures that if we have iterated past 3, then 0 is erased.
        if (flashlog->CURRENT_SECTOR == 4) {
            flashlog->CURRENT_SECTOR = 0;
        }
        flashlog->CURRENT_PAGE = 0;
    } else {
        flashlog->CURRENT_PAGE += 1;
    }

    // program the log 
    flash_range_program(
        LOG_BUFF_OFFSET + FLASH_SECTOR_SIZE*(flashlog->CURRENT_SECTOR) + FLASH_PAGE_SIZE*(flashlog->CURRENT_PAGE), 
        flashlog->LOG_BUF, 
        FLASH_PAGE_SIZE); // write the page

    // Enable interrupts
    restore_interrupts (ints); // Enable interrupts

    // Done.
    mutex_exit(flashlog->mutex);

}

// convenience function to free the flashlog->logbuf
void deinit_flashlog(void) {

    // cache the CURRENT_PAGE/CURRENT_SECTOR 
    cache_flash(); 

    // free everything 
    free(flashlog->mutex);
    free(flashlog->LOG_BUF);
    free(flashlog->logbuf);
    rtc_free(flashlog->EXT_RTC);
    free(flashlog);

}

// dump the entire log to serial port (standalone. Use vespertilio_usb_int to pace it/check it/etc.)
void flashlog_seridump(void) {

    debug_flash_LED(1, 100);
    debug_flash_LED(1, 200);
    fwrite(
        (char*)(XIP_BASE + LOG_BUFF_OFFSET), 
        1,
        CYCLIC_BUFF_SECTORS*FLASH_SECTOR_SIZE,
        stdout
    );

}

// function equivalent to printf that also includes logging of the message (This will printf at the end- do not double that up accidentally.)
void flashprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(
        flashlog->logbuf,
        FLASH_LOG_SIZE,
        format,
        args
    );
    va_end(args);
    write_to_log(flashlog->logbuf);
    printf(flashlog->logbuf);

}

// function equivalent to panic that also includes logging of the message (This will panic at the end- do not double that up accidentally.) Will deinit the flashlog (You need to reinit after calling this.)
void __attribute__((noreturn)) __printflike(1, 0) flashpanic(const char *format, ...) {

    PANIC = true;
    va_list args;
    va_start(args, format);
    vsnprintf(
        flashlog->logbuf,
        FLASH_LOG_SIZE,
        format,
        args
    );
    va_end(args);

    // Forcibly delete + recreate the mutex for our use here (just in case the mutex has been stalled somewhere)
    free(flashlog->mutex);
    flashlog->mutex = (mutex_t*)malloc(sizeof(mutex_t));
    mutex_init(flashlog->mutex);
    flashlog->EXT_RTC->mutex = flashlog->mutex;

    write_to_log(flashlog->logbuf);
    printf(flashlog->LOG_BUF);
    deinit_flashlog();
    PANIC = false;

    _exit(1); // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-exit-end-process-bypass-cleanup

}
