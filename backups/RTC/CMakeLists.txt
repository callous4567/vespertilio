# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Set name of project (as PROJECT_NAME) and C/C   standards
project(RTC C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    RTC.c
    ext_rtc.c
    ext_rtc_registers.c
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_util
    pico_multicore
    hardware_spi
    hardware_vreg
    hardware_clocks
    hardware_rtc
    hardware_adc
    hardware_rtc
    hardware_dma
    hardware_i2c
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# COMMAND TO MAKE/BUILD/ETC the Cmake files 
# cmake -G "MinGW Makefiles" .. 
# After that, run 
# make 
# Note about using the debugger: You need to include a sleep_ms() to ensure that the debugger has time to attach- 1,000 ms is good.
# 115200 baudrate for serial 