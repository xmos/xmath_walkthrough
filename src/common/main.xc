// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <xscope.h>
#include <stdlib.h>

#include "wav_io.h"
#include "timing.h"

extern "C" {
  extern void filter_task(chanend c_audio_data);
}

//// +main
int main(){
  // Channel used for communicating audio data between tile[0] and tile[1].
  chan c_audio_data;
  // Channel used for reporting timing info from tile[1] after the signal has
  // been processed.
  chan c_timing;

  par {
    // One thread runs on tile[0]
    on tile[0]: 
    {
      // Called so xscope will be used for prints instead of JTAG.
      xscope_config_io(XSCOPE_IO_BASIC);

      printf("Running Application: %s\n", APP_NAME);

      // This is where the app will spend all its time.
      wav_io_task(c_audio_data, 
                  c_timing, 
                  INPUT_WAV,    // These three macros are defined per-target
                  OUTPUT_WAV,   // in the CMake project.
                  OUTPUT_JSON);
      
      // Once wav_io_task() returns we are done.
      _Exit(0);
    }

    // Two threads on tile[1].
    // tiny task which just sits waiting to report timing info back to tile[0]
    on tile[1]: timer_report_task(c_timing);
    // The thread which does the signal processing.
    on tile[1]: filter_task(c_audio_data);
  }
  return 0;
}
//// -main