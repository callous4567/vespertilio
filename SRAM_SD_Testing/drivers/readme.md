# This is just a quick note on these libraries
There are a few bits here which I did not write myself! These include...

- FatFs_SPI is drawn from no-os-FatFS, the popular library used on the Pico for handling SD cards and the like. I'll modify bits here and there in the future to speed it up for my use case (the write speed is a bit dismal, if I'm honest- it's because only single-block writes are used at once. I'll also try to get SDIO working but, no promises there.)
- bme280, which was mostly written in pico_examples (and why I chose to use the BME280! Though I'll move to the BME688 at some point and use Bosch's library for it then.)

Unless clearly stated otherwise, other bits of code are probably written by me- I tend to be pretty obvious when I note that someone else wrote some code. 
