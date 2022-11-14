
#include "common.h"


int32_t filter_sample(
    int32_t sample_history[TAP_COUNT],
    exponent_t history_exp,
    headroom_t history_hr);


void filter_frame(
    int32_t frame_out[FRAME_OVERLAP],
    int32_t frame_in[FRAME_SIZE],
    const exponent_t frame_in_exp,
    const headroom_t frame_in_hr)
{
  // We've received FRAME_OVERLAP new samples which are in frame_in[] in 
  // reverse chronological order. We need to produce FRAME_OVERLAP output
  // samples.
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    frame_out[s] = filter_sample(&frame_in[FRAME_OVERLAP-s-1], 
                                  frame_in_exp, 
                                  frame_in_hr);
    timer_stop();
  }
}


void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  // This buffer stores the audio data received from the previous thread. It is
  // large enough to fit FRAME_OVERLAP samples plus TAP_COUNT historical samples
  // so the filter can be run on each of the newly received samples.
  int32_t frame_input[FRAME_SIZE] = {0};
  exponent_t frame_input_exp;
  headroom_t frame_input_hr;

  // This buffer is where output samples will be placed.
  int32_t frame_output[FRAME_OVERLAP] = {0};

  // Note that we will fill the frame_input[] buffer backwards so that coefficients
  // are stored with lowest lag first. frame_output[] are stored in forward-time
  // order.

  while(1) {
    // Receive FRAME_OVERLAP samples and place them in the frame buffer.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      frame_input[FRAME_OVERLAP-k-1] = sample_in;
    }

    // Here we're always considering the input exponent to be -31. This need
    // not always be the case, however.
    frame_input_exp = -31;

    // Compute headroom of input frame
    frame_input_hr = vect_s32_headroom(&frame_input[0], FRAME_SIZE);

    // Process the samples
    filter_frame(frame_output, frame_input, 
                 frame_input_exp, frame_input_hr);

    // Send samples to output thread.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, frame_output[k]);
    }

    // Shift the frame_input[] buffer up FRAME_OVERLAP samples
    memmove(&frame_input[FRAME_OVERLAP], &frame_input[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}

