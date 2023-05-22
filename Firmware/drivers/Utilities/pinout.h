#ifndef PINOUT_CUSTOM
#define PINOUT_CUSTOM

#include "hardware/i2c.h"
#include "hardware/spi.h"

#define VERSION_6 // You also need to alter vX.c in CMakeLists.txt 

// chain to decide which include
#ifdef VERSION_5
    #include "pinout/v5.h"
#else 
    #ifdef VERSION_6
        #include "pinout/v6.h"
    #else 
        #include "pinout/v8.h"
    #endif 
#endif

#endif // PINOUT_CUSTOM