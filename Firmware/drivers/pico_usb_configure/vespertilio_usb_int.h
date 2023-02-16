// Header Guard
#ifndef VTILIO_USB
#define VTILIO_USB

#include <stdbool.h>
#include <stdint.h>

const extern int32_t CONFIGURATION_BUFFER_INDEPENDENT_VALUES; // for external_config.c

int usb_configurate(void);

int32_t* read_from_flash(void);

#endif // VTILIO_USB