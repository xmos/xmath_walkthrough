
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
#define FRAME_OVERLAP (256)
#define FRAME_SIZE    (TAP_COUNT + FRAME_OVERLAP)

