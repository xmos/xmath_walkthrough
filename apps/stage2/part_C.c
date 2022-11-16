
#include "common.h"


/**
 * The box filter coefficient array.
 */
extern 
const q4_28 filter_coef[TAP_COUNT];

/**
 * The exponent associatd with the filter coefficients.
 * 
 * The value represented by the k'th coefficient is 
 * `ldexp(filter_coef[k], coef_exp)`.
 */
const exponent_t coef_exp = -28;

/**
 * The exponent associated with the input signal.
 * 
 * This exponent implies 32-bit PCM samples represent a value in the range
 * `[-1.0, 1.0)`.
 */
const exponent_t input_exp = -31;

/**
 * The exponent associated with the output signal.
 * 
 * This exponent implies 32-bit PCM samples represent a value in the range
 * `[-1.0, 1.0)`.
 */
const exponent_t output_exp = input_exp;

/**
 * The exponent associated with the accumulator.
 * 
 * The accumulator holds the 64-bit sum-of-products between the input history 
 * and filter coefficients. The associated exponents are `input_exp` and 
 * `coef_exp` respectively. The logical value represented by the `k`th term 
 * added to the accumulator is then:
 * 
 *      sample_history[k] * 2^(input_exp) * filter_coef[k] * 2^(coef_exp)
 *    = (sample_history[k] * filter_coef[k]) * 2^(input_exp + coef_exp)
 * 
 * And so `input_exp + coef_exp` is the exponent associated with the
 * accumulator. 
 */
const exponent_t acc_exp = input_exp + coef_exp + 30;

/**
 * Arithmetic right-shift applied to the filter's accumulator in order to 
 * achieve the correct output exponent.
 * 
 * The required shift is the desired exponent, `output_exp`, minus the current
 * exponent, `acc_exp`.
 */
const right_shift_t acc_shr = output_exp - acc_exp;


/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` contains the `TAP_COUNT` most-recent input samples, with
 * the newest samples first (reverse time order).
 * 
 * STAGE 3 implements this filter as a plain C loop, computing the sum of 
 * products between the sample history and filter coefficients and then shifting
 * it down to get the result.
 */
q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT])
{
  // Compute the 64-bit inner product between the sample history and the filter
  // coefficients using an optimized function from lib_xcore_math.
  int64_t acc = vect_s32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT, 0, 0);

  // Apply a right-shift, dropping the bit-depth back down to 32 bits.
  return ashr64(acc, acc_shr);
}