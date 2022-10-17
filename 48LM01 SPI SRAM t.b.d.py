import machine
import utime
import ustruct
import sys

# Define Pins
gpio_enable = 11
gpio_miso = 12
gpio_css = 13
gpio_sck = 14
gpio_mosi = 15
which_hardware = 1

machine.freq(140000000)

# Set enable
enable_pin = machine.Pin(gpio_enable, machine.Pin.OUT)
def enable_sram_module():
    enable_pin.off()
def disable_sram_module():
    enable_pin.on()
    
# Enable it
enable_sram_module() 

# Set up SPI
baudrate = 66000000
cs = machine.Pin(gpio_css, machine.Pin.OUT)
spi = machine.SPI(which_hardware,
                  baudrate=baudrate,
                  polarity=1,
                  phase=0,
                  bits=8,
                  firstbit=machine.SPI.MSB,
                  sck=machine.Pin(gpio_sck),
                  mosi=machine.Pin(gpio_mosi),
                  miso=machine.Pin(gpio_miso))
cs(1)

"""

The status register of the 48LM01 has 8 bits! (It's a standard byte.) Just give us the statis for the device! :) 

7) Set to 0
6) True/False Disable/Enable AutoStore (False/Enable default)
5) 0
4) Secure Write Monitor- redundant for us
3-2) Write Protection- redundant for us
1) Write Enable Latch Bit (Read Only.) True means the device is ready for writing/enabled, and False means it is not write enabled.
0) RDY/BSY Ready/Busy Status (Read only.) True means the device is busy, and False means it is ready. 

By default, we should set the register byte to...

0b01000000

"""

# Instructions (8-bit) for standard read/write. Note that the mode is automatically sequential: keep sending data, it'll keep writing it. To stop, pull CS high + send SWRDI.
SREAD = 0b00000011
SWRIT = 0b00000010

# Instructions (8-bit) for status register management 
RMREG = 0b00000101
WMREG = 0b00000001

# Instructions (8-bit) for write latch control: Set it, Reset it. 
SWREN = 0b00000110 # Must be sent prior to any WRITE operation to the chip 
SWRDI = 0b00000100 # Send after any WRITE operation has been completed 

# Set MAX ADDRESS. We have 131,072 bytes (meaning 131,071 is the max byte address.) In binary, that's a 17-bit value: 24-bit addresses.
# Note: We have 128 byte pages- i.e. 1024 pages available. 
MAX_ADDRESS = 131071 

# Convert a bytearray to a string representation 
def bytes2binstr(b, n=None):
    s = ' '.join(f'{x:08b}' for x in b)
    return s if n is None else s[:n + n // 8 + (0 if n % 8 else -1)]

def read_byte(address):
    
    # 131,072 bytes organized as   
    # Read
    cs(0)
    spi.write(bytearray([SREAD, 0b0000000 | address >> 16, address >> 8, address]))
    data = spi.read(1, 0x00)
    cs(1)
    
    # Return
    return data

def write_byte(address, data_byte):
    
    # Write
    cs(0)
    spi.write(bytearray([SWREN]))
    cs(1)
    cs(0)
    spi.write(bytearray([SWRIT, 0b0000000 | address >> 16, address >> 8, address, data_byte]))
    cs(1)
    cs(0)
    spi.write(bytearray([SWRDI])) # write disable command 
    cs(1)
  
# 8192 pages of 32 bytes = 262144 bytes (hence decimal from 0 to 262144 -1 = 262143!
# page indexing from 0 to 8191.
# We will save the first page (page 0) to make things easier- we can use page 0 for debug/etc. 
# page_start = page*32 - 1 
def write_page(page, data):
    
    # Write
    cs(0)
    spi.write(bytearray([SWREN]))
    cs(1)
    cs(0)
    address = page*128 - 1
    spi.write(bytearray([SWRIT, 0b0000000 | address >> 16, address >> 8, address]) + data)
    cs(1)
    cs(0)
    spi.write(bytearray([SWRDI])) # write disable command 
    cs(1)
    
def read_page(page):
    
    # Write
    cs(0)
    address = page*128 - 1
    spi.write(bytearray([SREAD, 0b0000000 | address >> 16, address >> 8, address]))
    data = spi.read(128, 0x00)
    cs(1)
    return data

def read_status_register():
    
    # Grab the status register!
    cs(0)
    spi.write(bytearray([RMREG]))
    u=spi.read(10,0x00) # really gun for it with the 0b0...'s 
    print(bytes2binstr(u))
    cs(1)
    
read_status_register()