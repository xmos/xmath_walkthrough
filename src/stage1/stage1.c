
#include "common.h"

/**
 * The box filter coefficient array.
 */
extern
const float filter_coef[TAP_COUNT];

/**
 * The exponent associated with the input signal.
 */
const exponent_t input_exp = -31;

/**
 * The exponent associated with the output signal.
 */
const exponent_t output_exp = input_exp;

/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` contains the `TAP_COUNT` most-recent input samples, with
 * the newest samples first (reverse time order).
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
  float sample_history[HISTORY_SIZE] = {0};

  // Buffer used to hold output samples.
  float frame_output[FRAME_SIZE] = {0};

  // Loop forever
  while(1) {
    // Read in a new frame. It is placed in reverse order at the beginning of
    // sample_history[]
    read_frame_as_float(&sample_history[0], c_audio, input_exp, FRAME_SIZE);

    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start();
      frame_output[s] = filter_sample(&sample_history[FRAME_SIZE-s-1]);
      timer_stop();
    }

    // Send out the processed frame
    send_frame_as_float(c_audio, &frame_output[0], output_exp, FRAME_SIZE);

    // Finally, shift the sample_history[] buffer up FRAME_SIZE samples.
    // This is required to maintain ordering of the sample history.
    memmove(&sample_history[FRAME_SIZE], &sample_history[0], 
            TAP_COUNT * sizeof(float));
  }
}
