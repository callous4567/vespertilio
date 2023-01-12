// Header Guard
#ifndef VTILIO_USB
#define VTILIO_USB

#include "pico/stdlib.h"
#include "tusb.h"
#include "pico/stdio/driver.h"
#include "pico/stdio_usb.h"
#include "pico/stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "malloc.h"
#include "hardware/gpio.h"
#include "../Utilities/utils.h"
#include "../ext_rtc/ext_rtc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

bool usb_configurate(void);

#endif // VTILIO_USB