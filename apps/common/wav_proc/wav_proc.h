
#pragma once


#ifndef __XC__
# include "fileio.h"
# include "wav_utils.h"
# include <xcore/channel.h>
# include <xcore/chanend.h>
# include <xcore/channel_transaction.h>
#endif

#ifdef __XC__
extern "C" {
  void wav_io_thread(
      chanend c_pcm_out, 
      chanend c_pcm_in, 
      chanend c_timing,
      const char* stage_name,
      const char* input_file_name, 
      const char* output_file_name);
}
#else
  void wav_io_thread(
      chanend_t c_pcm_out, 
      chanend_t c_pcm_in, 
      chanend_t c_timing,
      const char* stage_name,
      const char* input_file_name, 
      const char* output_file_name);

#endif