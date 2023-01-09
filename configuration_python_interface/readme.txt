This readme.txt will explain how to configure the vespertilio with the included JSON and executable file.
Please refer to https://github.com/callous4567/vespertilio for any updates, information on firmware updates, and so forth.

To start, set the power switch on vespertilio to OFF. 



You must first configure the vespertilio_config JSON file. 
The file will look something like this...



{
    "ADC_SAMPLE_RATE":192000,
    "RECORDING_MINUTES_PER_SUBRECORDING":5,
    "RECORDING_SESSION_LENGTH_MINUTES":120,
    "START_MINUTE":20,
    "START_HOUR":15,
    "USE_BME":true,
    "BME_PERIOD_SECONDS":3
}



ADC_SAMPLE_RATE: in Hz, sampling rate of the device.
RECORDING_SESSION_LENGTH_MINUTES: the length of the recording session, in minutes. 
RECORDING_MINUTES_PER_SUBRECORDING: minutes per recording. We must split recording sessions to minimize number of corrupted files.
START_MINUTE: 0:59, which minute to start the recording on. By default, we start at second=0.
START_HOUR: 0:23, which hour to start the recording on.
USE_BME: true-false on whether to use environmental sensing 
BME_PERIOD_SECONDS: period, in seconds, for environmental sensing 



Following configuration of the JSON file, you should save it and then set the power switch on vespertilio to ON.



You should then run the executable included within a minute of doing this. This executable will...



	- Parse JSON file 
	- Retrieve system time 
	- Handshake with vespertilio, send configuration to it, and tell Pico to save it to onboard flash
	- Transfer the system time to the Pico for it to configure the PCB RTC
	- Initiate a wait sequence.
	


Once complete, and if successful, the vespertilio will spasm its onboard LED in the colour GREEN, signifying success.



At this point, you should disconnect the USB cable and turn off the vespertilio- the order of this step doesn't matter. All success!



If you encounter any red LED flashes during this process, an error has occurred- double check your JSON with an online parser.
If the online parser says it's fine, and if you have ensured that the variables all match the same type as the template above...
Well, in that case you just have to ask me and I will check it- your device might be dogged (unlikely) or the code is busted (likely.)
Oh, or your USB cable is piss. 