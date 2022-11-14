
#include "common.h"

// Our original coefficient value was this
#define B_float   (0.0009765625)

// Converting that to fixed-point is a matter of scaling it up using the 
// intended exponent as:  ((int32_t)(round(ldexp(B_float, -coef_exp))))

// Unfortunately, we can't use that to initialize the coefficient array because
// it's not considered a compile time constant. The result of the above 
// expression is 0x40000.
#define B         (0x40000)

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
