
#include "common.h"
#include "userFilter.h"


// Filter samples in sample_buffer[] in-place
void filter_frame(
    int32_t sample_buffer[FRAME_OVERLAP])
{
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    sample_buffer[s] = userFilter(sample_buffer[s]);
    timer_stop();
  }
}


void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  // This buffer is where input/output samples will be placed.
  int32_t sample_buffer[FRAME_OVERLAP] = {0};

  // Initialize the userFilter. 
  userFilter_init();
  
  // If userFilter_exp_diff is not 0, the results will be wrong.
  assert(userFilter_exp_diff == 0);

  while(1) {
    // Receive FRAME_OVERLAP samples and place them in the sample_buffer.
    for(int k = 0; k < FRAME_OVERLAP; k++)
      sample_buffer[k] = (int32_t) chan_in_word(c_pcm_in);
    
    // Process the samples
    filter_frame(sample_buffer);

    // Send samples to output thread.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, sample_buffer[k]);
    }
  }
}
