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
  // The filter result is the simple inner product of the sample history and the
  // filter coefficients. Because we've stored the sample history in reverse
  // time order, the indices of sample_history[] match up with the indices of
  // filter_coef[].
  float acc = 0.0;
  for(int k = 0; k < TAP_COUNT; k++)
    acc += sample_history[k] * filter_coef[k];
  return acc;
}
