This readme.txt will explain how to configure the vespertilio with the included JSON and executable file.
Please refer to https://github.com/callous4567/vespertilio for any updates, information on firmware updates, and so forth.


========================
===JSON CONFIGURATION===
========================

You must first configure the vespertilio_config JSON file. 
The file will look something like this...



{
    "ADC_SAMPLE_RATE":480000,
    "RECORDING_MINUTES_PER_SUBRECORDING":5,
    "USE_BME":true,
    "BME_PERIOD_SECONDS":10,
    "NUMBER_OF_SESSIONS": 4,
    "ALARM_HOUR_1": 16,
    "ALARM_MINUTE_1": 30,
    "RECORDING_SESSION_MINUTES_1": 30,
    "ALARM_HOUR_2": 17,
    "ALARM_MINUTE_2": 30,
    "RECORDING_SESSION_MINUTES_2": 30,
    ,
    ,
    ,
    "ALARM_HOUR_N": 18,
    "ALARM_MINUTE_N": 30,
    "SESSION_LENGTH_MINUTES_N": 30,
}



The variables are self-explanatory for the first few- these are constant over all recording sessions that you encode to have run...



    ADC_SAMPLE_RATE: in Hz, sampling rate of the device. There is a limit of 499,999 (qualified on a Sandisk Ultra 64 GB to work at 480 kHz)
    RECORDING_MINUTES_PER_SUBRECORDING: minutes per recording. We must split recording sessions to minimize number of corrupted files.
    USE_BME: true-false on whether to use environmental sensing. Note that this may add file delay.
    BME_PERIOD_SECONDS: period, in seconds, for environmental sensing. Higher = better for file delay.



The next set of variables define your recording session schedule.
The number of sessions is constrained as we have limited the transfer to the Pico to 64 int32's for ease of code.
Consequently, just use a sensible number (who needs more than 10, anyway? Hint hint.)
It goes without saying, but do not overlap any two sessions- alarm sequencing will fail.



    NUMBER_OF_SESSIONS: the number of recording sessions to have- either over the same day, or successive days.
    ALARM_HOUR_N: the hour (0->23) at which to start the alarm.
    ALARM_MINUTE_N: the minute (0->59) at which to start the alarm.
    SESSION_LENGTH_MINUTES_N: the session length, in minutes, for this recording session.



Note the "either over the same day, or successive days" bit in how you can define alarms. If you define two identical alarms in sequence, i.e.

	ALARM_HOUR_1: 7,
	ALARM_MINUTE_1: 30,
	...,
	ALARM_HOUR_2: 7,
	ALARM_MINUTE_2: 30,
	...,
	
the second alarm will trigger after the first, i.e. the next day. The limit for this is spacing an alarm by 7 days or more- your next alarm needs 
to be set to within 7 days of the first, as the RTC has been set to trigger alarms on a 7-day system.



Following configuration of the JSON file, you should save it.


=====================
=== CONFIGURATION ===
=====================



The vespertilio will automatically enter "configuration mode" when a USB is inserted/present within 30 seconds of startup.
Before configuration, ensure that batteries are present for the RTC- the RTC can only survive 10-15 minutes without them.
If the RTC resets, your configuration resets- do not remove these batteries until after data gathering is done.



To start, plug in the USB and turn on battery power to the circuit using the ON-OFF microswitch.
This is not strictly required to configure the flash, but the RTC requires pullups, fed by the digital assembly.
The digital assembly is battery-powered and only active when ON-OFF microswitch is set to ON.



You should then run the executable included, immediately.
This executable will...



	- Parse JSON file 
	- Retrieve system time 
	- Handshake with vespertilio, send configuration to it, and tell Pico to save it to onboard flash
	- Transfer the system time to the Pico for it to configure the PCB RTC
	- Initiate a wait sequence, after which you are safe to remove the USB.
	


Once complete, and if successful, the vespertilio will spasm its onboard LED in the colour GREEN, signifying success.
Feel free to disconnect the USB once the flashes pop up, then turn off the vespertilio.


==============
=== ERRORS ===
==============



If the final green flashes do not occur, an error has occurred on the Pico.
If you see an absolute wallop of a spasm of flashes (we're talking more than a few per second) something has gone woefully wrong.

We have not included any error catchment: if an error occurs, all you can do is restart this process and try again.

If you have a PicoProbe then any "hard errors" will be easily debuggable using VSCode + Cortex M0 debugger.

Other than that, you won't be able to do much else.

My todo includes making the .exe give better terminal output + track all steps better, to allow easier error viewing/etc...
Until then though, well, sorry.