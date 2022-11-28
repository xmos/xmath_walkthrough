
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
 * This is the thread entry point for the hardware thread which will actually 
 * be applying the FIR filter.
 * 
 * `c_audio` is the channel over which PCM audio data is exchanged with tile[0].
 */
void filter_task(
    chanend_t c_audio)
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
    // Read in a new frame. It is placed in reverse order at the beginning of
    // sample_history[]
    read_frame(&sample_history[0], c_audio, FRAME_SIZE);

    // For now, the exponent associated with each new input frame is -31.
    sample_history_exp = -31;

    // Compute headroom of input frame
    sample_history_hr = vect_s32_headroom(&sample_history[0], HISTORY_SIZE);

    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start();
      frame_output[s] = filter_sample(&sample_history[FRAME_SIZE-s-1], 
                                      sample_history_exp, 
                                      sample_history_hr);
      timer_stop();
    }

    // Send out the processed frame
    send_frame(c_audio, &frame_output[0], FRAME_SIZE);

    // Finally, shift the sample_history[] buffer up FRAME_SIZE samples.
    // This is required to maintain ordering of the sample history.
    memmove(&sample_history[FRAME_SIZE], &sample_history[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
