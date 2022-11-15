// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <xscope.h>
#include <stdlib.h>

#include "wav_proc.h"
#include "timing.h"

#include "xscope_io_device.h"


extern "C" {
  extern void filter_thread(chanend c_pcm_in, chanend c_pcm_out);
}

#ifndef INPUT_WAV
# define INPUT_WAV  "xmath_walkthrough/wav/input.wav"
#endif

#ifndef OUTPUT_WAV_FMT
# define OUTPUT_WAV_FMT "out/output-stage%u.wav"
#endif

int main(){
    chan xscope_chan;
    chan c_tile0_to_tile1;
    chan c_tile1_to_tile0;
    chan c_timing;

    par {
        xscope_host_data(xscope_chan);
        on tile[0]: 
        {
          xscope_io_init(xscope_chan);

          printf("Running Application: stage%u\n", STAGE_NUMBER);

          char str_buff[100];
          sprintf(str_buff, OUTPUT_WAV_FMT, STAGE_NUMBER);

          wav_io_thread(c_tile0_to_tile1, 
                        c_tile1_to_tile0, 
                        c_timing, 
                        STAGE_NUMBER,
                        INPUT_WAV, 
                        str_buff);
          _Exit(0);
        }

        on tile[1]: timer_report_task(c_timing);
        on tile[1]: filter_thread(c_tile0_to_tile1, c_tile1_to_tile0);
    }
    return 0;
}
