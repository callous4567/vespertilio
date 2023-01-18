// Header Guard
#ifndef VTILIO_USB
#define VTILIO_USB

#include <stdbool.h>
#include <stdint.h>

bool usb_configurate(void);

int32_t* read_from_flash(void);

#endif // VTILIO_USB