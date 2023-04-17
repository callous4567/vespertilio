import struct
import time
import json
from serial.tools import list_ports
from serial import Serial


"""

XIP_BASE is 0x10000000 and extends from here. XIP_BASE is the zeroth byte of the flash.
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024) (pico.h)
#define FLASH_PAGE_SIZE (1u << 8) // program in page sizes (flash.h) ... 256 bytes, i.e. 64 int32_t values. 
#define FLASH_SECTOR_SIZE (1u << 12) // erase sector sizes (flash.h)

So, a valid flash byte can be found at XIP_BASE all the way to (XIP_BASE + PICO_FLASH_SIZE_BYTES.) 
No -1... idk why, see https://github.com/raspberrypi/pico-examples/blob/master/flash/program/flash_program.c

So, our process is...
- Erase the sector located at XIP_BASE + (PICO_FLASH_SIZE_BYTES) - FLASH_SECTOR_SIZE
- Write the page starting at XIP_BASE + (PICO_FLASH_SIZE_BYTES) - FLASH_SECTOR_SIZE

Which we can re-write to
- Erase the sector located at XIP_BASE + CONFIG_FLASH_OFFSET
- Write the page starting at XIP_BASE + CONFIG_FLASH_OFFSET

*/

/*

// // // // These are all the variables we need to retrieve from the host, in order of interpretation, zero-based indices.

CONFIGURATION_BUFFER_INDEPENDENT_VALUES     pertain to values that are static over the session... 
CONFIGURATION_RTC_INDEPENDENT_VALUES        pertain to values that are dynamic over the session (barring the first 7 values, which defined the initial state of the RTC at configuration.)

CONFIGURATION_BUFFER_INDEPENDENT_VALUES     = 4                                                                                                              // subject to constant change based on features                
CONFIGURATION_RTC_INDEPENDENT_VALUES        = 7 + 1 + 3*NUMBER_OF_ALARMS                                                                                     // where 7 is the RTC config and 1 is NUMBER_OF_ALARMS 
CONFIGURATION_BUFFER_TOTAL_SIZE_BYTES       = FOUR BYTES (INT32_T!!!) * [CONFIGURATION_BUFFER_INDEPENDENT_VALUES + CONFIGURATION_RTC_INDEPENDENT_VALUES + 1] // where 1 has been added to account for CONFIG_SUCCESS
CONFIGURATION_BUFFER_MAX_VARIABLES          = 64                                                                                                             // FLASH_PAGE_SIZE/sizeof(int32_t)

The host should pre-package a total FLASH_PAGE_SIZE=256 bytes=64 int32_t's for us. 
The forward values should be arranged as below. 
The end value, CONFIG_SUCCESS, should be set to unity.

// INDEPENDENT VARIABLES FOR RECORDING SPECIFICS: CONFIGURATION_BUFFER_INDEPENDENT_VALUES of them: set recording parameters.
0)                                                                              int32_t ADC_SAMPLE_RATE = 192000;               
1)                                                                              int32_t RECORDING_LENGTH_SECONDS = 30;           
2)                                                                              int32_t USE_ENV = true;                         
3 = CONFIGURATION_BUFFER_INDEPENDENT_VALUES - 1)                                int32_t ENV_RECORD_PERIOD_SECONDS = 2;           

// INDEPENDENT TIME VARIABLES- 7 of them. USED BY RTC starting from zero-based index CONFIGURATION_BUFFER_INDEPENDENT_VALUES total of CONFIGURATION_RTC_INDEPENDENT_VALUES
CONFIGURATION_BUFFER_INDEPENDENT_VALUES)                                        int32_t SECOND;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+1)                                      int32_t MINUTE;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+2)                                      int32_t HOUR;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+3)                                      int32_t DOTW; // // 1=SUNDAY (Mon = 2, Tues = 3, etc) Pi Pico SDK has 0 to Sunday, so DOTW-1 = Pi pico DOTW. See Python conversions which set Monday to 2.
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+4)                                      int32_t DOTM;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+5)                                      int32_t MONTH;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+6)                                      int32_t YEAR;

// INDEPENDENT TIME VARIABLES CONTINUED: CUSTOM TIME SCHEDULING FOR THE ALARM! There will be (3N+1) variables here, where the 1 is the "NUMBER_OF_ALARMS." Note that the RECORDING_SESSION_MINUTES must be defined for each alarm. 
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+7)                                      int32_t NUMBER_OF_SESSIONS;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+8)                                      int32_t ALARM_HOUR_1;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+9)                                      int32_t ALARM_MIN_1;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+10)                                     int32_t RECORDING_SESSION_MINUTES_1;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+11)                                     int32_t ALARM_HOUR_2;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+12)                                     int32_t ALARM_MIN_2;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES+13)                                     int32_t RECORDING_SESSION_MINUTES_2;
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t ...
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*WHICH_ALARM_ONEBASED - 2        int32_t ALARM_HOUR_WHICH
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*WHICH_ALARM_ONEBASED - 1        int32_t ALARM_MIN_WHICH
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*WHICH_ALARM_ONEBASED            int32_t RECORDING_SESSION_MINUTES_WHICH
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t ...
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*NUMBER_OF_ALARMS - 2)           int32_t ALARM_HOUR_NUMBER_OF_ALARMS;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*NUMBER_OF_ALARMS - 1)           int32_t ALARM_MIN_NUMBER_OF_ALARMS;
CONFIGURATION_BUFFER_INDEPENDENT_VALUES + 7 + 3*NUMBER_OF_ALARMS)               int32_t RECORDING_SESSION_MINUTES_NUMBER_OF_ALARMS;

// Population of zeros
:                                                                               int32_t host sets to zero
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t ...
:                                                                               int32_t host sets to zero

// INDEPENDENT CONFIGURATION BOOL/INT32_T
CONFIGURATION_BUFFER_MAX_VARIABLES - 1)                                         int32_t CONFIG_SUCCESS // FROM THE HOST! 1 if successful, 0 if else. The end of the flash page/buffer.


"""

# create 64-value list object- our FLASH_PAGE_SIZE buffer: 64*int32_t=256.


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
sert = Serial(port=port.name, baudrate=230400, timeout=2)  # timeout is in seconds
print("Found our serial port and opened it!")

def flushbuf():
    sert.flushInput()
    sert.flushOutput()


def do_handshake():

    u = sert.readline().decode('UTF-8').strip()

    if u == "Configure vespertilio?":
        print("vespertilio has requested configuration. Sending configuration accept.")
    else:
        print("DEBUG ", u)
        raise ValueError("vespertilio did not ask to be configured. ValueError.")

    # pass the slave a true
    sert.write("true".encode("UTF-8"))

    print("Sent our acceptance... now we wait for confirmation...")

    # Wait for it to accept the true, then check for a thanks!
    u = sert.readline().decode('UTF-8').strip()
    print(u)
    if u == "Thanks.":
        print("vespertilio gives us a", u)
    else:
        print(u)
        raise ValueError("No thanks given... we've failed in confirming our affections.")



do_handshake()
print("Handshake completed! Our pico thirsts for the data! Going on to prepare data...")
time.sleep(5e-3) # sleep 2 milliseconds to allow flushbuf in request_configuration on Pico


def prepare_dictionary():

    # Load the JSON file
    with open("vespertilio_config.json", "rb") as f:
        json_config = json.load(f)

    # Make a blank new dict to insert into (dicts are insertion ordered.)
    ordered_dictionary = dict()

    # INDEPENDENT VARIABLES FOR RECORDING SPECIFICS: CONFIGURATION_BUFFER_INDEPENDENT_VALUES of them: set recording parameters.
    ordered_dictionary['ADC_SAMPLE_RATE'] = json_config['ADC_SAMPLE_RATE']
    ordered_dictionary['RECORDING_LENGTH_SECONDS'] = json_config['RECORDING_MINUTES_PER_SUBRECORDING']*60
    ordered_dictionary['USE_ENV'] = json_config['USE_ENV']
    ordered_dictionary['ENV_RECORD_PERIOD_SECONDS'] = json_config['ENV_PERIOD_SECONDS']

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
    # INDEPENDENT TIME VARIABLES- 7 of them. USED BY RTC starting from zero-based index CONFIGURATION_BUFFER_INDEPENDENT_VALUES total of CONFIGURATION_RTC_INDEPENDENT_VALUES
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

    # Now, the NUMBER_OF_SESSIONS
    ordered_dictionary['NUMBER_OF_SESSIONS'] = json_config['NUMBER_OF_SESSIONS']

    # For each session, ALARM_HOUR_1 ALARM_MIN_1 RECORDING_SESSION_MINUTES_1 up to N
    for j in range(ordered_dictionary['NUMBER_OF_SESSIONS']):
        j = j + 1
        ordered_dictionary["ALARM_HOUR_" + str(j)] = json_config["ALARM_HOUR_" + str(j)]
        ordered_dictionary["ALARM_MINUTE_" + str(j)] = json_config["ALARM_MINUTE_" + str(j)]
        ordered_dictionary["RECORDING_SESSION_MINUTES_" + str(j)] = json_config["RECORDING_SESSION_MINUTES_" + str(j)]

    # Now we need to populate this thing all the way up to 256 elements...
    excess_needed = 64 - len(ordered_dictionary)
    for k in range(excess_needed):
        ordered_dictionary['packer_' + str(k)] = False
        if k == (excess_needed - 1):
            # CONFIG_SUCCESS is last element and we should set this to true.
            ordered_dictionary['packer_' + str(k)] = True

    return ordered_dictionary


dictionary_config = prepare_dictionary()
print("Dictionary prepared...")
print(len(dictionary_config))

def pack_dictionary():

    stringstruct = b''

    for entry in list(dictionary_config.keys()):
        stringstruct += struct.pack('<i', dictionary_config[entry])
        # see https://docs.python.org/3/library/struct.html#struct-alignment
        # we are packing with the < (little endianness) and i (int 4-byte int32) format

    return stringstruct


dictionary_stringstruct = pack_dictionary()
print("Packed dictionary into bytes...")

def send_configuration():

    print("Checking to see if Pico is ready to accept...")

    u = sert.readline().decode('UTF-8').strip()

    if u == "Ready to accept...":
        print("It is ready to accept! Writing data...")
        pass
    else:
        print(u)
        raise ValueError("Device not ready for configuration... error raised.")

    sert.write(dictionary_stringstruct)

    print("Verifying that data sent was correct by reading return...")
    return_stringstruct = sert.read(300).replace(b'\r', b'')  # 100 bytes max! Confirm this with the firmware.
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

for i in range(11):
    print(("Closing in {0}s ...").format(10-i))
    time.sleep(1)

print("Disconnect USB and turn off your vespertilio after doing so! :D")