/* hw_config.c
Copyright 2021 Carl John Kugler III
Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at
   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/
/*
This file should be tailored to match the hardware design.
There should be one element of the spi[] array for each hardware SPI used.
There should be one element of the sd_cards[] array for each SD card slot.
The name is should correspond to the FatFs "logical drive" identifier.
(See http://elm-chan.org/fsw/ff/doc/filename.html#vol)
The rest of the constants will depend on the type of
socket, which SPI it is driven by, and how it is wired.
*/

#include <string.h>
//
#include "my_debug.h"
//
#include "hw_config.h"
//
#include "ff.h" /* Obtains integer types */
//
#include "diskio.h" /* Declarations of disk functions */

void spi1_dma_isr();

// All the SRAM parameters for the SD CARD for the hardware configuration. You will need to manually copy these over to hw_config.c sadly. 
static const int SD_SPI_RX_PIN = 12;// Yellow 
static const int SD_SPI_CSN_PIN = 13; // White 
static const int SD_SPI_SCK_PIN = 14; // Blue
static const int SD_SPI_TX_PIN =  15; // Green 
static const int SD_BAUDRATE = 50*1000*1000; // 45*1000*1000; 

// The hardware configuration for the SD card we're using. 
static spi_t spis[] = {  // One for each SPI.
    {

        .hw_inst = spi1,  
        .miso_gpio = SD_SPI_RX_PIN, 
        .mosi_gpio = SD_SPI_TX_PIN, 
        .sck_gpio = SD_SPI_SCK_PIN,
        .baud_rate = SD_BAUDRATE,  // The limitation here is SPI slew rate.
        .dma_isr = spi1_dma_isr,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .set_drive_strength = true,
    }
};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0",           // Name used to mount device
        .spi = &spis[0],          // Pointer to the SPI driving this card
        .ss_gpio = SD_SPI_CSN_PIN,            // The SPI slave select GPIO for this SD card
        .use_card_detect = false,
        .card_detect_gpio = 21,   // Card detect
        .card_detected_true = -1,  // What the GPIO read returns when a card is
                                  // present. Use -1 if there is no card detect.
        .m_Status = STA_NOINIT
    }
};

void spi1_dma_isr() { spi_irq_handler(&spis[0]); }

/* ********************************************************************** */
size_t sd_get_num() { return count_of(sd_cards); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}
size_t spi_get_num() { return count_of(spis); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */