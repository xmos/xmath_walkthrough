
#include "common.h"

/**
 * The box filter coefficient array.
 */
extern 
const q4_28 filter_coef[TAP_COUNT];

/**
 * The exponent associatd with the filter coefficients.
 */
const exponent_t coef_exp = -28;

/**
 * The exponent associated with the input signal.
 */
const exponent_t input_exp = -31;

/**
 * The exponent associated with the output signal.
 */
const exponent_t output_exp = input_exp;

/**
 * The exponent associated with the accumulator.
 */
const exponent_t acc_exp = input_exp + coef_exp;

/**
 * The arithmetic right-shift applied to the filter's accumulator to achieve the
 * correct output exponent.
 */
const right_shift_t acc_shr = output_exp - acc_exp;

/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` contains the `TAP_COUNT` most-recent input samples, with
 * the newest samples first (reverse time order).
 */
q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  int64_t acc = 0;

  // For each filter tap, add the 64-bit product to the accumulator
  for(int k = 0; k < TAP_COUNT; k++){
    const int64_t smp = sample_history[k];
    const int64_t coef = filter_coef[k];
    acc += (smp * coef);
  }

  // Apply a right-shift, dropping the bit-depth back down to 32 bits.
  return ashr64(acc, acc_shr);
}

/**
 * This is the thread entry point for the hardware thread which will actually 
 * be applying the FIR filter.
 * 
 * `c_audio` is the channel over which PCM audio data is exchanged with tile[0].
 */
void filter_task(
    chanend_t c_audio)
{
  // Buffer used for storing input sample history.
  q1_31 sample_history[HISTORY_SIZE] = {0};

  // Buffer used to hold output samples.
  q1_31 frame_output[FRAME_SIZE] = {0};

  // Loop forever
  while(1) {
    // Read in a new frame. It is placed in reverse order at the beginning of
    // sample_history[]
    read_frame(&sample_history[0], c_audio, FRAME_SIZE);

    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start();
      frame_output[s] = filter_sample(&sample_history[FRAME_SIZE-s-1]);
      timer_stop();
    }

    // Send out the processed frame
    send_frame(c_audio, &frame_output[0], FRAME_SIZE);

    // Finally, shift the sample_history[] buffer up FRAME_SIZE samples.
    // This is required to maintain ordering of the sample history.
    memmove(&sample_history[FRAME_SIZE], &sample_history[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
