#include "ext_rtc.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "malloc.h"
#include <string.h>

// proxy for i2c_init 
static inline void init_rtc(ext_rtc_t *EXT_RTC) {

    // initialize i2c. default to master-mode. 
    i2c_init(
        EXT_RTC->hw_inst, 
        EXT_RTC->baudrate
    );

    // Set pin functions
    gpio_set_function(EXT_RTC->sda, GPIO_FUNC_I2C);
    gpio_set_function(EXT_RTC->scl, GPIO_FUNC_I2C);

}

// Print as binary the individual 8-bit byte a 
static void toBinary(uint8_t a) {
    uint8_t i;

    for(i=0x80;i!=0;i>>=1)
        printf("%c",(a&i)?'1':'0'); 
    printf("\r\n");
}

// Print as binary the individual 16-bit byte a 
static void toBinary_16(uint16_t a) {

    for (int i = 0; i < 16; i++) {
        printf("%d", (a & 0x8000) >> 15);
        a <<= 1;
    }
    printf("\r\n");
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
            panic("BCD lookup failed. Is your value under 10 or not?");
            break;

    }

}

// convert a full 8-bit of two nibbles into packed BCD 
uint8_t toBCD(uint8_t a) {

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
            panic("BCD lookup failed. Is your value under 10 or not?");
            break;

    }


}

// convert a packed 2-BCD BCD to uint8_t 
uint8_t fromBCD(volatile uint8_t a) {
    uint8_t result;
    result = 10*fromBCD_sub(a>>4);
    result += fromBCD_sub(a & 0b00001111);
    return result;
}


// convert binary-coded-decimal with two digits to standard uint8_t 
static inline uint8_t touint8(uint8_t a) {
    return a;
}

// Initialize the RTC object default. Returns it. MALLOC!!!!
ext_rtc_t* init_RTC_default(void) {

    // set up rtc object 
    ext_rtc_t *EXT_RTC;
    EXT_RTC = (ext_rtc_t*)malloc(sizeof(ext_rtc_t));
    EXT_RTC->sda = RTC_SDA_PIN;
    EXT_RTC->scl = RTC_SCK_PIN;
    EXT_RTC->ext_int = RTC_INT_PIN;
    EXT_RTC->hw_inst = RTC_I2C;
    EXT_RTC->baudrate = RTC_BAUD;
    init_rtc(EXT_RTC);

    // malloc up the timebuf too (7 bytes. See datasheet top-down of registers.)
    EXT_RTC->timebuf = (uint8_t*)malloc(7);


    // SET DEFAULT CONTROL REGISTER 

    /*
    BIT
    7: set to 0 to start oscillator
    6: n/a
    5: n/a
    4-3: Square-wave frequency. Irrelevant to set (we won't use square-wave output.)
    2: Whether we enable the alarm activation- we want to. Set to 1.
    1: Set to 1 to enable alarm 1
    0: Set to 1 to enable alarm 2. We only need alarm 1 though so set to 0.
    */
    uint8_t RTC_DEFAULT_CONTROL = 0b00000110;
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

    // Set some example times on the timebuf just for the sake of giving us some filenames/etc to work with. 
    /*
    *EXT_RTC->timebuf = 0;
    *(EXT_RTC->timebuf+1) = 59;
    *(EXT_RTC->timebuf+2) = 22;
    *(EXT_RTC->timebuf+4) = 8;
    *(EXT_RTC->timebuf+5) = 12;
    *(EXT_RTC->timebuf+6) = 22;
    rtc_set_current_time(EXT_RTC);
    */

    // return the malloc'd pointer :)
    return EXT_RTC;

}

/*
READ register ADDRESS to RESULT 
ADDRESS defined in EXT_RC.H 
RESULT must be POINTER
*/
void rtc_register_read(
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
void rtc_register_write(
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
void rtc_set_current_time(
    ext_rtc_t *EXT_RTC
) {
    
    // first convert our standard int time to BCD time
    *EXT_RTC->timebuf = toBCD(*EXT_RTC->timebuf);
    *(EXT_RTC->timebuf+1) = toBCD(*(EXT_RTC->timebuf+1));
    *(EXT_RTC->timebuf+2) = toBCD(*(EXT_RTC->timebuf+2));
    *(EXT_RTC->timebuf+3) = toBCD(*(EXT_RTC->timebuf+2));
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
void rtc_read_time(
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

    // convert all our read values from BCD to standard int
    *EXT_RTC->timebuf = fromBCD(*EXT_RTC->timebuf);
    *(EXT_RTC->timebuf+1) = fromBCD(*(EXT_RTC->timebuf+1));
    *(EXT_RTC->timebuf+2) = fromBCD(*(EXT_RTC->timebuf+2));
    *(EXT_RTC->timebuf+3) = fromBCD(*(EXT_RTC->timebuf+2));
    *(EXT_RTC->timebuf+4) = fromBCD(*(EXT_RTC->timebuf+4));
    *(EXT_RTC->timebuf+5) = fromBCD(*(EXT_RTC->timebuf+5));
    *(EXT_RTC->timebuf+6) = fromBCD(*(EXT_RTC->timebuf+6));


}

// use malloc for return of string (free later.) 
char* rtc_read_string_time(ext_rtc_t *EXT_RTC) {
    /*
    the maximum number of chars/bytes written is...
    2 for second 
    2 for minute
    2 for hour
    1 for day
    2 for month
    2 for year 
    2 per spacer w/ 5 spacers = 21
    hence 16 bytes 
    */
    char *fullstring;
    fullstring = (char*)malloc(21);
    
    // USE THIS
    snprintf(
        fullstring,
        21,
        "%d_%d_%d_%d_%d_%d", 
        *(EXT_RTC->timebuf),
        *(EXT_RTC->timebuf+1),
        *(EXT_RTC->timebuf+2),
        *(EXT_RTC->timebuf+4),
        *(EXT_RTC->timebuf+5),
        *(EXT_RTC->timebuf+6)
    );

    return fullstring;

}
