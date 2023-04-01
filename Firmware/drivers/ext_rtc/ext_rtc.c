#include "ext_rtc.h"
#include "ext_rtc_registers.h"
#include "../Utilities/utils.h"
#include "hardware/gpio.h"
#include "malloc.h"
#include "pico/time.h"
#include "pico/sleep.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

// the pin used to power the pullups used for I2C communication to not only the RTC, but also the VEML light sensing module from Vishay 
static const int PULLUP_PIN = 10; // pin number 14, GPIO 10, on Version 4 of the PCB 

static void enable_external_pulls(void) {

    // set the pulls on the internals to off, too
    gpio_set_pulls(RTC_SDA_PIN, false, false);
    gpio_set_pulls(RTC_SCK_PIN, false, false);
    gpio_set_pulls(RTC_INT_PIN, false, false);

    // configure the external pullup setup 
    gpio_set_pulls(PULLUP_PIN, false, false);
    gpio_init(PULLUP_PIN);
    gpio_set_dir(PULLUP_PIN, GPIO_OUT);
    gpio_put(PULLUP_PIN, 1);
    gpio_set_drive_strength(PULLUP_PIN, GPIO_DRIVE_STRENGTH_8MA);
    busy_wait_ms(100); // for capacitor inrush 
    

}
static void disable_external_pulls(void) {

    // pull all the pins down by default
    gpio_set_pulls(RTC_SDA_PIN, false, true);
    gpio_set_pulls(RTC_SCK_PIN, false, true);
    gpio_set_pulls(RTC_INT_PIN, false, true);

    // disable the external pullup setup 
    gpio_put(PULLUP_PIN, 0);
    gpio_deinit(PULLUP_PIN);
    gpio_set_pulls(PULLUP_PIN, false, true);

}

// free the rtc (and turn of all the external pullups in the process)
void rtc_free(ext_rtc_t* EXT_RTC) {

    i2c_deinit(EXT_RTC->hw_inst); // deinit the i2c instance (v. important.)
    free(EXT_RTC->timebuf);
    free(EXT_RTC->alarmbuf);
    free(EXT_RTC->fullstring);
    free(EXT_RTC);
    disable_external_pulls(); // disable the pullup setup 

}

// convert a 4-bit nibble (inside a uint8_t right bits) to BCD representation 
static uint8_t toBCD_sub(uint8_t a) {
    switch(a) {

        case 0  :
            return 0x00;
            break;
        
        case 1  :
            return 0b00000001;
            break;
        
        case 2  : 
            return 0b00000010;
            break;

        case 3  :
            return 0b00000011;
            break;
        
        case 4  :
            return 0b00000100;
            break;
        
        case 5  :
            return 0b00000101;
            break;

        case 6  :
            return 0b00000110;
            break;

        case 7  :
            return 0b00000111;
            break;

        case 8  :
            return 0b00001000;
            break;

        case 9  :
            return 0b00001001;
            break;

        default:
            panic("BCD lookup failed. Is your value under 10 or not?, %d", a);
            break;

    }

}

// convert a full 8-bit of two nibbles into packed BCD 
static uint8_t toBCD(uint8_t a) {

    return (toBCD_sub(a/10)<<4) | toBCD_sub(a%10);
}

// the reverse (BCD to int!) but sub 
static uint8_t fromBCD_sub(uint8_t a) {

    switch(a) {

        case 0x00  :
            return 0;
            break;
        
        case 0b00000001  :
            return 1;
            break;
        
        case 0b00000010  : 
            return 2;
            break;

        case 0b00000011  :
            return 3;
            break;
        
        case 0b00000100  :
            return 4;
            break;
        
        case 0b00000101  :
            return 5;
            break;

        case 0b00000110  :
            return 6;
            break;

        case 0b00000111  :
            return 7;
            break;

        case 0b00001000  :
            return 8;
            break;

        case 0b00001001  :
            return 9;
            break;

        default:
            panic("BCD lookup failed. Is your value under 10 or not?, %d", a);
            break;

    }


}

// convert a packed 2-BCD BCD to uint8_t 
static uint8_t fromBCD(volatile uint8_t a) {
    uint8_t result;
    result = 10*fromBCD_sub(a>>4);
    result += fromBCD_sub(a & 0b00001111);
    return result;
}

// convert binary-coded-decimal with two digits to standard uint8_t 
static inline uint8_t touint8(uint8_t a) {
    return a;
}

/*
READ register ADDRESS to RESULT 
ADDRESS defined in EXT_RC.H 
RESULT must be POINTER
*/
static void rtc_register_read(
    ext_rtc_t *EXT_RTC, 
    uint8_t address, 
    uint8_t *result,
    uint8_t len 
    ) {

    uint8_t* address_ptr;
    address_ptr = &address;

    int number_written;
    number_written = i2c_write_blocking(
        EXT_RTC->hw_inst, 
        RTC_SLAVE_ADDRESS, 
        address_ptr, 
        1,
        false
    );

    int number_read;
    number_read = i2c_read_blocking(
        EXT_RTC->hw_inst, 
        RTC_SLAVE_ADDRESS, 
        result,
        len,
        false
    );

}

/*
WRITE DATA to ADDRESS. 
For convenience we haven't required the pre-inclusion of the address in the data,
Given it is a pointer, we thusly need to allocate on the stack a copy with this included 
*/
static void rtc_register_write(
    ext_rtc_t *EXT_RTC,
    uint8_t address,
    uint8_t *data,
    uint8_t len
) {

    uint8_t data_buff[len + 1];
    data_buff[0] = address;
    for (int i=0; i < len; i++) {
        data_buff[i+1] = *(data+i);
    }
    uint8_t *data_buff_ptr = &data_buff[0];

    i2c_write_blocking(
        EXT_RTC->hw_inst,
        RTC_SLAVE_ADDRESS,
        data_buff_ptr,
        len+1,
        false
    );

}

// Initialize the RTC object default. Returns it. MALLOC!!!!
ext_rtc_t* init_RTC_default(void) {

    // set all internal pulls to off since we are using external pullups 
    enable_external_pulls();

    // set up rtc object 
    ext_rtc_t *EXT_RTC;
    EXT_RTC = (ext_rtc_t*)malloc(sizeof(ext_rtc_t));
    EXT_RTC->sda = RTC_SDA_PIN;
    EXT_RTC->scl = RTC_SCK_PIN;
    EXT_RTC->ext_int = RTC_INT_PIN;
    EXT_RTC->hw_inst = RTC_I2C;
    EXT_RTC->baudrate = RTC_BAUD;

    // i2c setup
    i2c_init(
        EXT_RTC->hw_inst, 
        EXT_RTC->baudrate
    );

    // Set pin functions
    gpio_set_function(EXT_RTC->sda, GPIO_FUNC_I2C);
    gpio_set_function(EXT_RTC->scl, GPIO_FUNC_I2C);

    // malloc up the timebuf too (7 bytes. See datasheet top-down of registers.)
    EXT_RTC->timebuf = (uint8_t*)malloc(7);

    // malloc up the alarmbuf too
    EXT_RTC->alarmbuf = (uint8_t*)malloc(4);

    // set all internal pulls to off since we are using external pullups 
    enable_external_pulls();

    // SET DEFAULT CONTROL REGISTER 

    /*
    BIT
    7: set to 0 to start oscillator
    6: n/a
    5: whether to interrupt when we ran out of battery juice: set t0 
    4-3: Square-wave alarm frequency. 
    2: Whether we enable the alarm activation- we want to. Set to 1.
    1: Set to 1 to enable alarm 2
    0: Set to 1 to enable alarm 2. We only need alarm 1 though so set to 0.
    */
    uint8_t RTC_DEFAULT_CONTROL = 0b00011101;
    uint8_t *RTC_DEFAULT_CONTROL_ptr = &RTC_DEFAULT_CONTROL;

    // write our new register
    rtc_register_write(
        EXT_RTC,
        RTC_CONTROL,
        RTC_DEFAULT_CONTROL_ptr,
        1
    );


    // SET DEFAULT TRICKLE REGISTER 

    /*
    0b10101001 - ONE DIODE 200 OHM RESISTOR 
    3.3 V - ~1V / 200 ~ mA charging of the RTC supercapacitor 
    Necessary to reduce inflow current.
    */

    uint8_t RTC_DEFAULT_TRICKLE = 0b00000110;
    uint8_t *RTC_DEFAULT_TRICKLE_ptr = &RTC_DEFAULT_CONTROL;

    // write trickle register 
    rtc_register_write(
        EXT_RTC,
        RTC_TRICKLE,
        RTC_DEFAULT_TRICKLE_ptr,
        1
    );

    // The internal fullstring too
    EXT_RTC->fullstring = (char*)malloc(22);

    // default status
    rtc_default_status(EXT_RTC);

    // return the malloc'd pointer :)
    return EXT_RTC;

}

// Set the alarm from the alarmbuf of uint8_ts.
static void rtc_set_alarm1(
    ext_rtc_t *EXT_RTC
) {

    // first convert alarmbuf from regular int to BCD
    *EXT_RTC->alarmbuf = toBCD(*EXT_RTC->alarmbuf);
    *(EXT_RTC->alarmbuf+1) = toBCD(*(EXT_RTC->alarmbuf+1));
    *(EXT_RTC->alarmbuf+2) = toBCD(*(EXT_RTC->alarmbuf+2));
    *(EXT_RTC->alarmbuf+3) = toBCD(*(EXT_RTC->alarmbuf+3));

    // Set alarm seconds, minutes, and hours 
    rtc_register_write(
        EXT_RTC,
        RTC_ALARM_1_SECONDS,
        EXT_RTC->alarmbuf,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_ALARM_1_MINUTES,
        EXT_RTC->alarmbuf+1,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_ALARM_1_HOURS,
        EXT_RTC->alarmbuf+2,
        1
    );

    /*
    To have it go off every day
    we need to set bit 7 to unity for the day-date to be ignored
    See the datasheet, page 24.

    "
    The Day, /Date bits (bit 6 of the alarm day/date registers) control whether the alarm value stored in bits 0 ~ 5 of that register
    reflects the day of the week or the date of the month. If the bit is written to logic 0, the alarm is the result of a match with date of
    the month. If the bit is written to logic 1, the alarm is the result of a match with day of the week. 
    "

    We will use day-of-the-week to trigger the alarm (eventually we will include an alarm to configure for day/date, so yeah- this works best.)
    Note: when you set "Day of the Week" in timebuf, it's arbitrary- monday or tuesday or whatever could be DOTW=1, etc etc...
    It just counts that DOTW up to 7 then resets, that's it.
    Anyway,

    Set day-date of the alarm to...
    0b11000000

    TODO:
    - Make sure that you can set the alarm day too, if this functionality is desired!


    */
    uint8_t daydate;
    uint8_t *daydate_ptr = &daydate; 
    daydate = 0b11000000 | *EXT_RTC->alarmbuf+3;
    rtc_register_write(
        EXT_RTC,
        RTC_ALARM_1_DAYDATES,
        daydate_ptr,
        1
    );

}

/*
timebuf is a pointer to a 7-uint8_t-long timeset
takes the form in the datasheet:
SECONDS 00:59
MINUTES 00:59
HOURS 00:23
DOTW 01:07
DOTM 01:31
MONTH 01:12
YEAR 00:99
note: this form is for the 24-hour-system. there is nonsense for AM/PM too.
*/
static void rtc_set_current_time(
    ext_rtc_t *EXT_RTC
) {
    
    // first convert our standard int time to BCD time
    *EXT_RTC->timebuf = toBCD(*EXT_RTC->timebuf);
    *(EXT_RTC->timebuf+1) = toBCD(*(EXT_RTC->timebuf+1));
    *(EXT_RTC->timebuf+2) = toBCD(*(EXT_RTC->timebuf+2));
    *(EXT_RTC->timebuf+3) = toBCD(*(EXT_RTC->timebuf+3));
    *(EXT_RTC->timebuf+4) = toBCD(*(EXT_RTC->timebuf+4));
    *(EXT_RTC->timebuf+5) = toBCD(*(EXT_RTC->timebuf+5));
    *(EXT_RTC->timebuf+6) = toBCD(*(EXT_RTC->timebuf+6));

    rtc_register_write(
        EXT_RTC,
        RTC_SECONDS,
        EXT_RTC->timebuf,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_MINUTES,
        EXT_RTC->timebuf+1,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_HOURS,
        EXT_RTC->timebuf+2,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_DOTW,
        EXT_RTC->timebuf+3,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_DOTM,
        EXT_RTC->timebuf+4,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_MONTHS,
        EXT_RTC->timebuf+5,
        1
    );
    rtc_register_write(
        EXT_RTC,
        RTC_YEARS,
        EXT_RTC->timebuf+6,
        1
    );

}

// Read current time saving to uint8_t timebuf[7]
static void rtc_read_time(
    ext_rtc_t *EXT_RTC
) {

    rtc_register_read(
        EXT_RTC,
        RTC_SECONDS,
        EXT_RTC->timebuf,
        1
    );
    rtc_register_read(
        EXT_RTC,
        RTC_MINUTES,
        EXT_RTC->timebuf+1,
        1
    );
    rtc_register_read(
        EXT_RTC,
        RTC_HOURS,
        EXT_RTC->timebuf+2,
        1
    );
    rtc_register_read(
        EXT_RTC,
        RTC_DOTW,
        EXT_RTC->timebuf+3,
        1
    );
    rtc_register_read(
        EXT_RTC,
        RTC_DOTM,
        EXT_RTC->timebuf+4,
        1
    );
    rtc_register_read(
        EXT_RTC,
        RTC_MONTHS,
        EXT_RTC->timebuf+5,
        1
    );
    rtc_register_read(
        EXT_RTC,
        RTC_YEARS,
        EXT_RTC->timebuf+6,
        1
    );

    // Quick debug step
    /*
    printf("TIMEBUF DEBUG!!!\r\n");
    for (int i = 0; i < 7; i++) {
        toBinary(*(EXT_RTC->timebuf+i));
    }
    printf("TIMEBUF DEBUG DONE!");
    */
    // convert all our read values from BCD to standard int
    *EXT_RTC->timebuf = fromBCD(*EXT_RTC->timebuf);
    *(EXT_RTC->timebuf+1) = fromBCD(*(EXT_RTC->timebuf+1));
    *(EXT_RTC->timebuf+2) = fromBCD(*(EXT_RTC->timebuf+2));
    *(EXT_RTC->timebuf+3) = fromBCD(*(EXT_RTC->timebuf+3));
    *(EXT_RTC->timebuf+4) = fromBCD(*(EXT_RTC->timebuf+4));
    *(EXT_RTC->timebuf+5) = fromBCD(*(EXT_RTC->timebuf+5));
    *(EXT_RTC->timebuf+6) = fromBCD(*(EXT_RTC->timebuf+6));


}

// read the current time from RTC, in integer format, to the timestring/fullstring. You do not need to call rtc_read_time before this.
void rtc_read_string_time(ext_rtc_t *EXT_RTC) {
    /*
    the maximum number of chars/bytes written is...
    2 for second 
    2 for minute
    2 for hour
    2 for day
    2 for month
    2 for year 
    2 per spacer w/ 5 spacers = 22
    hence 22 bytes 
    */

    rtc_read_time(EXT_RTC);

    // USE THIS
    snprintf(
        EXT_RTC->fullstring,
        22,
        "%d_%d_%d_%d_%d_%d", 
        *(EXT_RTC->timebuf),
        *(EXT_RTC->timebuf+1),
        *(EXT_RTC->timebuf+2),
        *(EXT_RTC->timebuf+4),
        *(EXT_RTC->timebuf+5),
        *(EXT_RTC->timebuf+6)
    );

}

// enable the rosc clock 
static void rosc_enable(void)
{
    uint32_t tmp = rosc_hw->ctrl;
    tmp &= (~ROSC_CTRL_ENABLE_BITS);
    tmp |= (ROSC_CTRL_ENABLE_VALUE_ENABLE << ROSC_CTRL_ENABLE_LSB);
    rosc_write(&rosc_hw->ctrl, tmp);
    // Wait for stable
    while ((rosc_hw->status & ROSC_STATUS_STABLE_BITS) != ROSC_STATUS_STABLE_BITS);
}

// recover clocks after sleep (only necessary if using XOSC sleep- keep it here though just in case.)
static void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){

    //Re-enable ring Oscillator control
    rosc_enable();

    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;

    //reset clocks
    clocks_init();
    //stdio_uart_init();

    return;
}

// enter dormant mode by rosc until RTC alarm is triggered 
void rtc_sleep_until_alarm(ext_rtc_t *EXT_RTC) {

    //save values for later
    uint scb_orig = scb_hw->scr;
    uint clock0_orig = clocks_hw->sleep_en0;
    uint clock1_orig = clocks_hw->sleep_en1;

    // Wait for the fifo to be drained so we get reliable output
    //uart_default_tx_wait_blocking();

    // go to sleep + then wait until dormant.
    printf("Doing ROSC sleep\r\n");
    sleep_run_from_rosc();
    sleep_goto_dormant_until_pin(EXT_RTC->ext_int, true, false);
    printf("Recovering...\r\n");
    recover_from_sleep(scb_orig, clock0_orig, clock1_orig);

    // Wait for the fifo to be drained so we get reliable output
    //uart_default_tx_wait_blocking();

}

void rtc_default_status(ext_rtc_t* EXT_RTC) {

    uint8_t status_result = 0b00000000; // default status 
    uint8_t* statres = &status_result;

    // Write the default status 
    rtc_register_write(
        EXT_RTC,
        RTC_STATUS,
        statres,
        1
    );

}

// just generates a conveniently programmed RTC for debugging purposes (malloc'd obviously/etc.) Alarm 15 seconds after init.
ext_rtc_t* rtc_debug(void) {
    
    ext_rtc_t *EXT_RTC = init_RTC_default();

    
    *EXT_RTC->alarmbuf = 15;
    *(EXT_RTC->alarmbuf+1) = 0;
    *(EXT_RTC->alarmbuf+2) = 1;
    *(EXT_RTC->alarmbuf+3) = 1; 
    rtc_set_alarm1(EXT_RTC);

    *EXT_RTC->timebuf = 0;
    *(EXT_RTC->timebuf+1) = 0;
    *(EXT_RTC->timebuf+2) = 1;
    *(EXT_RTC->timebuf+3) = 1;
    *(EXT_RTC->timebuf+4) = 1;
    *(EXT_RTC->timebuf+5) = 1;
    *(EXT_RTC->timebuf+6) = 0;
    rtc_set_current_time(EXT_RTC);
    
    // Write the status register default 
    rtc_default_status(EXT_RTC);

    return EXT_RTC;

}

// will initialize the internal RTC and set it to the internal time of the provided ext_rtc_t. you need to set the ext_rtc time first. returns a malloc.
datetime_t* init_pico_rtc(ext_rtc_t* EXT_RTC) {

    rtc_init();
    datetime_t* dtime = (datetime_t*)malloc(sizeof(datetime_t));
    dtime->sec   =   *(EXT_RTC->timebuf    )          ; // We have set the configuration code up appropriately so that sunday is DOTW=1 on the ext RTC and 0 on the int RTC.
    dtime->min   =   *(EXT_RTC->timebuf + 1)          ; // USE 0 FOR SUNDAY ON THE EXTERNAL RTC!!! IMPORTANT!!!
    dtime->hour  =   *(EXT_RTC->timebuf + 2)          ; // NOTE THAT DAY OF THE WEEK ON THE PICO HAS 0 FOR SUNDAY! 
    dtime->dotw  =   *(EXT_RTC->timebuf + 3) - 1      ; // the external RTC has day of the week [01,07] 
    dtime->day   =   *(EXT_RTC->timebuf + 4)          ; // https://raspberrypi.github.io/pico-sdk-doxygen/structdatetime__t.html
    dtime->month =   *(EXT_RTC->timebuf + 5)          ; // the pi pico has [0,6] though 
    dtime->year  =   *(EXT_RTC->timebuf + 6) + 2000   ; // on the pico, year is [0,4095] so yeah , on the RTC [00,99]
    rtc_set_datetime(dtime);
    return dtime;

}

// update the internal RTC of the pico from the provided ext_rtc_t. you need to set the ext_rtc time first.
void update_pico_rtc(ext_rtc_t* EXT_RTC, datetime_t* dtime) {

    dtime->sec   =   *(EXT_RTC->timebuf    )          ;
    dtime->min   =   *(EXT_RTC->timebuf + 1)          ; 
    dtime->hour  =   *(EXT_RTC->timebuf + 2)          ; 
    dtime->dotw  =   *(EXT_RTC->timebuf + 3) - 1      ; 
    dtime->day   =   *(EXT_RTC->timebuf + 4)          ; 
    dtime->month =   *(EXT_RTC->timebuf + 5)          ; 
    dtime->year  =   *(EXT_RTC->timebuf + 6) + 2000   ; 
    rtc_set_datetime(dtime);

}

/* set default time to RTC from buffer. specific to use in USB implementation. creates and frees an EXT_RTC.*/
void configure_rtc(int32_t* configuration_buffer, int32_t CONFIGURATION_BUFFER_INDEPENDENT_VALUES) {

    // init a default RTC object 
    ext_rtc_t* EXT_RTC = init_RTC_default();

    // set default time 
    *EXT_RTC->timebuf     = *(configuration_buffer+CONFIGURATION_BUFFER_INDEPENDENT_VALUES);
    *(EXT_RTC->timebuf+1) = *(configuration_buffer+CONFIGURATION_BUFFER_INDEPENDENT_VALUES+1);
    *(EXT_RTC->timebuf+2) = *(configuration_buffer+CONFIGURATION_BUFFER_INDEPENDENT_VALUES+2);
    *(EXT_RTC->timebuf+3) = *(configuration_buffer+CONFIGURATION_BUFFER_INDEPENDENT_VALUES+3);
    *(EXT_RTC->timebuf+4) = *(configuration_buffer+CONFIGURATION_BUFFER_INDEPENDENT_VALUES+4);
    *(EXT_RTC->timebuf+5) = *(configuration_buffer+CONFIGURATION_BUFFER_INDEPENDENT_VALUES+5);
    *(EXT_RTC->timebuf+6) = *(configuration_buffer+CONFIGURATION_BUFFER_INDEPENDENT_VALUES+6);
    rtc_set_current_time(EXT_RTC);

    // set the RTC on appropriately 
    rtc_default_status(EXT_RTC);

    // free the RTC
    rtc_free(EXT_RTC);

}

/**
 * Set the alarm for EXT_RTC following the usual format from the USB configurator for the given WHICH_ALARM_ONEBASED from the provided configuration_buffer
 * Will set alarm, then enter dormancy until the alarm goes off. 
 * After waking up, will free the internal EXT_RTC.
 * Note that this self-disables the digital assembly. 
 * **/
void rtc_setsleep_WHICH_ALARM_ONEBASED(int32_t* configuration_buffer, int32_t CONFIGURATION_BUFFER_INDEPENDENT_VALUES, int32_t WHICH_ALARM_ONEBASED) {

    // init a default RTC object 
    ext_rtc_t* EXT_RTC = init_RTC_default();

    *EXT_RTC->alarmbuf = 0; // always set second to zero so who cares 
    *(EXT_RTC->alarmbuf+1) = *(configuration_buffer + CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + (3*WHICH_ALARM_ONEBASED) - 1); // the minute
    *(EXT_RTC->alarmbuf+2) = *(configuration_buffer + CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + (3*WHICH_ALARM_ONEBASED) - 2); // the hour 
    *(EXT_RTC->alarmbuf+3) = 1; // repeat every day, hence irrelevant  
    rtc_set_alarm1(EXT_RTC);

    // set the RTC on appropriately 
    rtc_default_status(EXT_RTC);

    // sleep + then wake up
    rtc_sleep_until_alarm(EXT_RTC);

    // we're awake! free the RTC and continue.
    rtc_free(EXT_RTC);

}

void alarmtest(void) {

    printf("Generating ext rtc\r\n");
    ext_rtc_t* EXT_RTC = rtc_debug();
    printf("Sleeping\r\n");
    rtc_sleep_until_alarm(EXT_RTC);
    printf("Woke up.\r\n");

}


/* FOR THE ALARMS NOTE SOME THINGS
- change code to account for internal pullups on all the lines- DONE 
- this will mean that we are independent from the digital powerline/etc entirely
- note DO NOT CONNECT on ver. 3 of the PCB for these pullups.
This is necessary to ensure we do not need to deal with digi at all when it comes to the RTC.
*/