![](https://github.com/callous4567/Batcorder/blob/main/design_bat.png)

This is my: first time with C, first time doing anything electrical/circuits/etc, and one of my first times modelling anything in 3D. Also my first time with PCBs. Thusly, do not expect anything magical- this is all a work-in-progress to make something that performs exceptionally well for bats with exceptional modifiability for the average hobbyist (see: me.)

Every last scrap of code is here, every last design/etc is here, and most importantly, you can program the damn thing with the Pi Pico SDK, arguably the best SDK an MCU has in terms of documentation (and great for the first time C'er, i.e. me.) Plus, cheap. If only the RP2040 had a better ADC, but I'll rectify that later.

NOTE! IMPORTANT NOTE! THE CURRENT GERBER HAS NOT BEEN PRINTED/TESTED. I AM CURRENTLY PROGRAMMING ON THE PREVIOUS VERSION OF THE PCB, 
WHICH INSTEAD OF HAVING A DIFFERENTIAL-INVERTING, HAS AN INVERTING-INVERTING CASCADE! Other than that, it's the same as this version of the Gerber. 

## Features/Hardware 
- BME280 for Environmental Sensing (Temp, Pressure, Humidity)
- Knowles Winfrey Microphone with Differential Amplifier -> Inverting Amplifier Cascade for audio
- Variable gain on the inverting amplifier w/ fixed gain on the differential for better noise performance 
- Low-power-standby of ~1mA dormancy when awaiting recording sessions
- Various passive filters in circuitry to minimize noise, paired with first-order high-pass filter on ADC input
- Compatible with any/all MicroSD cards (must be Class 10 for 384 kHz performance, though.)
- Supercapacitor on RTC allows upwards of 15 minutes to exchange batteries without losing timing configuration 
- Recommend conformal coating Silicone, avoiding ports on BME280 + Winfrey, to increase resilience to environment (sensor needs exposure.)

## Planned Hardware Features
- External ADC rather than RP2040 ADC, giving 14/16-bit instead of 12-bit. Aiming to integrate a MCP33151/41-XX, specifically, or something similarly specced.
- Transfer BME280 to secondary PCB with wire connector through waterproof case (at moment, need case with hole on bottom) to maximize damage safety, and replace with the BME688 to allow for air quality to also be obtained- the AI module may prove useful. 
- Reduce PCB size to be comparable to other similar devices (this is my first time on the block- cut me some slack.)
- Replace Winfrey with Infineon Ultrasonic MEMS when it gets released, which features ultrasonic audio functionality and better performance/SNR/AOP/etc at higher f.



