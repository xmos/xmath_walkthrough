#pragma once

#include <stdint.h>

#define FRAME_BUFF_WORDS(N_SAMPLES)   ((N_SAMPLES)+2)

typedef struct {
  unsigned size;
  unsigned current;
  int32_t data[];
} frame_buffer_t;


void frame_reset(
    frame_buffer_t* fb,
    const unsigned frame_size);

unsigned frame_add_sample(
    frame_buffer_t* fb,
    const int32_t sample);

/**
 * If the current size of the supplied frame buffer is 0, this function returns
 * 0. Otherwise, the remaining frame data is padded with zeros and a non-zero
 * value is returned.
 * 
 * This is used to pad out the final frame to the right length.
 */
unsigned frame_finish(
    frame_buffer_t* fb);