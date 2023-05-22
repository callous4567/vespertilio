![](https://github.com/callous4567/Batcorder/blob/main/design_bat.png)

## General description 
A battery-powered bioacoustic recorder with a built-in ambient light sensor, and an optional weather-sensing addon attached by an onboard hijacked USB-C connector! Very much a WIP!

This is my first time with C (embedded or otherwise) and with a bunch of other things- PCBs, electronics, 3D CAD/printing/etc, so you will have to bare with me whilst I work through this project bit by bit 😅

## Where is it at the moment?
V6 design complete and functional code-wise/hardware-wise, with 75 mA power use @ 384 ksps with 12-bit internal RP2040 ADC. V9 design in progress, aiming for 50 mA power use @ 384 ksps with 16-bit dedicated ADC- hardware stage (firmware t.b.d.) 

## Features 
- BME280 for Environmental Sensing (Temp, Pressure, Humidity) as an optional attachment (attached to environmental case with 6-core custom USB-C cable!) 
- Knowles Winfrey Microphone with Differential Amplifier -> Inverting Amplifier Cascade for audio (giving single-ended-equivalent gains minimum 220+!)
- Vishay VEML6040A3OG to provide ambient light sensing of the environment
- 4-way PGA secondary gain on cascaded differential-inverting amplifier
- Compatible with usual MicroSD cards FAT32/exFAT/... @ up to 480 ksps sample rate 
- Supercapacitor on RTC allows upwards of 15 minutes to exchange batteries without losing timing configuration- no need to reconfigure after re-batterying... battery-re.. whatever!
- Competitive BOM cost: roughly £55 as of Ver 9 inclusive of weather sensor, which adds about £10 to the final cost.
- USB configurable recording schedules using desktop .exe and .json. **WIP**, allow for setting sampling frequency with each session, too, and make a GUI for configuration. **WIP**, also set the firmware over USB without having to toggle bootsel on the Pico. 

## Todo/current
- (!!!Extremely immediate!!!) Coding/Designing V9 PCB arrangement, which has 16-bit MCP33131 500ksps ADC instead, and analogue PGA switch arrangement for gain control. 
- (!!!Extremely immediate!!!) V9 PCB external regulator board design with dual MAX38640 buck regulators & 2nd-order filter design for digital power supply 
- (Delayed) Weatherproof case designs (for V6+ Audiomoth Scale, V5 done and worked fine.) 

![](https://github.com/callous4567/vespertilio/blob/main/current_pcbs.jpg)
^*The current (outdated*) spread of PCBS and concept. vespertilio is optionally attached, by a USB-C connector manhandled into an SPI connector, to a weather module PCB.
You don't need to use the weather module- doing so costs about £10-ish. The 3D-printed case plans will allow for optional use of it. You'll note that I added that little lightmeter- U1 :)*


