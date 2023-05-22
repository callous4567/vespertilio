![](https://github.com/callous4567/Batcorder/blob/main/design_bat.png)

## General description 
A battery-powered (for now!) bioacoustic recorder with a built-in ambient light sensor, and an optional weather-sensing addon attached by an onboard hijacked USB-C connector! Very much a WIP!

This is my first time with C (embedded or otherwise) and with a bunch of other things- PCBs, electronics, 3D CAD/printing/etc, so you will have to bare with me whilst I work through this project bit by bit 😅

Note that at the moment, Rev. 6 of the project is fully-functional. Rev. 9 is the current WIP (codeskip 7,8) and incorporates a huge boatload of changes to hardware and software- this is t.b.d.

## Features 
- BME280 for Environmental Sensing (Temp, Pressure, Humidity) as an optional attachment (attached to environmental case with 6-core custom USB-C cable!) 
- Knowles Winfrey Microphone with Differential Amplifier -> Inverting Amplifier Cascade for audio (giving single-ended-equivalent gains minimum 220+!)
- Vishay VEML6040A3OG to provide ambient light sensing of the environment
- Variable gain on the inverting amplifier w/ fixed gain on the differential for better noise performance 
- Low-power-standby of ~2mA dormancy when awaiting recording sessions (digital assembly and analogue disabled during standby.)
- Compatible with usual MicroSD cards- Sandisk has been tested (16GB, 32GB, 64GB Ultra) to work fine at 480 kHz.
- Supercapacitor on RTC allows upwards of 15 minutes to exchange batteries without losing timing configuration- no need to reconfigure after re-batterying... battery-re.. whatever!
- Competitive BOM cost: roughly £50 as of Ver 6 inclusive of weather sensor, which adds about £10 to the final cost.
- USB configurable recording schedules using desktop .exe and .json. **WIP**, allow for setting sampling frequency with each session, too, and make a GUI for configuration. **WIP**, also set the firmware over USB without having to toggle bootsel on the Pico. 
- Same form factor, microphone positioning, as Audiomoth, giving compatibility with Audiomoth cases.

## Todo/current
- (Immediate) Weatherproof case designs (for V6+ Audiomoth Scale, V5 done and worked fine.) 
- (Immediate) Clean up schematics, BOM, etc, for V7+. PCBs 
- (Immediate) Cleaning up main body C code and optimization, alongside changing clock settings (distant.)
- (Distant) Planning for process on characterizing recording capabilities and specification w.r.t directionality, sensitivity, etc. *Need bats, an ultrasonic audio source, and somewhere quiet.*
- (Immediate) Testing whether digipot MCP4131 arrangement for variable gain introduces too much noise compared to classical PGA arrangement, being tested on PCB V7. 

![](https://github.com/callous4567/vespertilio/blob/main/current_pcbs.jpg)
^*The current spread of PCBS and concept. vespertilio is optionally attached, by a USB-C connector manhandled into an SPI connector, to a weather module PCB.
You don't need to use the weather module- doing so costs about £10-ish. The 3D-printed case plans will allow for optional use of it. You'll note that I added that little lightmeter- U1 :)*


