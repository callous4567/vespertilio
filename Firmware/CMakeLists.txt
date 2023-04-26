# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)
# set(CMAKE_BUILD_TYPE "Release")

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# and pico extras 
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

# Creates a pico-sdk subdirectory in our project for the libraries
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
pico_sdk_init()

add_subdirectory(drivers/FatFs_SPI build)

# Set name of project (as PROJECT_NAME) and C/C   standards
project(vespertilio C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    vespertilio.cpp
    drivers/Utilities/utils.c
    drivers/Utilities/external_config.c
    drivers/mSD/mSD.c
    drivers/mSD/hw_config.c
    drivers/ext_rtc/ext_rtc_registers.c 
    drivers/ext_rtc/ext_rtc.c 
    drivers/recording/recording_singlethread.cpp
    drivers/mcp4131_digipot/mcp4131_registers.c
    drivers/mcp4131_digipot/spi_driver.c
    drivers/bme280/bme280_spi.c
    drivers/pico_usb_configure/vespertilio_usb_int.c
    drivers/veml/i2c_driver.c 
    drivers/veml/veml_registers.c
    drivers/Utilities/pinout/v5.c
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
    hardware_adc
    hardware_rtc
    hardware_dma
    FatFs_SPI
    hardware_i2c
    hardware_sleep
    hardware_rosc
    pico_runtime
    pico_stdio_usb
    hardware_flash
    hardware_sync
    hardware_irq
    hardware_uart
    hardware_pio
    hardware_pwm
)

# Enable UART and USB
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)



# COMMAND TO MAKE/BUILD/ETC the Cmake files 
# cmake -G "MinGW Makefiles" .. 
# After that, run 
# make 
# Note about using the debugger: You need to include a sleep_ms() to ensure that the debugger has time to attach- 1,000 ms is good.
# 115200 baudrate for serial 