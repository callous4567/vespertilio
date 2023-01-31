![](https://github.com/callous4567/Batcorder/blob/main/design_bat.png)

## General description 
A mid-spec battery-powered bat-recorder with an environmental sensor on the board, featuring the BME280 and Knowles Winfrey paired with a Pi Pico, with low-loss parts! Hopefully it'll be a great bat recording device! Hopefully!

This is my first time with C (embedded or otherwise) and with a bunch of other things- PCBs, electronics, 3D cases, so you will have to bare with me whilst I work through this project bit by bit 😅

## Features/Hardware 
- BME280 for Environmental Sensing (Temp, Pressure, Humidity)
- Knowles Winfrey Microphone with Differential Amplifier -> Inverting Amplifier Cascade for audio
- Variable gain on the inverting amplifier w/ fixed gain on the differential for better noise performance 
- Low-power-standby of ~1mA dormancy when awaiting recording sessions
- Various passive filters in circuitry to minimize noise, paired with first-order high-pass filter on ADC input
- Compatible with any/all MicroSD cards ostensibly. Cheap 64GB Sandisk Ultra's can handle the full 499.999 kHz that the Pi Pico ADC can output!
- Supercapacitor on RTC allows upwards of 15 minutes to exchange batteries without losing timing configuration 
- Recommend conformal coating Silicone, avoiding ports on BME280 + Winfrey, to increase resilience to environment (sensor needs exposure.)

## Planned Hardware Features
- Replace Winfrey with Infineon Ultrasonic MEMS when it gets released, which features ultrasonic speaker functionality and better performance/SNR/AOP/etc at higher f.
- External ADC rather than RP2040 ADC, giving 14/16-bit instead of 12-bit at 1 MHz. Aiming to integrate a MCP33151/41-XX, specifically, or something similarly specced. 1 Mhz -> 5x oversampling @ 192 kHz, 10x oversampling @ 96 kHz. Would improve noise performance, as ADC measurements carried out isolated from internals of RP2040 (SPI line exception.) This will be a very late WIP project- I haven't tested whether the performance of the onboard ADC is good enough yet to warrant upgrading from it. 

## Current work
- Modifying 3D waterproof case designs to accommodate current design, including adding port for weather sensor option.
- Cleaning up main body C code and optimization, alongside changing clock settings anticipating 1+ MHz ADC oversampling requirements.
- Planning for process on characterizing recording capabilities and specification w.r.t directionality, sensitivity, etc.
- Characterizing power usage. Currently uses about 30 mA at 192 kHz recording, 50-60 mA at 480 kHz recording.

![](https://github.com/callous4567/Batcorder/blob/main/VER_3_EX.jpg)
^*just the example I'm currently programming on, the third version of the PCB! The strange orange glob is a ball of kapton tape encasing a MAX8510, testing the disabling & desoldering/removal of the onboard DC-DC converter of the Pi Pico to improve noise performance (just disabling it isn't enough- you have to remove it completely for it to be extra effective. It works great, btw!*
![](https://github.com/callous4567/vespertilio/blob/main/current_pcbs.jpg)
^*The current spread of PCBS as of Version 4, currently WIP. The leftmost is the main body, the right two provide the optional weather sensor module (attached to the otherwise-waterproof, then weatherproof, case. You can note the addition of that ambient light sensor I mentioned, the externalization of the weather sensor, and various other things I previously promised! Though you'll have to wait for me to program everything- the PCB is a WIP, nevermind getting a working version here for me to work on.*
