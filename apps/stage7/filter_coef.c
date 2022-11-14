
#include "common.h"

// Our original coefficient value was this
#define B_float   (0.0009765625)
#define B         (0x100000)

const int32_t filter_coef[TAP_COUNT] = {
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
