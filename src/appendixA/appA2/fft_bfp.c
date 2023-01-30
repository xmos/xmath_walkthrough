// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "appA2.h"


void appA2_bfp_real_fft(
    bfp_s32_t* frame_in_out)
{
  bfp_fft_forward_mono(frame_in_out);
}

void appA2_bfp_real_ifft(
    bfp_complex_s32_t* frame_in_out)
{
  bfp_fft_inverse_mono(frame_in_out);
}

void appA2_bfp_complex_fft(
    bfp_complex_s32_t* frame_in_out)
{
  bfp_fft_forward_complex(frame_in_out);
}

void appA2_bfp_complex_ifft(
    bfp_complex_s32_t* frame_in_out)
{
  bfp_fft_inverse_complex(frame_in_out);
}
