![](https://github.com/callous4567/Batcorder/blob/main/design_bat.png)

## General description 
A battery-powered (for now!) bioacoustic recorder with a built-in ambient light sensor, and an optional weather-sensing addon attached by an onboard hijacked USB-C connector! Very much a WIP!

This is my first time with C (embedded or otherwise) and with a bunch of other things- PCBs, electronics, 3D CAD/printing/etc, so you will have to bare with me whilst I work through this project bit by bit 😅

## Features 
- BME280 for Environmental Sensing (Temp, Pressure, Humidity) as an optional attachment (attached to environmental case with 6-core custom USB-C cable!) 
- Knowles Winfrey Microphone with Differential Amplifier -> Inverting Amplifier Cascade for audio (giving single-ended-equivalent gains minimum 220+!)
- Vishay VEML6040A3OG to provide ambient light sensing of the environment
- Variable gain on the inverting amplifier w/ fixed gain on the differential for better noise performance 
- Low-power-standby of ~1mA dormancy when awaiting recording sessions (digital assembly and analogue disabled during standby.)
- Compatible with usual MicroSD cards- Sandisk has been tested (16GB, 32GB, 64GB Ultra) to work fine at 480 kHz.
- Supercapacitor on RTC allows upwards of 15 minutes to exchange batteries without losing timing configuration- no need to reconfigure after re-batterying... battery-re.. whatever!
- Competitive BOM cost: roughly £40 as of Ver 4 without the weather sensor addon, which adds about £10 to the final cost.
- USB configurable recording schedules using desktop .exe and .json. **WIP**, allow for setting sampling frequency with each session, too, and make a GUI for configuration. **WIP**, also set the firmware over USB without having to toggle bootsel on the Pico. 
- **WIP**, provided case designs can be 3D printed appropriately to provide waterproof (weatherproof with weather sensor- it compromises the waterproof seal) sealed case for the device, allowing use in all usual environments 
- **WIP**, power usage is roughly 30 mA at 192 kHz and 60 mA at 480 kHz. Aiming to reduce (by modifying system and SPI clock against ADC clock requirement) power usage.


## Todo/current
- Clean up schematics, BOM, etc, for V4. PCBs
- Designing 3D weatherproof casings for vespertilio, BME attachment, and BME USB-SPI interface. *The concept is done- just need time to modify current designs.*
- Cleaning up main body C code and optimization, alongside changing clock settings. *Need time.*
- Planning for process on characterizing recording capabilities and specification w.r.t directionality, sensitivity, etc. *Need bats, an ultrasonic audio source, and somewhere quiet.*



![](https://github.com/callous4567/Batcorder/blob/main/VER_3_EX.jpg)
^*just the example I'm currently programming on, the third version of the PCB! The strange orange glob is a ball of kapton tape encasing a MAX8510, testing the disabling & desoldering/removal of the onboard DC-DC converter of the Pi Pico to improve noise performance (just disabling it isn't enough- you have to remove it completely for it to be extra effective. It works great, btw!*



![](https://github.com/callous4567/vespertilio/blob/main/current_pcbs.png)
^*The current spread of PCBS and concept. vespertilio is optionally attached, by a USB-C connector manhandled into an SPI connector, to a weather module PCB.
You don't need to use the weather module- doing so costs about £10-ish. The 3D-printed case plans will allow for optional use of it. You'll note that I added that little lightmeter- U1 :)*


