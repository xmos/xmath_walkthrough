
#include "common.h"

/**
 * The box filter coefficient array.
 */
extern 
const q2_30 filter_coef[TAP_COUNT];

/**
 * The exponent associatd with the filter coefficients.
 * 
 * The value represented by the k'th coefficient is 
 * `ldexp(filter_coef[k], coef_exp)`.
 */
const exponent_t coef_exp = -30;

/**
 * The exponent associated with the output signal.
 * 
 * This exponent implies 32-bit PCM samples represent a value in the range
 * `[-1.0, 1.0)`.
 */
const exponent_t output_exp = -31;

/**
 * Arithmetic right-shift applied to each element-wise product as the total
 * is accumulated.
 * 
 * When `p_shr` is equal to the number of fractional bits of the coefficient
 * (as it is here), the exponent associated with the product is simply the 
 * exponent of the other multiplicand.
 */
const right_shift_t p_shr = -coef_exp;


/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` contains the `TAP_COUNT` most-recent input samples, with
 * the newest samples first (reverse time order).
 * 
 * `history_exp` is the exponent associated with the samples in 
 * `sample_history[]`.
 * 
 * `history_hr` is the headroom present in `sample_history[]`.
 * 
 * STAGE 6 implements this filter using a block floating-point approach (in 
 * plain C).
 */
q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr)
{
  // The accumulator into which partial results are added.
  // This is a non-standard floating-point type with a 64-bit mantissa and
  // an exponent.
  float_s64_t acc;

  // The exponent associated with a product is the sum of exponents of the 
  // multiplicands. A right-shift of `p_shr` bits is applied to each product
  // before being added to the accumulator, and so is added to get the 
  // final accumulator exponent.
  acc.exp = (history_exp + coef_exp) + p_shr;
  acc.mant = 0;

  // Compute the inner product between the history and coefficient vectors.
  for(int k = 0; k < TAP_COUNT; k++){
    int32_t b = sample_history[k];
    int32_t c = filter_coef[k];
    int64_t p =  (((int64_t)b) * c);
    acc.mant += ashr64(p, -coef_exp);
  }

  // Much of the time in block floating-point, we don't require values to use
  // any particular exponent. However, because we must send 32-bit PCM samples
  // back to the host, we must ensure that all output samples are associated
  // with the same exponent. Otherwise the output will make no sense.
  // Here we convert our custom 64-bit float value to a 32-bit fixed-point
  // q1_31 value.
  q1_31 sample_out = float_s64_to_fixed(acc, output_exp);

  return sample_out;
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
 * 
 * `history_in_exp` is the block floating-point exponent associated with the
 * samples in `history_in[]`.
 * 
 * `history_in_hr` is the headroom of the `history_in[]` vector.
 */
void filter_frame(
    q1_31 frame_out[FRAME_OVERLAP],
    const int32_t history_in[FRAME_SIZE],
    const exponent_t history_in_exp,
    const headroom_t history_in_hr)
{
  // Compute FRAME_OVERLAP output samples.
  // Each output sample will use a TAP_COUNT-sample window of the input
  // history. That window slides over 1 element for each output sample.
  // A timer (100 MHz freqency) is used to measure how long each output sample
  // takes to process.
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    frame_out[s] = filter_sample(&history_in[FRAME_OVERLAP-s-1], 
                                  history_in_exp, 
                                  history_in_hr);
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
  // Exponent associated with frame_history[]
  exponent_t frame_history_exp;
  // Headroom associated with frame_history[]
  headroom_t frame_history_hr;

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

    // For now, the exponent associated with each new input frame is -31.
    frame_history_exp = -31;

    // Compute headroom of input frame
    frame_history_hr = 32;
    for(int k = 0; k < FRAME_SIZE; k++)
      frame_history_hr = MIN(frame_history_hr, HR_S32(frame_history[k]));

    // Apply the filter to the new frame of audio, producing FRAME_OVERLAP 
    // output samples in frame_output[].
    filter_frame(frame_output, frame_history, 
                 frame_history_exp, frame_history_hr);

    // Send FRAME_OVERLAP new output samples at the end of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, frame_output[k]);
    }

    // Finally, shift the frame_history[] buffer up FRAME_OVERLAP samples.
    // This is required to maintain ordering of the sample history.
    memmove(&frame_history[FRAME_OVERLAP], &frame_history[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}