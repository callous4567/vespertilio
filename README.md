![](https://github.com/callous4567/Batcorder/blob/main/design_bat.png)

## General description 
A mid-spec battery-powered bat-recorder with an environmental sensor on the board, featuring the BME280 and Knowles Winfrey paired with a Pi Pico, with low-loss parts! Hopefully it'll be a great bat recording device! Hopefully!

This is my: first time with C, first time doing anything electrical/circuits/etc, and one of my first times modelling anything in 3D. Also my first time with PCBs. Thusly, do not expect anything magical- this is all a work-in-progress to make something that performs exceptionally well for bats with exceptional modifiability for the average hobbyist (see: me.) ***This page is extremely WIP, by the way. Most code here will not work- note how there are no "releases"!*** The only working build at the moment is local to me since I am just testing features in situ before slapping 'em all together.

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
- Replace Winfrey with Infineon Ultrasonic MEMS when it gets released, which features ultrasonic audio functionality and better performance/SNR/AOP/etc at higher f.
- Addition of an ambient light sensor to give not 3-parameter, but 4-parameter environmental sensing. 

![](https://github.com/callous4567/Batcorder/blob/main/VER_1_EX.jpg)
^*just the example I'm currently programming on, the first version of the PCB! :D* 