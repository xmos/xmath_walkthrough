
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