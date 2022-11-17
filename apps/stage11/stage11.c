
#include "common.h"

/**
 * The files userFilter.c and userFilter.h were generated using one of the FIR
 * filter conversion scripts from lib_xcore_math.
 */
#include "userFilter.h"


/**
 * Apply the filter to a frame with `FRAME_OVERLAP` new input samples, producing
 * one output sample for each new sample.
 * 
 * When called, `sample_buffer[]` contains the new input samples with the oldest
 * samples first (forward time order). Then `sample_buffer[]` is used to return
 * the output samples, also in forward time order.
 */
void filter_frame(
    int32_t sample_buffer[FRAME_OVERLAP])
{
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    // userFilter() is the generated function to add a new input sample and get
    // back the filtered result.
    sample_buffer[s] = userFilter(sample_buffer[s]);
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
  // This buffer is where input/output samples will be placed.
  int32_t sample_buffer[FRAME_OVERLAP] = {0};

  // Initialize userFilter. userFilter allocates and manages its own buffers and
  // filter object, so no buffer needs to be supplied.
  userFilter_init();
  
  // If userFilter_exp_diff is not 0, the results will be wrong.
  assert(userFilter_exp_diff == 0);

  // Loop forever
  while(1) {
    // Receive FRAME_OVERLAP new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++)
      sample_buffer[k] = (int32_t) chan_in_word(c_pcm_in);
    
    // Process the samples
    filter_frame(sample_buffer);

    // Send FRAME_OVERLAP new output samples at the end of each frame.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, sample_buffer[k]);
    }
  }
}
