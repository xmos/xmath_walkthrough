
#include "common.h"

// The exponent associated with the integer coefficients
const exponent_t coef_exp = -30;

// We want the output exponent to be the same as the input exponent for this,
// so that the result is comparable to other stages.
const exponent_t output_exp = -31;

// The filter coefficients
extern const int32_t filter_coef[TAP_COUNT];

// BFP vector representing filter coefficients
bfp_s32_t bfp_filter_coef;

float_s64_t filter_sample(
    bfp_s32_t* sample_history)
{
  return bfp_s32_dot(sample_history, &bfp_filter_coef);
}


void filter_frame(
    bfp_s32_t* frame_out,
    bfp_s32_t* frame_in)
{
  // We've received FRAME_OVERLAP new samples which are in frame_in[] in 
  // reverse chronological order. We need to produce FRAME_OVERLAP output
  // samples.

  // We're going to use a fake output exponent here, which will require us to
  // convert samples to use the correct output exponent in the calling function.
  // Note that this will also effectively lose us two bits of precision compared
  // to using output_exp by itself.
  frame_out->exp = output_exp+2;

  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    // We're going to do something a little weird here, just to demonstrate the
    // BFP functions. We're going to create a 'virtual' BFP vector for the
    // sample history for each output sample. This isn't something you would
    // normally need to do.
    bfp_s32_t sample_history;
    bfp_s32_init(&sample_history, &frame_in->data[FRAME_OVERLAP-s-1],
                 frame_in->exp, TAP_COUNT, 0);

    // This might not be precisely correct (in case the largest magnitude sample
    // is not included in this sub-frame), but it's guaranteed to be safe.
    sample_history.hr = frame_in->hr;

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



void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{

  // Initialize BFP vector representing filter coefficients
  bfp_s32_init(&bfp_filter_coef, (int32_t*) &filter_coef[0], 
               coef_exp, TAP_COUNT, 1);

  // Initialize BFP vector representing input frame
  int32_t frame_input_buff[FRAME_SIZE] = {0};
  bfp_s32_t frame_input;
  bfp_s32_init(&frame_input, &frame_input_buff[0], -31, FRAME_SIZE, 0);
  bfp_s32_set(&frame_input, 0, -31);

  // Initialize BFP vector representing output frame
  int32_t frame_output_buff[FRAME_OVERLAP] = {0};
  bfp_s32_t frame_output;
  bfp_s32_init(&frame_output, &frame_output_buff[0], 0, FRAME_OVERLAP, 0);


  while(1) {
    // Receive FRAME_OVERLAP samples and place them in the frame buffer.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      frame_input.data[FRAME_OVERLAP-k-1] = sample_in;
    }

    // Here we're always considering the input exponent to be -31. This need
    // not always be the case, however.
    frame_input.exp = -31;

    // Compute headroom of input frame
    bfp_s32_headroom(&frame_input);

    // Process the samples
    filter_frame(&frame_output, &frame_input);

    // The output samples MUST all use the same exponent (for every frame) in
    // order for the sample in the output wav file make sense. However, in 
    // general we shouldn't assume a BFP vector set by a callee has the exponent
    // we expect, so we'll force it to the desired exponent.
    bfp_s32_use_exponent(&frame_output, output_exp);

    // Send samples to output thread.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, frame_output.data[k]);
    }

    // Shift the frame_input[] buffer up FRAME_OVERLAP samples
    memmove(&frame_input.data[FRAME_OVERLAP], &frame_input.data[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}