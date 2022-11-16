
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
 * Apply the filter to a frame with `FRAME_OVERLAP` new input samples, producing
 * one output sample for each new sample.
 * 
 * Computed output samples are placed into `frame_out[]` with the oldest samples
 * first (forward time order).
 * 
 * `history_in[]` contains the most recent `FRAME_SIZE` samples with the newest
 * samples first (reverse time order). The first `FRAME_OVERLAP` samples of 
 * `history_in[]` are new.
 */
void filter_frame(
    float frame_out[FRAME_OVERLAP],
    const float history_in[FRAME_SIZE])
{
  // Compute FRAME_OVERLAP output samples.
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    frame_out[s] = filter_sample(&history_in[FRAME_OVERLAP-s-1]);
    timer_stop();
  }
}

/**
 * This is the thread entry point for the hardware thread which will actually 
 * be applying the FIR filter.
 * 
 * `c_pcm_in` is the channel from which PCM input samples are received.
 * 
 * `c_pcm_out` is the channel to which PCM output samples are sent.
 */
void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  // Buffer used for storing input sample history.
  float frame_history[FRAME_SIZE] = {0};

  // Buffer used to hold output samples.
  float frame_output[FRAME_OVERLAP] = {0};

  // Loop forever
  while(1) {
    // Receive FRAME_OVERLAP new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      // Read PCM sample from channel
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      // Convert PCM sample to floating-point
      const float samp_f = ldexpf(sample_in, input_exp);
      // Place at beginning of history buffer in reverse order (to match the
      // order of filter coefficients).
      frame_history[FRAME_OVERLAP-k-1] = samp_f;
    }

    // Apply the filter to the new frame of audio, producing FRAME_OVERLAP 
    // output samples in frame_output[].
    filter_frame(frame_output, frame_history);

    // Send FRAME_OVERLAP new output samples at the end of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      // Get float sample from frame output buffer (in forward order)
      const float samp_f = frame_output[k];
      // Convert float sample back to PCM using the output exponent.
      const q1_31 sample_out = roundf(ldexpf(samp_f, -output_exp));
      // Put PCM sample in output channel
      chan_out_word(c_pcm_out, sample_out);
    }

    // Finally, shift the frame_history[] buffer up FRAME_OVERLAP samples.
    // This is required to maintain ordering of the sample history.
    memmove(&frame_history[FRAME_OVERLAP], &frame_history[0], 
            TAP_COUNT * sizeof(float));
  }
}
