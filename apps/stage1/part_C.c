#include "common.h"

/**
 * The box filter coefficient array.
 */
extern
const float filter_coef[TAP_COUNT];


/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` contains the `TAP_COUNT` most-recent input samples, with
 * the newest samples first (reverse time order).
 * 
 * STAGE 0 implements this filter as a simple C loop computing the sum of 
 * products between the sample history and filter coefficients, using 
 * double-precision floating-point arithmetic.
 */
float filter_sample(
    const float sample_history[TAP_COUNT])
{
  // Return the inner product of sample_history[] and filter_coef[]
  return vect_f32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT);
}
