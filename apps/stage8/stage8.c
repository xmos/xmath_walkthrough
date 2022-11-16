
#include "common.h"

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
 * Block floating-point vector representing the filter coefficients.
 */
bfp_s32_t bfp_filter_coef;


/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` is a BFP vector representing the `TAP_COUNT` history
 * samples needed to compute the current output.
 */
int32_t filter_sample(
    const bfp_s32_t* sample_history
    const exponent_t out_exp)
{
  // Compute the dot product of sample_history[] and bfp_filter_coef[].
  float_s64_t acc = bfp_s32_dot(sample_history, &bfp_filter_coef);
  
  // Convert the result to a fixed-point value using the output exponent
  int32_t sample_out = float_s64_to_fixed(acc, out_exp);

  return sample_out;
}

/**
 * Apply the filter to a frame with `FRAME_OVERLAP` new input samples, producing
 * one output sample for each new sample.
 * 
 * Computed output samples are placed into `frame_out[]` with the oldest samples
 * first (forward time order).
 * 
 * `history_in[]` is a BFP vector containing the most recent `FRAME_SIZE`
 * samples with the newest samples first (reverse time order). The first
 * `FRAME_OVERLAP` samples of `history_in[]` are new.
 */
void filter_frame(
    bfp_s32_t* frame_out,
    const bfp_s32_t* history_in)
{

  // We're faking an exponent here, just for the sake of demonstrating the BFP
  // behavior.
  frame_out->exp = output_exp+2;

  // Initialize a new BFP vector which is a 'view' onto a TAP_COUNT-element 
  // window of the history_in[] vector.
  // The sample_history[] window will 'slide' along history_in[] for each output
  // sample.
  bfp_s32_t sample_history;
  bfp_s32_init(&sample_history, &history_in->data[FRAME_OVERLAP],
                history_in->exp, TAP_COUNT, 0);
  // Might not be precisely correct, but is safe
  sample_history.hr = history_in->hr;

  // Compute FRAME_OVERLAP output samples.
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    // Slide the window down one index, towards newer samples
    sample_history.data = sample_history.data - 1;
    // Get next output sample
    frame_out->data[s] = filter_sample(&sample_history);
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

    // For now, the exponent associated with each new input frame is -31.
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
      // Put PCM sample in output channel
      chan_out_word(c_pcm_out, frame_output.data[k]);
    }

    // Finally, shift the frame_history[] buffer up FRAME_OVERLAP samples.
    // This is required to maintain ordering of the sample history.
    memmove(&frame_history.data[FRAME_OVERLAP], &frame_history.data[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
