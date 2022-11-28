
#pragma once

#include <stdint.h>

#include "xmath/xmath.h"

static inline
int32_t ashr32(int32_t x, right_shift_t shr)
{
  int32_t y;
  if(shr >= 0) y = (x >> ( shr) );
  else         y = (x << (-shr) );
  return y;
}

static inline
int64_t ashr64(int64_t x, right_shift_t shr)
{
  int64_t y;
  if(shr >= 0) y = (x >> ( shr) );
  else         y = (x << (-shr) );
  return y;
}


static inline 
int32_t float_s32_to_fixed(float_s32_t v, exponent_t output_exp)
{
  right_shift_t shr = output_exp - v.exp;
  return ashr32(v.mant, shr); 
}


static inline 
int32_t float_s64_to_fixed(float_s64_t v, exponent_t output_exp)
{
  right_shift_t shr = output_exp - v.exp;
  return ashr64(v.mant, shr); 
}

static inline
int32_t float_to_fixed(float x, exponent_t output_exp)
{
  float_s32_t y = f32_to_float_s32(x);
  return float_s32_to_fixed(y, output_exp);
}

#ifndef __XC__

static inline 
void read_frame_as_double(
    double buff[],
    const chanend_t c_audio,
    const exponent_t input_exp,
    const unsigned frame_size)
{    
  for(int k = 0; k < frame_size; k++){
    // Read PCM sample from channel
    const int32_t sample_in = (int32_t) chan_in_word(c_audio);
    // Convert PCM sample to floating-point
    const double samp_f = ldexp(sample_in, input_exp);
    // Place at beginning of history buffer in reverse order (to match the
    // order of filter coefficients).
    buff[frame_size-k-1] = samp_f;
  }
}

static inline 
void send_frame_as_double(
    const chanend_t c_audio,
    const double buff[],
    const exponent_t output_exp,
    const unsigned frame_size)
{    
  // Send FRAME_SIZE new output samples at the end of each frame.
  for(int k = 0; k < frame_size; k++){
    // Get double sample from frame output buffer (in forward order)
    const double samp_f = buff[k];
    // Convert double sample back to PCM using the output exponent.
    const q1_31 sample_out = round(ldexp(samp_f, -output_exp));
    // Put PCM sample in output channel
    chan_out_word(c_audio, sample_out);
  }
}


static inline 
void read_frame_as_float(
    float buff[],
    const chanend_t c_audio,
    const exponent_t input_exp,
    const unsigned frame_size)
{    
  for(int k = 0; k < frame_size; k++){
    // Read PCM sample from channel
    const int32_t sample_in = (int32_t) chan_in_word(c_audio);
    // Convert PCM sample to floating-point
    const float samp_f = ldexpf(sample_in, input_exp);
    // Place at beginning of history buffer in reverse order (to match the
    // order of filter coefficients).
    buff[frame_size-k-1] = samp_f;
  }
}

static inline 
void send_frame_as_float(
    const chanend_t c_audio,
    const float buff[],
    const exponent_t output_exp,
    const unsigned frame_size)
{    
  // Send FRAME_SIZE new output samples at the end of each frame.
  for(int k = 0; k < frame_size; k++){
    // Get float sample from frame output buffer (in forward order)
    const float samp_f = buff[k];
    // Convert float sample back to PCM using the output exponent.
    const q1_31 sample_out = roundf(ldexpf(samp_f, -output_exp));
    // Put PCM sample in output channel
    chan_out_word(c_audio, sample_out);
  }
}


static inline 
void read_frame(
    int32_t buff[],
    const chanend_t c_audio,
    const unsigned frame_size)
{    
  for(int k = 0; k < frame_size; k++){
    // Read PCM sample from channel
    const q1_31 sample_in = (q1_31) chan_in_word(c_audio);
    // Place at beginning of history buffer in reverse order (to match the
    // order of filter coefficients).
    buff[frame_size-k-1] = sample_in;
  }
}

static inline 
void send_frame(
    const chanend_t c_audio,
    const int32_t buff[],
    const unsigned frame_size)
{    
  // Send FRAME_SIZE new output samples at the end of each frame.
  for(int k = 0; k < frame_size; k++){
    // Put PCM sample in output channel
    chan_out_word(c_audio, buff[k]);
    }
}

#endif //__XC__