
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
 * Block floating-point vector representing the filter coefficients.
 */
bfp_s32_t bfp_filter_coef;


/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history` is a BFP vector representing the `TAP_COUNT` history samples
 * needed to compute the current output.
 * 
 * STAGE 8 implements this filter using lib_xcore_math's BFP API. The
 * function bfp_s32_dot() manages exponents, headroom and selects shift 
 * parameters for you, and uses the VPU-accelerated low-level API to actually
 * compute the result.
 */
float_s64_t filter_sample(
    const bfp_s32_t* sample_history)
{
  return bfp_s32_dot(sample_history, &bfp_filter_coef);
}

/**
 * Apply the filter to a frame with `FRAME_OVERLAP` new input samples, producing
 * one output sample for each new sample.
 * 
 * Computed output samples are placed into `frame_out` with the oldest samples
 * first (forward time order).
 * 
 * `history_in` is a BFP vector containing the most recent `FRAME_SIZE` samples
 * with the newest samples first (reverse time order). The first `FRAME_OVERLAP`
 * samples of `history_in` are new.
 */
void filter_frame(
    bfp_s32_t* frame_out,
    const bfp_s32_t* history_in)
{

  // We're going to use a fake output exponent here, just to force us to convert
  // samples to use the correct output exponent in the calling function. Note
  // that this will also effectively lose us two bits of precision compared to
  // using output_exp by itself.
  frame_out->exp = output_exp+2;

  // Compute FRAME_OVERLAP output samples.
  // Each output sample will use a TAP_COUNT-sample window of the input
  // history. That window slides over 1 element for each output sample.
  // A timer (100 MHz freqency) is used to measure how long each output sample
  // takes to process.
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    // We're doing something a little weird here. We're going to create a
    // 'virtual' BFP vector for the sample history for each output sample. This
    // isn't something you would typically need to do. 
    // The reason we need to do this is that the filter coefficient BFP vector
    // has length TAP_COUNT, but history_in has length `FRAME_SIZE`, which
    // makes them incompatible for `bfp_s32_dot()`. By creating this 'virtual'
    // vector pointing to a different offset into history_in each time (and by
    // reusing the headroom and exponent of history_in) we're supplying
    // filter_sample() with a usable BFP vector.
    bfp_s32_t sample_history;
    bfp_s32_init(&sample_history, &history_in->data[FRAME_OVERLAP-s-1],
                 history_in->exp, TAP_COUNT, 0);

    // This might not be precisely correct (in case the largest magnitude sample
    // is not included in this sub-frame), but it's guaranteed to be safe.
    sample_history.hr = history_in->hr;

    // In this case we happen to know a priori that each float_s64_t that comes
    // out of filter_sample() will have the same exponent, because the input
    // exponents and headroom don't change between samples. Normally we can't
    // count on that, however, so we'll convert each output sample to the 
    // output exponent so that we know they can share a BFP vector.
    float_s64_t smp_out_f64 = filter_sample(&sample_history);
    frame_out->data[s] = float_s64_to_fixed(smp_out_f64, frame_out->exp);
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

  // Initialize BFP vector representing filter coefficients
  bfp_s32_init(&bfp_filter_coef, (int32_t*) &filter_coef[0], 
               coef_exp, TAP_COUNT, 1);

  // Initialize BFP vector representing input frame
  int32_t frame_history_buff[FRAME_SIZE] = {0};
  bfp_s32_t frame_history;
  bfp_s32_init(&frame_history, &frame_history_buff[0], -31, FRAME_SIZE, 0);
  bfp_s32_set(&frame_history, 0, -31);

  // Initialize BFP vector representing output frame
  int32_t frame_output_buff[FRAME_OVERLAP] = {0};
  bfp_s32_t frame_output;
  bfp_s32_init(&frame_output, &frame_output_buff[0], 0, FRAME_OVERLAP, 0);

  // Loop forever
  while(1) {
    // Receive FRAME_OVERLAP new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      // Read PCM sample from channel
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      // Place at beginning of history buffer in reverse order (to match the
      // order of filter coefficients).
      frame_history.data[FRAME_OVERLAP-k-1] = sample_in;
    }

    // Here we're always considering the input exponent to be -31. This need
    // not always be the case, however.
    frame_history.exp = -31;

    // Compute headroom of input frame
    bfp_s32_headroom(&frame_history);

    // Apply the filter to the new frame of audio, producing FRAME_OVERLAP 
    // output samples in frame_output.
    filter_frame(&frame_output, &frame_history);

    // The output samples MUST all use the same exponent (for every frame) in
    // order for the sample in the output wav file make sense. However, in 
    // general we shouldn't assume a BFP vector set by a callee has the exponent
    // we expect, so we'll force it to the desired exponent.
    bfp_s32_use_exponent(&frame_output, output_exp);

    // Send FRAME_OVERLAP new output samples at the end of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, frame_output.data[k]);
    }

    // Finally, shift the frame_history[] buffer up FRAME_OVERLAP samples.
    // This is required to maintain ordering of the sample history.
    memmove(&frame_history.data[FRAME_OVERLAP], &frame_history.data[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}