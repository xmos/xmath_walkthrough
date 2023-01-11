// Copyright 2017-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>

#include "xmath/xmath.h"


void filter_wrapped_init(
    const unsigned tap_count);

void filter_wrapped(
    float frame_in_out[]);


void filter_wrapped_deinit();


void filter_float_init(
    const unsigned tap_count);

void filter_float(
    float frame_in_out[]);


void filter_float_deinit();