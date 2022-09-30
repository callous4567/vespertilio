import machine
import utime

# Define Pins for use
gpio_miso = 8
gpio_css = 9
gpio_sck = 10
gpio_mosi = 11

# Set up SPI (max baudrate for this model is 20M.)
baudrate = 20000000
cs = machine.Pin(gpio_css, machine.Pin.OUT)
spi = machine.SPI(1,
                  baudrate=baudrate,
                  polarity=1,
                  phase=1,
                  bits=8,
                  firstbit=machine.SPI.MSB,
                  sck=machine.Pin(gpio_sck),
                  mosi=machine.Pin(gpio_mosi),
                  miso=machine.Pin(gpio_miso))
cs(1)

# Instructions (8-bit) given as HEX 
SREAD = 0x03 # 0b00000011 
SWRIT = 0x02 # 0b00000010
RMREG = 0x05 # 0b00000101
WMREG = 0x01 # 0b00000001

# Mode register set (8bits = byte, 0b starts a bit string) 
MODE_BYTE = 0b00000000
MODE_PAGE = 0b10000000
MODE_SEQU = 0b01000000

# Set MAX ADDRESS (256KBits -> 259,999 + 1, 0-based.) 
MAX_ADDRESS = 255999
        
# Set the system to either BYTE, PAGE, or SEQUENTIAL mode
def set_mode(mode):
    
    if mode == 0:
        
        # Set to Byte Mode 
        cs(0)
        spi.write(bytearray([WMREG, MODE_BYTE]))
        cs(1)
        cs(0)
        u = spi.read(2, 0x05)[1:]
        print("Write mode has been set to... Byte!", u, bytes2binstr(u))
        cs(1)
        
    if mode == 1:
        
        cs(0)
        spi.write(bytearray([WMREG, MODE_PAGE]))
        cs(1)
        cs(0)
        u = spi.read(2, 0x05)[1:]
        print("Write mode has been set to... Page!", u, bytes2binstr(u))
        cs(1)
        
    if mode == 2:
        
        cs(0)
        spi.write(bytearray([WMREG, MODE_SEQU]))
        cs(1)
        cs(0)
        u = spi.read(2, 0x05)[1:]
        print("Write mode has been set to... Sequential!", u, bytes2binstr(u))
        cs(1)

# Convert a bytearray to a string representation (thanks Stackexchange.)
def bytes2binstr(b, n=None):
    s = ' '.join(f'{x:08b}' for x in b)
    return s if n is None else s[:n + n // 8 + (0 if n % 8 else -1)]

def read_byte(address):
    
    # 8192*32 bytes     
    # Read
    cs(0)
    spi.write(bytearray([SREAD, 0b000000 | address >> 16, address >> 8, address]))
    data = spi.read(1, 0x00)
    cs(1)
    
    # Return
    return data

def write_byte(address, data_byte):

    # 8192*32 bytes
    # Write
    cs(0)
    spi.write(bytearray([SWRIT, 0b000000 | address >> 16, address >> 8, address, data_byte]))
    cs(1)
  
# 8192 pages of 32 bytes = 262144 bytes (hence decimal from 0 to 262144 -1 = 262143!
# I've ignored Page 0 from here- it's an extra if-else loop (or some smart way of using Modulo/%.) Not useful for me.
def write_page(page, data):
    
    # Write
    cs(0)
    address = page*32 - 1
    spi.write(bytearray([SWRIT, 0b000000 | address >> 16, address >> 8, address]) + data)
    cs(1)
    
def read_page(page):
    
    # Read
    cs(0)
    address = page*32 - 1
    spi.write(bytearray([SREAD, 0b000000 | address >> 16, address >> 8, address]))
    data = spi.read(32, 0x00)
    cs(1)
    return data

# Some tests
"""    

# Go for page mode 
set_mode(1)

# Set up a dummy page (with start/finish bytes.) 
example_data = bytearray([0b00000000,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,
                          0b11111111])

# Write the page
page_start = 999
write_page(page_start, example_data)

# Set mode back to byte
set_mode(0)

# Read the start/finish byte to verify it worked (IT DID! :D) 
start_byte = read_byte(page_start*32 - 1)
end_byte = read_byte(page_start*32 - 1 + 32)
print(bytes2binstr(start_byte), bytes2binstr(end_byte))

# Write 6000 pages and time it. That's 6,000 * 32 bytes = 192 kilobytes. Takes 0.8 seconds. 
import utime
start = utime.ticks_us()
i = 1 
while i < 6001:
    write_page(i, example_data)
    i += 1 
    
delta = utime.ticks_diff(utime.ticks_us(), start) 
print("To write 192 kilobytes...", delta)

# Read 6000 pages and time it. That's 6,000 * 32 bytes = 192 kilobytes. Takes XXX seconds. 
import utime
start = utime.ticks_us()
i = 1 
while i < 6001:
    read_page(i)
    i += 1 
    
delta = utime.ticks_diff(utime.ticks_us(), start) 
print("To read 192 kilobytes...", delta)

"""

# An example on how to use sequential mode (with pages- most efficient way to add data sequentially is with these.)
"""
# Set mode to sequential + start timer 
start = utime.ticks_us()
set_mode(2)

# Set up a dummy page (with start/finish bytes.) 
example_data = bytearray([0b00000000,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,0b01010101,0b01010101,
                          0b01010101,0b01010101,
                          0b11111111])

# Enable
cs(0)

# Write starting from zero
spi.write(bytearray([SWRIT, 0b000000 | 0 >> 16, 0 >> 8, 0]))
i=0
while i < 18000:
    
    spi.write(example_data)
    i += 1
    
cs(1)
delta = utime.ticks_diff(utime.ticks_us(), start) 
print("To write sequential... 18000 pages...", delta/1000000)

# Test page to check
page_test = 13

# Read the start/finish byte to verify it worked (IT DID! :D)
start_byte = read_byte(page_test*32)
end_byte = read_byte(page_test*32 + 31)
print(bytes2binstr(start_byte), bytes2binstr(end_byte))

# Now test the read
spi.write(bytearray([SREAD, 0b000000 | 0 >> 16, 0 >> 8, 0]))
i=0
while i < 4500:
    
    spi.read(128, 0x00)
    i += 1
    
cs(1)
delta = utime.ticks_diff(utime.ticks_us(), start) 
print("To read sequential... 18000 pages...", delta/1000000)

"""
