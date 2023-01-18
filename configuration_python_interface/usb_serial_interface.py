import struct
import time
import json
from serial.tools import list_ports
from serial import Serial

def find_configuration():
    # Write our handshake!
    print("Searching for device...")

    # The standard VID/PID for the Pi Pico SDK CDC UART.
    DEVICE_VID = 0x2E8A
    DEVICE_PID = 0x000A
    found = False
    porter = None

    # identify the port for our PID/VID
    for p in list_ports.comports():
        if p.vid == DEVICE_VID and p.pid == DEVICE_PID:
            found = True,
            porter = p

    # if found is false, we are fucked
    if not found:
        print("Found is false and We are Fucked.")
        raise RuntimeError("We could not find the hardware device on the COM ports.")
    else:
        pass

    # open serial port
    print("Found device in port ", porter.name, " and now attempting configuration handshake.")
    return porter


port = find_configuration()
print("Opened serial port")
sert = Serial(port=port.name, baudrate=115200, timeout=3) # timeout is in seconds


def flushbuf():
    sert.flushInput()
    sert.flushOutput()


def do_handshake():

    u = sert.readline().decode('UTF-8').strip()

    if u == "Configure vespertilio?":
        print("vespertilio has requested configuration. Sending configuration accept.")
    else:
        raise ValueError("vespertilio did not ask to be configured. ValueError.")

    # pass the slave a true
    sert.write("true".encode("UTF-8"))

    print("Sent our acceptance... now we wait for confirmation...")

    # Wait for it to accept the true, then check for a thanks!
    time.sleep(10e-3)
    u = sert.readline().decode('UTF-8').strip()
    print(u)
    if u == "Thanks.":
        print("vespertilio gives us a", u)
    else:
        raise ValueError("No thanks given... we've failed in confirming our affections.")



do_handshake()
print("Handshake completed! Going on to prepare data...")
time.sleep(5e-3) # sleep 2 milliseconds to allow flushbuf in request_configuration on Pico


def prepare_dictionary():

    """
    .
    """

    """
    includes variables (now in a Python dict as a result...)
    "ADC_SAMPLE_RATE":192000,
    "RECORDING_MINUTES_PER_SUBRECORDING":5
    "RECORDING_SESSION_LENGTH_MINUTES":120,
    "START_MINUTE":20,
    "START_HOUR":15,
    "USE_BME":true,
    "BME_PERIOD_SECONDS":3,
    """

    # Load the JSON file
    with open("vespertilio_config.json", "rb") as f:
        json_config = json.load(f)

    # Make a blank new dict to insert into (dicts are insertion ordered.)
    ordered_dictionary = dict()

    # Insert the recording parameters + BME parameters
    ordered_dictionary['ADC_SAMPLE_RATE'] = json_config['ADC_SAMPLE_RATE']
    ordered_dictionary['RECORDING_LENGTH_SECONDS'] = json_config['RECORDING_MINUTES_PER_SUBRECORDING']*60
    ordered_dictionary['RECORDING_SESSION_MINUTES'] = json_config['RECORDING_SESSION_LENGTH_MINUTES']
    ordered_dictionary['USE_BME'] = json_config['USE_BME']
    ordered_dictionary['BME_RECORD_PERIOD_SECONDS'] = json_config['BME_PERIOD_SECONDS']

    """
    tm_year: 2023, ex. REMOVE THE 20, ONLY NEED LAST 2 DIGITS
    tm_mon [1,12] SAME AS RTC 
    tm_mday [1,31] SAME AS RTC 
    tm_hour [0,23] SAME AS RTC 
    tm_min [0,59] SAME AS RTC 
    tm_sec [0,60] SAME AS RTC 
    tm_wday [0,6] monday=0 ADD 1, RTC IS [1,7] 
    access via time.localtime().variable 
    """
    # Get current time + add to our config
    current_time = time.localtime()
    ordered_dictionary['SECOND'] = current_time.tm_sec
    ordered_dictionary['MINUTE'] = current_time.tm_min
    ordered_dictionary['HOUR'] = current_time.tm_hour

    """
    https://docs.python.org/3/library/time.html
    The RTC is [1,7]
    The localtime() is [0,6]
    Hence add 1 to make it [1,7]
    Monday = 0 -> Monday = 1 (see link, Monday = 0.)
    Now, on the RTC we have [1,7] 
    We need the RTC to register 1 as Sunday
    Consequently, We need Monday to be equal to 2 in ordered_dictionary
    So, 0 -> 2 is our necessary translation
    Add 2 to ordered_dictionary['DOTW']
    The final value (sunday in localtime) will roll over to 6 + 2 = 8, which is now set to unity- the first dotw.)
    """
    ordered_dictionary['DOTW'] = current_time.tm_wday + 2
    if ordered_dictionary['DOTW'] == 8:
        ordered_dictionary['DOTW'] = 1

    ordered_dictionary['DOTM'] = current_time.tm_mday
    ordered_dictionary['MONTH'] = current_time.tm_mon
    ordered_dictionary['YEAR'] = int(str(current_time.tm_year)[2:4])

    # Finally, add in our alarm times, too
    ordered_dictionary['ALARM_MIN'] = json_config['START_MINUTE']
    ordered_dictionary['ALARM_HOUR'] = json_config['START_HOUR']

    return ordered_dictionary


dictionary_config = prepare_dictionary()
print(dictionary_config)

def pack_dictionary():

    """

    Pack the dictionary_config into a bytestring of int32's (int under struct_alignment on the python3 documentation.)
    This can then be sent to the vespertilio.

    """

    """
    COPY PASTED FROM THE VESPERTILIO CODEBASE: VESPERTILIO_USB_INT.C UNDER DRIVERS/PICO_USB_CONFIGURE

    /*

    // These are all the variables we need to retrieve from the host, in order of interpretation. 

    // INDEPENDENT VARIABLES (retrieve from host)
    0) int32_t ADC_SAMPLE_RATE = 192000;
    1) int32_t RECORDING_LENGTH_SECONDS = 30; // note that BME files are matched to this recording length, too. 
    2) int32_t RECORDING_SESSION_MINUTES = 1; 
    3) int32_t USE_BME = true;
    4) int32_t BME_RECORD_PERIOD_SECONDS = 2;

    // INDEPENDENT TIME VARIABLES (retrieve from host) USED BY RTC 
    5) int32_t SECOND;
    6) int32_t MINUTE;
    7) int32_t HOUR;
    8) int32_t DOTW; // 1=SUNDAY (Mon = 2, Tues = 3, etc)
    9) int32_t DOTM;
    10) int32_t MONTH;
    11) int32_t YEAR;
    12) int32_t ALARM_MIN; // alarm second/day-date are irrelevant, we set alarm to go off every day/same time
    13) int32_t ALARM_HOUR;

    // INDEPENDENT CONFIGURATION BOOL/INT32_T (which is set by the pico- we do not send this from the host!!!)
    14) int32_t CONFIG_SUCCESS // 1 if successful, 0 if else.

    */
    """

    stringstruct = b''

    for entry in list(dictionary_config.keys()):
        stringstruct += struct.pack('<i', dictionary_config[entry])
        # see https://docs.python.org/3/library/struct.html#struct-alignment
        # we are packing with the < (little endianness) and i (int 4-byte int32) format

    print("Len of the data string in bytes is ... ", len(stringstruct.decode('UTF-8')))

    return stringstruct


dictionary_stringstruct = pack_dictionary()
print("Data prepared...")

def send_configuration():

    """
    Send configuration and check that we have successfully sent it by reading the return "SEIKO!"
    """

    print("Checking to see if Pico is ready to accept...")

    u = sert.readline().decode('UTF-8').strip()

    if u == "Ready to accept...":
        pass
    else:
        print(u)
        raise ValueError("Device not ready for configuration... error raised.")

    time.sleep(50e-3)

    sert.write(dictionary_stringstruct)

    print("Verifying that data sent was correct by reading return...")
    return_stringstruct = sert.read(80).replace(b'\r', b'')  # 100 bytes max! Confirm this with the firmware.
    # We set this to 100 bytes because the pico sends nasty ass \r's.

    if dictionary_stringstruct == return_stringstruct:
        print("Transaction successful: vespertilio has our data intact!")
    else:
        print(dictionary_stringstruct)
        print(return_stringstruct)
        unpacked_iter_dict = struct.iter_unpack('<i', dictionary_stringstruct)
        unpacked_iter_retr = struct.iter_unpack('<i', return_stringstruct)
        for i in range(0,14):
            print(next(unpacked_iter_dict), next(unpacked_iter_retr))
        raise RuntimeError("We did not successfully transfer over to vespertilio.")

    time.sleep(5e-3)
    flushbuf()
    time.sleep(5e-3)

    sert.write("Completed.".encode("UTF-8"))  # tell vespertilio that we are done- all is good.

    time.sleep(100e-3)
    flash_written_or_not = sert.readline().decode('UTF-8').strip()
    print(flash_written_or_not)
    if flash_written_or_not == "Flash written 1.":
        print("Slave has written data to flash and configured RTC. All done!")
    else:
        raise RuntimeError("Slave has failed to write data to flash and configure RTC.")


send_configuration()

print("Configuration has been successful from our side.")
print("Check vespertilio to confirm LED flashes.")
print("You can now disconnect USB.")
for i in range(11):
    print(("Closing in {0}s ...").format(10-i))
    time.sleep(1)
