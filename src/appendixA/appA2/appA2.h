// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>

#include "xmath/xmath.h"

#define MIN_FFT_N_LOG2  (4)
#define MAX_FFT_N_LOG2  (10)

#define MIN_FFT_N   (1<<(MIN_FFT_N_LOG2))
#define MAX_FFT_N   (1<<(MAX_FFT_N_LOG2))

void appA2_bfp_real_fft(
    bfp_s32_t* frame_in_out);
void appA2_bfp_real_ifft(
    bfp_complex_s32_t* frame_in_out);
void appA2_bfp_complex_fft(
    bfp_complex_s32_t* frame_in_out);
void appA2_bfp_complex_ifft(
    bfp_complex_s32_t* frame_in_out);

void appA2_wrapped_real_fft(
    float frame_in_out[], 
    const unsigned fft_n);
void appA2_wrapped_real_ifft(
    complex_float_t frame_in_out[], 
    const unsigned fft_n);
void appA2_wrapped_complex_fft(
    complex_float_t frame_in_out[], 
    const unsigned fft_n);
void appA2_wrapped_complex_ifft(
    complex_float_t frame_in_out[], 
    const unsigned fft_n);


void appA2_float_init();
    
void appA2_float_real_fft(
    float frame_in_out[],
    const unsigned fft_n);
void appA2_float_real_ifft(
    complex_float_t frame_in_out[],
    const unsigned fft_n);
void appA2_float_complex_fft(
    complex_float_t frame_in_out[],
    const unsigned fft_n);
void appA2_float_complex_ifft(
    complex_float_t frame_in_out[],
    const unsigned fft_n);
    