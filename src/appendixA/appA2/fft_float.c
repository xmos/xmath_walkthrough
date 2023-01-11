// Copyright 2017-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <complex.h>

#include "appA2.h"
#include "floating_fft.h"
#include "xmath_fft_lut.h"

#define W_SIZE  ((1<<MAX_DIT_FFT_LOG2)-4)

static
complex_float_t W[W_SIZE] = {{0}};

void appA2_float_init()
{
  if(W[0].re == 0){
    for(int k = 0; k < W_SIZE; k++){
      W[k].re = F30(xmath_dit_fft_lut[k].re);
      W[k].im = F30(xmath_dit_fft_lut[k].im);
    }
  }
}

/**
 * 
*/
void appA2_float_complex_fft(
    complex_float_t frame_in_out[],
    const unsigned fft_n)
{
  fft_index_bit_reversal((complex_s32_t*) frame_in_out, fft_n);
  flt_fft_forward_float(frame_in_out, fft_n, W);
}

/**
 * 
*/
void appA2_float_complex_ifft(
    complex_float_t frame_in_out[],
    const unsigned fft_n)
{
  fft_index_bit_reversal((complex_s32_t*) frame_in_out, fft_n);
  flt_fft_inverse_float(frame_in_out, fft_n, W);
}

/**
 * 
*/
void appA2_float_real_fft(
    float frame_in_out[],
    const unsigned fft_n)
{
  appA2_float_complex_fft((complex_float_t*) frame_in_out, fft_n >> 1);
  flt_fft_mono_adjust_float((complex_float_t*) &frame_in_out[0], fft_n, 0, W);
}

/**
 * 
*/
void appA2_float_real_ifft(
    complex_float_t frame_in_out[],
    const unsigned fft_n)
{
  flt_fft_mono_adjust_float((complex_float_t*) &frame_in_out[0], fft_n, 1, W);
  appA2_float_complex_ifft((complex_float_t*) frame_in_out, fft_n >> 1);
}