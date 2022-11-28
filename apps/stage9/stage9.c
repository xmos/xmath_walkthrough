
#include "common.h"

/**
 * Apply the filter to produce a single output sample.
 * 
 * This function is implemented in stage9.xc so as to make use of the convenient
 * `par {}` syntax available in the XC language.
 */
q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr);

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
 * 
 * `history_in_exp` is the block floating-point exponent associated with the
 * samples in `history_in[]`.
 * 
 * `history_in_hr` is the headroom of the `history_in[]` vector.
 */
void filter_frame(
    q1_31 frame_out[FRAME_SIZE],
    const int32_t history_in[HISTORY_SIZE],
    const exponent_t history_in_exp,
    const headroom_t history_in_hr)
{
  // Compute FRAME_SIZE output samples.
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    frame_out[s] = filter_sample(&history_in[FRAME_SIZE-s-1], 
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
  int32_t sample_history[HISTORY_SIZE] = {0};
  // Exponent associated with sample_history[]
  exponent_t sample_history_exp;
  // Headroom associated with sample_history[]
  headroom_t sample_history_hr;

  // Buffer used to hold output samples.
  q1_31 frame_output[FRAME_SIZE] = {0};

  // Loop forever
  while(1) {
    // Receive FRAME_SIZE new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_SIZE; k++){
      // Read PCM sample from channel
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      // Place at beginning of history buffer in reverse order (to match the
      // order of filter coefficients).
      sample_history[FRAME_SIZE-k-1] = sample_in;
    }

    // For now, the exponent associated with each new input frame is -31.
    sample_history_exp = -31;

    // Compute headroom of input frame
    sample_history_hr = vect_s32_headroom(&sample_history[0], HISTORY_SIZE);

    // Apply the filter to the new frame of audio, producing FRAME_SIZE 
    // output samples in frame_output[].
    filter_frame(frame_output, sample_history, 
                 sample_history_exp, sample_history_hr);

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
