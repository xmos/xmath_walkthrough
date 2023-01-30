// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once


#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef __XC__
# include <xcore/channel.h>
# include <xcore/chanend.h>
# include <xcore/hwtimer.h>
#endif

#include "xmath/xmath.h"

#include "timing.h"
#include "misc_func.h"


#define TAP_COUNT     (1024)
#define FRAME_SIZE    (256)
#define HISTORY_SIZE  (TAP_COUNT + FRAME_SIZE)