// Copyright 2020-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once

#include <complex.h>

#include "xmath/xmath.h"

EXTERN_C
void flt_fft_forward_float(
    complex_float_t x[],
    const unsigned N,
    const complex_float_t W[]);

EXTERN_C
void flt_fft_inverse_float(
    complex_float_t x[], 
    const unsigned N,
    const complex_float_t W[]);

EXTERN_C
void flt_fft_mono_adjust_float(
    complex_float_t x[],
    const unsigned FFT_N,
    const unsigned inverse,
    const complex_float_t W[]);