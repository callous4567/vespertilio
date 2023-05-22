// Header Guard
#ifndef VTIUSB
#define VTIUSB

#include "../Utilities/universal_includes.h"

const extern int32_t CONFIGURATION_BUFFER_INDEPENDENT_VALUES; // for external_config.c

int8_t main_USB(void);

int32_t usb_configurate(void);

int32_t* read_from_flash(void);

void write_default_test_configuration(void); 
#endif // VTIUSB