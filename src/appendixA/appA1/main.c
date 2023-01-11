// Copyright 2017-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <xscope.h>
#include <stdlib.h>
#include <xcore/hwtimer.h>
#include <math.h>

#include "appA1.h"


#define MAX_TAPS   (1024)
#define INCR_TAPS  (64)

// Frame size is always tap count / 8
#define MAX_FRAME_SIZE  ((MAX_TAPS) >> 3)

// The number of frames to send for each filter length
#define FRAMES_PER_ITER  (16)

float frame[MAX_FRAME_SIZE] = {0};

static unsigned t_start = 0;


static inline 
void timer_start()
{
  t_start = get_reference_time();
}

static inline
unsigned timer_stop()
{
  unsigned t_stop = get_reference_time();
  return t_stop - t_start;
}

static inline
void rand_frame(
    const unsigned frame_size)
{
  for(int k = 0; k < frame_size; k++){
    frame[k] = ldexpf(rand(), -31);
  }
}

int main()
{
  xscope_config_io(XSCOPE_IO_BASIC);
  
  printf("Now running appA1.\n");

  srand(0x12345678);

  // We'll store the average tick counts so we can print them nicely at the end
  float ave_ticks_us[2][(MAX_TAPS / INCR_TAPS)] = {{0.0f}};
  unsigned dex = 0;


  // Iterate over various numbers of filter taps 
  for(int tap_count = (dex+1)*INCR_TAPS; tap_count <= MAX_TAPS; tap_count += INCR_TAPS){
    printf("tap_count: %d\n", tap_count);

    const unsigned frame_size = tap_count >> 3;

    // First do the float filter
    if(1){
      printf("  Float filter...\n");
      // Initialize
      filter_float_init(tap_count);

      uint64_t total_ticks = 0UL;

      // Send frames
      for(int frame_num = 0; frame_num < FRAMES_PER_ITER; frame_num++){
        rand_frame(frame_size);
        timer_start();
        filter_float(frame);
        total_ticks += timer_stop();
      }
      // De-initialize
      filter_float_deinit();

      // Report
      const float ave_ticks = (1.0f * total_ticks) / (MAX_TAPS / INCR_TAPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[0][dex] = ave_us;
    }

    // Then the wrapped filter
    if(1){
      printf("  Wrapped filter...\n");
      // Initialize
      filter_wrapped_init(tap_count);

      uint64_t total_ticks = 0UL;

      // Send frames
      for(int frame_num = 0; frame_num < FRAMES_PER_ITER; frame_num++){
        rand_frame(frame_size);
        timer_start();
        filter_wrapped(frame);
        total_ticks += timer_stop();
      }
      // De-initialize
      filter_wrapped_deinit();

      // Report
      const float ave_ticks = (1.0f * total_ticks) / (MAX_TAPS / INCR_TAPS);
      const float ave_us = ave_ticks * 0.01f; // Reference clock is 100 MHz
      ave_ticks_us[1][dex] = ave_us;
    }

    dex++;
  }


  // Print nice tables

  printf("\n\n");

  printf("|--------------------------------------------|\n");
  printf("|              Frame Time (us)               |\n");
  printf("|------|---------|---------------------------|\n");
  printf("|   N  |   Taps  |    Float    |   Wrapped   |\n");
  printf("|------|---------|-------------|-------------|\n");
  for(int k = 0; k < (MAX_TAPS / INCR_TAPS); k++){
    int tap_count = INCR_TAPS * (k+1);

    printf("| % 3d  ", k);
    printf("|  % 5d  ", tap_count);
    printf("|  % 9.02f  ", ave_ticks_us[0][k]);
    printf("|  % 9.02f  |", ave_ticks_us[1][k]);

    printf("\n");
  }
  printf("|--------------------------------------------|\n");

  printf("\n\n");

  printf("|--------------------------------------------|\n");
  printf("|             Sample Time (us)               |\n");
  printf("|------|---------|---------------------------|\n");
  printf("|   N  |   Taps  |    Float    |   Wrapped   |\n");
  printf("|------|---------|-------------|-------------|\n");
  for(int k = 0; k < (MAX_TAPS / INCR_TAPS); k++){
    int tap_count = INCR_TAPS * (k+1);
    int frame_size = tap_count >> 3;

    printf("| % 3d  ", k);
    printf("|  % 5d  ", tap_count);
    printf("|  % 9.02f  ", ave_ticks_us[0][k] / frame_size);
    printf("|  % 9.02f  |", ave_ticks_us[1][k] / frame_size);

    printf("\n");
  }
  printf("|--------------------------------------------|\n");

  printf("\n\n");
  printf("|-------------------------------------------------|\n");
  printf("|              Sample Time Ratios                 |\n");
  printf("|-------------------------------------------------|\n");
  printf("|   N  | Float[N] /   | Float[N] / | Wrapped[N] / |\n");
  printf("|      |  Wrapped[N]  |  Float[0]  |  Wrapped[0]  |\n");
  printf("|------|--------------|------------|--------------|\n");
  for(int k = 0; k < (MAX_TAPS / INCR_TAPS); k++){
    int tap_count = INCR_TAPS * (k+1);
    int frame_size = tap_count >> 3;

    printf("| % 3d  ", k);

    printf("|    %6.02f    ", ave_ticks_us[0][k] / ave_ticks_us[1][k]);
    printf("|  %7.02f   ", (ave_ticks_us[0][k] / frame_size) / (ave_ticks_us[0][0] / (INCR_TAPS>>3)));
    printf("|   %7.02f    |", (ave_ticks_us[1][k] / frame_size) / (ave_ticks_us[1][0] / (INCR_TAPS>>3)));

    printf("\n");
  }
  printf("|-------------------------------------------------|\n");


  

  printf("\n\n");
  printf("|-------------------------------|\n");
  printf("|       Accumulation Rates      |\n");
  printf("|-------------------------------|\n");
  printf("|  Taps   |   Float  |  Wrapped |\n");
  printf("|---------|----------|----------|\n");

  for(int k = 0; k < (MAX_TAPS / INCR_TAPS); k++){
    int tap_count = INCR_TAPS * (k+1);
    int frame_size = tap_count >> 3;
    int maccs_per_frame = tap_count * frame_size;

    printf("| % 6d  ", tap_count);

    printf("| %7.02e ", (maccs_per_frame / ave_ticks_us[0][k]) * 1e6);
    printf("| %7.02e |", (maccs_per_frame / ave_ticks_us[1][k]) * 1e6);

    printf("\n");
  }
  printf("|-------------------------------|\n");


  return 0;
}