#ifndef MSD_H
#define MSD_H 

#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "hw_config.h"

// struct to contain all mSD variables applicable for us to use (that may change.) All malloc'd except for the sd_card_t object. 
typedef struct {

    // the SD card object. NOT IN MALLOC.
    sd_card_t *pSD;

    // file object structures for audio file + environment file 
    FIL *fp_audio;
    FIL *fp_env;

    // bytes written 
    UINT *bw;
    UINT *bw_env;

    // filenames of the two files for data recording (the audio file and the environmental data file)
    char *fp_audio_filename;
    char *fp_env_filename;

} mSD_struct_t; 

// check if file exists on SD (returns a bool true/false) 
bool SD_IS_EXIST(const char *test_filename);

void characterize_SD_write_time(int32_t adc_buf_size_samples);
#endif // MSD_H 