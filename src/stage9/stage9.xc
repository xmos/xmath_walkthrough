
#include "common.h"

/**
 * The number of threads that will be used to compute the result.
 */
#define THREADS     4

/**
 * The box filter coefficient array.
 */
extern 
const q2_30 filter_coef[TAP_COUNT];

/**
 * The exponent associatd with the filter coefficients.
 */
const exponent_t coef_exp = -30;

/**
 * The exponent associated with the output signal.
 */
const exponent_t output_exp = -31;


/**
 * Apply the filter to produce a single output sample.
 * 
 * 
 * This function is implemented in stage9.xc so as to make use of the convenient
 * `par {}` syntax available in the XC language.
 * 
 * In this case, the work will be divided by index range of the BFP vectors.
 * Because there are 1024 taps and 4 threads, the first worker thread will 
 * compute the dot product of the first 256 elements of the two vectors, the
 * second will compute the dot product of elements 256-512, etc.
 * 
 * NOTE: In the comments below I refer to "master thread" and "worker threads".
 *       To avoid confusion I should specify what I'm calling the "master 
 *       thread" is the hardware thread on which this function was called, and 
 *       while in the context of the 'par' block below, the master thread is
 *       also used as one of the 4 worker threads.
 */
q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr)
{
  // The accumulator into which partial results are added.
  float_s64_t acc;

  // The headroom of the filter_coef[] vector. Used by vect_s32_dot_prepare().
  headroom_t coef_hr = HR_S32(filter_coef[0]);

  // vect_s32_dot_prepare() takes in the exponent and headroom of both input 
  // vectors and the length of the vectors and gives us 3 things:
  //  - The exponent that will be associated with the output of vect_s32_dot()
  //  - The b_shr and c_shr shift parameters required by vect_s32_dot().
  // vect_s32_dot_prepare() chooses these three values so as to maximize the
  // precision of the result while avoiding overflows on the inputs or output.
  // We execute this in the master thread prior to spawning the worker threads
  // because the worker threads will each make use of the shift parameters.
  right_shift_t b_shr, c_shr;
  vect_s32_dot_prepare(&acc.exp, &b_shr, &c_shr, 
                        history_exp, coef_exp,
                        history_hr, coef_hr, TAP_COUNT);

  // The worker threads will place their portion of the sum into the appropriate
  // `partial` bin here. In order to avoid race conditions we'll join on the 
  // worker threads and wait to add the partial results together until we've
  // resumed in the master thread.
  int64_t partial[THREADS];

  // The number of elements being handled by each thread.
  const unsigned elms = (TAP_COUNT / THREADS);

  // Here's it actually goes parallel to compute partial results.
  par(int tid = 0; tid < THREADS; tid++)
  {
    partial[tid] = vect_s32_dot(&sample_history[elms * tid],
                                &filter_coef[elms * tid],
                                elms, b_shr, c_shr);
  }
  // The above par block joins on the threads that were started and will resume
  // when each thread completes.

  // Sum partial results for final result.
  acc.mant = 0;
  for(int k = 0; k < 4; k++)
    acc.mant += partial[k];


  // Convert the result to a fixed-point value using the output exponent
  q1_31 sample_out = float_s64_to_fixed(acc, output_exp);

  return sample_out;
}

