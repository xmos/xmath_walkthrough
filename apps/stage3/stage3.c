
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
const exponent_t acc_exp = input_exp + coef_exp;

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
  // Initialize accumulator to zero
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
    q1_31 frame_out[FRAME_OVERLAP],
    const int32_t history_in[FRAME_SIZE])
{
  // Compute FRAME_OVERLAP output samples.
  // Each output sample will use a TAP_COUNT-sample window of the input
  // history. That window slides over 1 element for each output sample.
  // A timer (100 MHz freqency) is used to measure how long each output sample
  // takes to process.
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
  int32_t frame_history[FRAME_SIZE] = {0};

  // Buffer used to hold output samples.
  q1_31 frame_output[FRAME_OVERLAP] = {0};

  // Loop forever
  while(1) {
    // Receive FRAME_OVERLAP new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      // Read PCM sample from channel
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      // Place at beginning of history buffer in reverse order (to match the
      // order of filter coefficients).
      frame_history[FRAME_OVERLAP-k-1] = sample_in;
    }

    // Apply the filter to the new frame of audio, producing FRAME_OVERLAP 
    // output samples in frame_output[].
    filter_frame(frame_output, frame_history);

    // Send FRAME_OVERLAP new output samples at the end of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      // Put PCM sample in output channel
      chan_out_word(c_pcm_out, frame_output[k]);
    }

    // Finally, shift the frame_history[] buffer up FRAME_OVERLAP samples.
    // This is required to maintain ordering of the sample history.
    memmove(&frame_history[FRAME_OVERLAP], &frame_history[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
