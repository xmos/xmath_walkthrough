// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once

#include <stdint.h>

#include "xmath/xmath.h"

// Signed, arithmetic right-shift of a 32-bit integer, with saturation
static inline
int32_t ashr32(int32_t x, right_shift_t shr)
{
  if(shr >= 0) return x >> shr;

  int64_t y = ((int64_t)x) << (-shr);

  if(y <= INT32_MIN) return INT32_MIN;
  if(y >= INT32_MAX) return INT32_MAX;
  return y;
}

// Signed, arithmetic right-shift of a 64-bit integer
static inline
int64_t ashr64(int64_t x, right_shift_t shr)
{
  int64_t y;
  if(shr >= 0) y = (x >> ( shr) );
  else         y = (x << (-shr) );
  return y;
}

// Saturate 64-bit integer to 32-bit bounds
static inline
int32_t sat32(int64_t x)
{
  if(x <= INT32_MIN) return INT32_MIN;
  if(x >= INT32_MAX) return INT32_MAX;
  return x;
}

// Convert float_s32_t to a 32-bit fixed-point value using the specified
// exponent
static inline 
int32_t float_s32_to_fixed(float_s32_t v, exponent_t output_exp)
{
  right_shift_t shr = output_exp - v.exp;
  return ashr32(v.mant, shr); 
}

// Convert float_s64_t to a 32-bit fixed-point value using the specified
// exponent, with saturation
static inline 
int32_t float_s64_to_fixed(float_s64_t v, exponent_t output_exp)
{
  right_shift_t shr = output_exp - v.exp;
  return sat32(ashr64(v.mant, shr)); 
}

// Convert float to 32-bit fixed-point value using the specified exponent
static inline
int32_t float_to_fixed(float x, exponent_t output_exp)
{
  float_s32_t y = f32_to_float_s32(x);
  return float_s32_to_fixed(y, output_exp);
}