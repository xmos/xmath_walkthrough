// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/**
 * This file defines a simple 1024-tap averaging filter with fixed-point 
 * coefficients in a Q4.28 format.
 */
#include "common.h"

// Each coefficient is (1.0/1024) expressed in Q4.28 format
//  2^(-10) * 2^(28) = 2^(18) = 0x40000
#define B         (0x40000)

/**
 * 1024-tap Averaging Filter
 * 
 * Coefficients are all identical, but formally the coefficients are in 
 * ascending order.  e.g. b[0], b[1], b[2], etc
 */
const q4_28 filter_coef[TAP_COUNT] = {
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, // 32
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, // 64
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, // 128
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, // 256

  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, // 512

  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B,
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B,
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B,
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B,

  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, 
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, // 1024
};
