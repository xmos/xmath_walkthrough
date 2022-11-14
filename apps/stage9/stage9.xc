
#include "common.h"

// The exponent associated with the integer coefficients
const exponent_t coef_exp = -30;

// We want the output exponent to be the same as the input exponent for this,
// so that the result is comparable to other stages.
const exponent_t output_exp = -31;

// The filter coefficients
extern const int32_t filter_coef[TAP_COUNT];

#define THREADS     4

int32_t filter_sample(
    int32_t sample_history[TAP_COUNT],
    exponent_t history_exp,
    headroom_t history_hr)
{
  float_s64_t a;
  right_shift_t b_shr, c_shr;
  headroom_t coef_hr = HR_S32(filter_coef[0]);

  vect_s32_dot_prepare(&a.exp, &b_shr, &c_shr, 
                        history_exp, coef_exp,
                        history_hr, coef_hr, TAP_COUNT);

  int64_t partial[THREADS];
  const unsigned elms = (TAP_COUNT / THREADS);

  // Here's where it actually goes parallel to compute partial results
  par(int tid = 0; tid < THREADS; tid++){
    partial[tid] = vect_s32_dot(&sample_history[elms * tid],
                                &filter_coef[elms * tid],
                                elms, b_shr, c_shr);
  }
  // The above par block joins on the threads that were started and will resume
  // when each thread completes.

  // Sum partial results for final result.
  a.mant = 0;
  for(int k = 0; k < 4; k++)
    a.mant += partial[k];

  // We need the output to have the same exponent every time or else the 
  // contents of the wav file won't make any sense.
  int32_t sample_out = float_s64_to_fixed(a, output_exp);
  return sample_out;
}

