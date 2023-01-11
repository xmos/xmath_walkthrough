// Copyright 2017-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "appA2.h"


// This does not currently exist as a library function like fft_f32_forward()
complex_float_t* fft_f32_forward_complex(
    complex_float_t x[],
    const unsigned fft_length)
{
  exponent_t exp = vect_f32_max_exponent((float*) x, 2*fft_length) + 2;
  vect_f32_to_vect_s32((int32_t*) x, (float*) x, 2*fft_length, exp);

  // Now call the three functions to do an FFT
  fft_index_bit_reversal((complex_s32_t*) x, fft_length);
  headroom_t hr = 2;
  fft_dit_forward( (complex_s32_t*) x, fft_length, &hr, &exp);

  // And unpack back to floating point values
  vect_s32_to_vect_f32((float*) x, (int32_t*) x, 2*fft_length, exp);

  return &x[0];
}

// This does not currently exist as a library function like fft_f32_inverse()
complex_float_t* fft_f32_inverse_complex(
    complex_float_t X[],
    const unsigned fft_length)
{
  exponent_t exp = vect_f32_max_exponent((float*) X, 2*fft_length) + 2;
  vect_f32_to_vect_s32((int32_t*) X, (float*) X, 2*fft_length, exp);

  fft_index_bit_reversal((complex_s32_t*) X, fft_length);
  headroom_t hr = 2;
  fft_dit_inverse((complex_s32_t*) X, fft_length, &hr, &exp);

  vect_s32_to_vect_f32((float*) X, (int32_t*) X, 2*fft_length, exp);

  return &X[0];
}




void appA2_wrapped_real_fft(
    float frame_in_out[], 
    const unsigned fft_n)
{
  fft_f32_forward(frame_in_out, fft_n);
}


void appA2_wrapped_real_ifft(
    complex_float_t frame_in_out[], 
    const unsigned fft_n)
{
  fft_f32_inverse(frame_in_out, fft_n);
}


void appA2_wrapped_complex_fft(
    complex_float_t frame_in_out[], 
    const unsigned fft_n)
{
  fft_f32_forward_complex(frame_in_out, fft_n);
}


void appA2_wrapped_complex_ifft(
    complex_float_t frame_in_out[], 
    const unsigned fft_n)
{
  fft_f32_inverse_complex(frame_in_out, fft_n);
}
