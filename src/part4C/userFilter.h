// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once
#include "xmath/xmath.h"

// Number of filter coefficients 
#define TAP_COUNT_userFilter	(1024)

// The difference between the filter's output exponent and input exponent.
// For example, if the floating-point equivalent input to the filter is 1.0 in a Q1.30 format 
// (`((int32_t)ldexpf(1.0, 30)) == 0x40000000`) and the filter happens to output the value 
// 0x01010101, then the correct floating point conversion of the ouput value is 
// `ldexpf(0x01010101, -30 + userFilter_exp_diff)`.
extern const exponent_t userFilter_exp_diff;

// Exponent associated with filter outputs iff the exponent associated with the input is -30
// [DEPRECATED] This variable should not be used in new applications. `userFilter_exp_diff` gives the
// difference between the output and input exponents and should be used instead. This exponent is
// only correct if the fixed-point inputs are in the Q1.30 format.
extern const exponent_t userFilter_exp;

// Call once to initialize the filter
C_API
void userFilter_init();

// Call to add a sample to the filter without computing an output sample
C_API
void userFilter_add_sample(int32_t new_sample);

// Call to process an input sample and generate an output sample
C_API
int32_t userFilter(int32_t new_sample);
    