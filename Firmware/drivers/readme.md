# This is just a quick note on these libraries
There are a few bits here which I did not write myself! These include...

- FatFs_SPI is drawn from no-os-FatFS, the popular library used on the Pico for handling SD cards and the like. carlk3 wrote it! He is great! I modified f_write to allow for 2.5 MBps writing of the ADC (limit- the ADC in reality only caps out at 1 MBps, but I have qualified my method to 2.5 MBps, so you know that you are safe recording at 499.999 kHz! :D)
- bme280, which was mostly written in pico_examples (and why I chose to use the BME280! Though I'll move to the BME688 at some point and use Bosch's library for it then.)

Unless clearly stated otherwise, other bits of code are probably written by me- I tend to be pretty obvious when I note that someone else wrote some code. 
