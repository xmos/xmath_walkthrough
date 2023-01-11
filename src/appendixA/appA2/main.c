// Copyright 2017-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <xscope.h>
#include <stdlib.h>
#include <xcore/hwtimer.h>
#include <math.h>
#include <string.h>

#include "appA2.h"

#define FFT_COUNT   ((MAX_FFT_N_LOG2) - (MIN_FFT_N_LOG2) + 1)
#define REPS        (8)

enum {
  FLOAT = 0,
  BFP = 1,
  WRAPPED = 2,
  TYPE_COUNT = 3,
} _fft_type;

static unsigned t_start = 0;

/**
 * Start timer using 100MHz reference clock
 */
static inline 
void timer_start()
{
  t_start = get_reference_time();
}

/**
 * Stop timer and get elapsed time in reference clock ticks
 */
static inline
unsigned timer_stop()
{
  unsigned t_stop = get_reference_time();
  return t_stop - t_start;
}

/**
 * Print out timing info in a table
 */
void print_tables(
    const float ave_ticks_us[TYPE_COUNT][FFT_COUNT],
    const char* time_title)
{
  printf("\n");

  char title_buff[52];
  memset(title_buff, ' ', sizeof(title_buff));

  const size_t n = strnlen(time_title, 52);

  memcpy(&title_buff[26-(n>>1)], time_title, n);



  printf("+---------------------------------------------------+\n");
  printf("|%.*s|\n", 51, title_buff);
  printf("+---------+-------------+-------------+-------------+\n");
  printf("|  FFT_N  |    Float    |     BFP     |   Wrapped   |\n");
  printf("+---------+-------------+-------------+-------------+\n");
  for(int k = 0; k < FFT_COUNT; k++){
    int fft_n = (1 << (k+(MIN_FFT_N_LOG2)));

    printf("|  % 5d  ", fft_n);
    printf("|  % 9.02f  ", ave_ticks_us[FLOAT][k]);
    printf("|  % 9.02f  ", ave_ticks_us[BFP][k]);
    printf("|  % 9.02f  |", ave_ticks_us[WRAPPED][k]);

    printf("\n");
  }
  printf("+---------+-------------+-------------+-------------+\n");
}


/**
 * Real FFTs
 */
void real_fft()
{
  // We'll store the average tick counts so we can print them nicely at the end
  float ave_ticks_us[TYPE_COUNT][(FFT_COUNT)] = {{0.0f}};
  unsigned dex = 0;

  for(unsigned FFT_N = MIN_FFT_N; FFT_N <= MAX_FFT_N; FFT_N *= 2){

    ////// FLOAT
    if(1){

      // Frame data
      float DWORD_ALIGNED frame[MAX_FFT_N] = {0};
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++)
          frame[k] = ldexpf(rand(), -30);

        timer_start();
        appA2_float_real_fft(frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[FLOAT][dex] = ave_us;
    }

    ////// BFP
    if(1){

      // Frame data
      int32_t DWORD_ALIGNED frame[MAX_FFT_N] = {0};

      bfp_s32_t bfp_frame;
      bfp_s32_init(&bfp_frame, frame, -30, FFT_N, 0);
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){

        for(int k = 0; k < FFT_N; k++)
          bfp_frame.data[k] = rand();

        // HR has to be correct
        bfp_s32_headroom(&bfp_frame);

        timer_start();
        appA2_bfp_real_fft(&bfp_frame);
        total_ticks += timer_stop();
        
        // These get modified within fft_bfp(), so they need to be reset
        bfp_frame.length = FFT_N;
        bfp_frame.exp = -30;
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[BFP][dex] = ave_us;
    }

    ////// WRAPPED
    if(1){

      // Frame data
      float DWORD_ALIGNED frame[MAX_FFT_N] = {0};
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++)
          frame[k] = ldexpf(rand(), -30);

        timer_start();
        appA2_wrapped_real_fft(frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[WRAPPED][dex] = ave_us;
    }

    dex++;
  }

  print_tables(ave_ticks_us, "Real FFT Time (us)");

}


/**
 * Real iFFTs
 */
void real_ifft()
{
  // We'll store the average tick counts so we can print them nicely at the end
  float ave_ticks_us[TYPE_COUNT][(FFT_COUNT)] = {{0.0f}};
  unsigned dex = 0;

  for(unsigned FFT_N = MIN_FFT_N; FFT_N <= MAX_FFT_N; FFT_N *= 2){

    ////// FLOAT
    if(1){

      // Frame data
      float DWORD_ALIGNED frame[MAX_FFT_N] = {0};
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++)
          frame[k] = ldexpf(rand(), -30);
        
        // Start with forward FFT
        appA2_float_real_fft(frame, FFT_N);

        timer_start();
        appA2_float_real_ifft((complex_float_t*) frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[FLOAT][dex] = ave_us;
    }

    ////// BFP
    if(1){

      // Frame data
      int32_t DWORD_ALIGNED frame[MAX_FFT_N] = {0};

      bfp_s32_t bfp_frame;
      bfp_s32_init(&bfp_frame, frame, -30, FFT_N, 0);
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){

        for(int k = 0; k < FFT_N; k++)
          bfp_frame.data[k] = rand();

        // HR has to be correct
        bfp_s32_headroom(&bfp_frame);
        
        // Start with forward FFT
        appA2_bfp_real_fft(&bfp_frame);

        timer_start();
        appA2_bfp_real_ifft((bfp_complex_s32_t*) &bfp_frame);
        total_ticks += timer_stop();
        
        // These get modified within fft_bfp(), so they need to be reset
        bfp_frame.length = FFT_N;
        bfp_frame.exp = -30;
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[BFP][dex] = ave_us;
    }

    ////// WRAPPED
    if(1){

      // Frame data
      float DWORD_ALIGNED frame[MAX_FFT_N] = {0};
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++)
          frame[k] = ldexpf(rand(), -30);
        
        // Start with forward FFT
        appA2_wrapped_real_fft(frame, FFT_N);

        timer_start();
        appA2_wrapped_real_ifft((complex_float_t*) frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[WRAPPED][dex] = ave_us;
    }

    dex++;
  }

  print_tables(ave_ticks_us, "Real IFFT Time (us)");

}


/**
 * Complex FFTs
 */
void complex_fft()
{
  // We'll store the average tick counts so we can print them nicely at the end
  float ave_ticks_us[TYPE_COUNT][(FFT_COUNT)] = {{0.0f}};
  unsigned dex = 0;

  for(unsigned FFT_N = MIN_FFT_N; FFT_N <= MAX_FFT_N; FFT_N *= 2){

    ////// FLOAT
    if(1){

      // Frame data
      complex_float_t DWORD_ALIGNED frame[MAX_FFT_N] = {{0}};

      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++){
          frame[k].re = ldexpf(rand(), -30);
          frame[k].im = ldexpf(rand(), -30);
        }

        timer_start();
        appA2_float_complex_fft(frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[FLOAT][dex] = ave_us;
    }

    ////// BFP
    if(1){

      // Frame data
      complex_s32_t DWORD_ALIGNED frame[MAX_FFT_N] = {{0}};

      bfp_complex_s32_t bfp_frame;
      bfp_complex_s32_init(&bfp_frame, frame, -30, FFT_N, 0);
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){

        for(int k = 0; k < FFT_N; k++){
          bfp_frame.data[k].re = rand();
          bfp_frame.data[k].im = rand();
        }

        // HR has to be correct
        bfp_complex_s32_headroom(&bfp_frame);

        timer_start();
        appA2_bfp_complex_fft(&bfp_frame);
        total_ticks += timer_stop();
        
        // These get modified within fft_bfp(), so they need to be reset
        bfp_frame.length = FFT_N;
        bfp_frame.exp = -30;
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[BFP][dex] = ave_us;
    }

    ////// WRAPPED
    if(1){

      // Frame data
      complex_float_t DWORD_ALIGNED frame[MAX_FFT_N] = {{0}};
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++){
          frame[k].re = ldexpf(rand(), -30);
          frame[k].im = ldexpf(rand(), -30);
        }

        timer_start();
        appA2_wrapped_complex_fft(frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[WRAPPED][dex] = ave_us;
    }

    dex++;
  }

  print_tables(ave_ticks_us, "Complex FFT Time (us)");

}


/**
 * Complex iFFTs
 */
void complex_ifft()
{
  // We'll store the average tick counts so we can print them nicely at the end
  float ave_ticks_us[TYPE_COUNT][(FFT_COUNT)] = {{0.0f}};
  unsigned dex = 0;

  for(unsigned FFT_N = MIN_FFT_N; FFT_N <= MAX_FFT_N; FFT_N *= 2){

    ////// FLOAT
    if(1){

      // Frame data
      complex_float_t DWORD_ALIGNED frame[MAX_FFT_N] = {{0}};

      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++){
          frame[k].re = ldexpf(rand(), -30);
          frame[k].im = ldexpf(rand(), -30);
        }
        
        // Start with forward FFT
        appA2_float_complex_fft(frame, FFT_N);

        timer_start();
        appA2_float_complex_ifft(frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[FLOAT][dex] = ave_us;
    }

    ////// BFP
    if(1){

      // Frame data
      complex_s32_t DWORD_ALIGNED frame[MAX_FFT_N] = {{0}};

      bfp_complex_s32_t bfp_frame;
      bfp_complex_s32_init(&bfp_frame, frame, -30, FFT_N, 0);
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){

        for(int k = 0; k < FFT_N; k++){
          bfp_frame.data[k].re = rand();
          bfp_frame.data[k].im = rand();
        }

        // HR has to be correct
        bfp_complex_s32_headroom(&bfp_frame);
        
        // Start with forward FFT
        appA2_bfp_complex_fft(&bfp_frame);

        timer_start();
        appA2_bfp_complex_ifft(&bfp_frame);
        total_ticks += timer_stop();
        
        // These get modified within fft_bfp(), so they need to be reset
        bfp_frame.length = FFT_N;
        bfp_frame.exp = -30;
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[BFP][dex] = ave_us;
    }

    ////// WRAPPED
    if(1){

      // Frame data
      complex_float_t DWORD_ALIGNED frame[MAX_FFT_N] = {{0}};
      
      uint64_t total_ticks = 0UL;

      for(int rep = 0; rep < REPS; rep++){
        for(int k = 0; k < FFT_N; k++){
          frame[k].re = ldexpf(rand(), -30);
          frame[k].im = ldexpf(rand(), -30);
        }
        
        // Start with forward FFT
        appA2_wrapped_complex_fft(frame, FFT_N);

        timer_start();
        appA2_wrapped_complex_ifft(frame, FFT_N);
        total_ticks += timer_stop();
      }

      // Calc average time
      const float ave_ticks = (1.0f * total_ticks) / (REPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[WRAPPED][dex] = ave_us;
    }

    dex++;
  }

  print_tables(ave_ticks_us, "Complex IFFT Time (us)");

}



int main()
{
  xscope_config_io(XSCOPE_IO_BASIC);
  
  printf("Now running appA2.\n");
  
  appA2_float_init();

  srand(0xABCDEF01);

  real_fft();
  complex_fft();
  real_ifft();
  complex_ifft();

  return 0;
}