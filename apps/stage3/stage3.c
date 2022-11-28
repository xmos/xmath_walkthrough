
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
 * Apply the filter to a frame with `FRAME_SIZE` new input samples, producing
 * one output sample for each new sample.
 * 
 * Computed output samples are placed into `frame_out[]` with the oldest samples
 * first (forward time order).
 * 
 * `history_in[]` contains the most recent `HISTORY_SIZE` samples with the newest
 * samples first (reverse time order). The first `FRAME_SIZE` samples of 
 * `history_in[]` are new.
 */
void filter_frame(
    q1_31 frame_out[FRAME_SIZE],
    const q1_31 history_in[HISTORY_SIZE])
{
  // Compute FRAME_SIZE output samples.
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    frame_out[s] = filter_sample(&history_in[FRAME_SIZE-s-1]);
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
  q1_31 sample_history[HISTORY_SIZE] = {0};

  // Buffer used to hold output samples.
  q1_31 frame_output[FRAME_SIZE] = {0};

  // Loop forever
  while(1) {
    // Receive FRAME_SIZE new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_SIZE; k++){
      // Read PCM sample from channel
      const q1_31 sample_in = (q1_31) chan_in_word(c_pcm_in);
      // Place at beginning of history buffer in reverse order (to match the
      // order of filter coefficients).
      sample_history[FRAME_SIZE-k-1] = sample_in;
    }

    // Apply the filter to the new frame of audio, producing FRAME_SIZE 
    // output samples in frame_output[].
    filter_frame(frame_output, sample_history);

    // Send FRAME_SIZE new output samples at the end of each frame.
    for(int k = 0; k < FRAME_SIZE; k++){
      // Put PCM sample in output channel
      chan_out_word(c_pcm_out, frame_output[k]);
    }

    // Finally, shift the sample_history[] buffer up FRAME_SIZE samples.
    // This is required to maintain ordering of the sample history.
    memmove(&sample_history[FRAME_SIZE], &sample_history[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
