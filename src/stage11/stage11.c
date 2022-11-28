
#include "common.h"

/**
 * The files userFilter.c and userFilter.h were generated using one of the FIR
 * filter conversion scripts from lib_xcore_math.
 */
#include "userFilter.h"


/**
 * This is the thread entry point for the hardware thread which will actually 
 * be applying the FIR filter.
 * 
 * `c_audio` is the channel over which PCM audio data is exchanged with tile[0].
 */
void filter_task(
    chanend_t c_audio)
{
  // This buffer is where input/output samples will be placed.
  int32_t sample_buffer[FRAME_SIZE] = {0};

  // Initialize userFilter. userFilter allocates and manages its own buffers and
  // filter object, so no buffer needs to be supplied.
  userFilter_init();
  
  // If userFilter_exp_diff is not 0, the results will be wrong.
  assert(userFilter_exp_diff == 0);

  // Loop forever
  while(1) {
    // Receive FRAME_SIZE new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_SIZE; k++)
      sample_buffer[k] = (int32_t) chan_in_word(c_audio);
    
    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start();
      // userFilter() is the generated function to add a new input sample and get
      // back the filtered result.
      sample_buffer[s] = userFilter(sample_buffer[s]);
      timer_stop();
    }

    // Send out the processed frame
    send_frame(c_audio, &sample_buffer[0], FRAME_SIZE);
  }
}
