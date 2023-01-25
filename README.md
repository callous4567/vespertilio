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
- External ADC rather than RP2040 ADC, giving 14/16-bit instead of 12-bit at 1 MHz. Aiming to integrate a MCP33151/41-XX, specifically, or something similarly specced. 1 Mhz -> 5x oversampling @ 192 kHz, 10x oversampling @ 96 kHz. Would improve noise performance, as ADC measurements carried out isolated from internals of RP2040 (SPI line exception.)
- Transfer BME280 to secondary PCB with wire connector through waterproof case (at moment, need case with hole on bottom) to maximize damage safety, and replace with the BME688 to allow for air quality to also be obtained- the AI module may prove useful. 
- Reduce PCB size to be comparable to other similar devices (this is my first time on the block- cut me some slack.)
- Replace Winfrey with Infineon Ultrasonic MEMS when it gets released, which features ultrasonic speaker functionality and better performance/SNR/AOP/etc at higher f.
- Addition of an ambient light sensor to give not 3-parameter, but 4-parameter environmental sensing.

## Current work
- Establishing design plan for moving BME280/BME688 to external module (see Case & Environmental Concept) to make main environmental case waterproof.
- Designing external 3D case and modifying pre-existing internal design to allow for interconnection.
- Designing external BME280/BME688 PCB and internal USB-C connector PCB.
- Cleaning up main body C code and optimization, alongside changing clock settings anticipating 1+ MHz ADC oversampling requirements.
- Planning for process on characterizing recording capabilities and specification w.r.t directionality, sensitivity, etc.
- Awaiting delivery of Version 3 PCB, which will be used for in-field testing and establishing if my LTSpice simulations were reasonable.
- Characterizing power usage. Currently uses about 30 mA at 192 kHz recording, 50-60 mA at 480 kHz recording.

![](https://github.com/callous4567/Batcorder/blob/main/VER_1_EX.jpg)
^*just the example I'm currently programming on, the first version of the PCB! :D* 

