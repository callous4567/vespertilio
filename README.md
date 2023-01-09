![](https://github.com/callous4567/Batcorder/blob/main/design_bat.png)

Aye- Latin for bat! Or bats- I have no clue. Witness my first attempts with Microcontrollers, Embedded Programming, C, circuits, and more! I've never
done electronics, but here she goes! 

This is very much a WIP. There are a bunch of folders... Case has some 3D design stuff for the battery enclosure, 
configuration_python_interface a way to configure over USB serial- again WIP, Datasheets/etc obvious... Design of Silk has the silkscreen designs,
KiCad Schemas has circuit designs, SRAM_SD_Testing is current C codebase. There's also the current Gerber file included.

I've not included the planned software features because I'm very much in the middle of hashing it out at the moment. The initial goal is just a device
that can be configured by setting up the .json, running a .exe, transferring over a microUSB, then dumping in the field for a recording day. Obviously
after that point, and verifying that the hardware is gravy, then I can add more software features- for now that is my goal though. 

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
- External ADC rather than RP2040 ADC, giving 16-bit instead of 12-bit ADC performance
- Transfer BME280 to secondary PCB with wire connector through waterproof case (at moment, need case with hole on bottom) to maximize safety
- ??? 



