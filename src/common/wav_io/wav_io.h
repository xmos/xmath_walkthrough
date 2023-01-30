// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once


#ifndef __XC__
# include "wav_utils.h"
# include <xcore/channel.h>
# include <xcore/chanend.h>
# include <xcore/channel_transaction.h>
#endif

#ifdef __XC__
extern "C" {
  void wav_io_task(
      chanend c_audio, 
      chanend c_timing,
      const char* input_file_name, 
      const char* output_file_name,
      const char* perf_file_name);
}
#else
  void wav_io_task(
      chanend_t c_audio, 
      chanend_t c_timing,
      const char* input_file_name, 
      const char* output_file_name,
      const char* perf_file_name);

#endif