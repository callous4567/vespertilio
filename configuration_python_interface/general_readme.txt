===========================
=== GENERAL README FILE === 
===========================

This is the GENERAL README file- some could call it an "operational guide." 
Obviously it's a work-in-progress- I plan to put this into a proper LaTeX document later.
For now though, it is just a file which I have written to give to my lass to guide her (hi!) in using the vespertilio.



=================================
=== ENCLOSURE AND FIELD SETUP ===
=================================
For now, I recommend a plastic ziploc bag, with some vents cut into the bottom (you can cut a large hole and then stitch it to prevent 
the unit falling out, put some fabric tape on it- just something to allow environmental air into the enclosure. 

This will allow reliable pressure and temperature measurements to be taken. Relative Humidity (RH) will be difficult without direct exposure
of the environmental sensor to the environmental air with some airflow- you will need the purpose-built case (WIP) for this. 

Your unit will have a weather sensor marked "ENV" on the PCB, on the top right (opposite the MicroSD slot.) 
This is the environmental sensor, and should be positioned as close to the vent on the enclosure as possible (will discuss this soon.)

It will also have a microphone, marked "MIC" on the PCB, toward the lower left.
This should be aimed roughly at the area you want to detect bats. I have not yet characterized directional response, so aye. 

Other than these notes, you can use the unit just like the Audiomoth, Pipistrelle, and other such units. Over time I will characterize how
my 3D cases and whatnot respond to the environment- until then, keep the above in mind before assuming that the directional response is as 
good as the Audiomoth- it may not be.



==================================
=== TURNING IT ON AND USING IT ===
==================================
On startup, the vespertilio tries to find a configuration host from the MicroUSB port. This fails after 30 seconds if none is found, after
which it will then hold a steady green for 10 seconds indicating that it is going into "recording mode." 

It will then load the previous configuration from the onboard flash, and cycle through the alarms it has been assigned. By default, it will 
stop at the last alarm and enter sleep. 

The external RTC is used to pace recordings and handle alarm interrupts. It's accurate to within a second over a 30+ day period (typical)
and has an external supercapacitor to keep backup whilst you replace batteries. If you do not need to reconfigure it, and want to use the 
same alarm schedule, it is safe to just replace the batteries and keep using it without reconfiguring for as long as necessary. 

Note however, the alarm does not take account daylight savings time changes. You should watch out for that (I will include a fix for this 
later with the configuration utility, but for now, just be careful.)



