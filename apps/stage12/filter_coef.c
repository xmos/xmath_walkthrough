
/**
 * This file defines a 1024-tap averaging filter with fixed-point 
 * coefficients in a Q4.28 format.
 * 
 * In this case, the 1024-tap filter is padded on both ends with zeros to
 * facilitate 'sliding' the filter across a longer input sequence while 
 * nullifying the samples outside of the window.
 */
#include "common.h"

// Each coefficient is (1.0/1024) expressed in Q4.28 format
//  2^(-10) * 2^(28) = 2^(18) = 0x40000
#define B         (0x40000)


/**
 * 1024-tap Averaging Filter padded on each side with 256 zeros.
 * 
 * Other than padding, coefficients are all identical, but formally the
 * coefficients are in ascending order.  e.g. b[0], b[1], b[2], etc
 * with b[0] at filter_coef[FRAME_OVERLAP].
 */
const q4_28 filter_coef[TAP_COUNT+(2*(FRAME_OVERLAP))] = {
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // -256
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  // 0
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
  B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, B,B,B,B,B,B,B,B, // 1024 = TAP_COUNT
  // TAP_COUNT
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // TAP_COUNT + 32
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //TAP_COUNT + FRAME_OVERLAP
};
