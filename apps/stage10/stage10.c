
#include "common.h"

// The exponent associated with the integer coefficients
const exponent_t coef_exp = -30;

// The filter coefficients
extern const int32_t filter_coef[TAP_COUNT];

// Filter samples in sample_buffer[] in-place
void filter_frame(
    filter_fir_s32_t* filter,
    int32_t sample_buffer[FRAME_OVERLAP])
{
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    // We can overwrite the data in sample_buffer[] because the filter object
    // will keep track of its own history. So, once we've supplied it with a
    // sample, we can overwrite that memory.
    sample_buffer[s] = filter_fir_s32(filter, sample_buffer[s]);
    timer_stop();
  }
}


void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{

  // This stage functions differently than those previous. The lib_xcore_math
  // filter_fir_s32_t object will manage the state of the FIR filter, so there's
  // no need to manage the history here. We will still read in a whole frame
  // overlap of samples before computing a result.
  
  // Buffer used to hold filter state. Initializing the filter does not clear
  // the filter state to zeros, so we must do that here.
  int32_t filter_state[TAP_COUNT] = {0};

  // The filter object itself
  filter_fir_s32_t fir_filter;
  
  // This buffer is where input/output samples will be placed.
  int32_t sample_buffer[FRAME_OVERLAP] = {0};

  // The filter needs to be initialized before supplying it with samples.
  filter_fir_s32_init(&fir_filter, &filter_state[0], 
                      TAP_COUNT, &filter_coef[0], -(30+coef_exp));


  while(1) {
    // Receive FRAME_OVERLAP samples and place them in the sample_buffer.
    for(int k = 0; k < FRAME_OVERLAP; k++)
      sample_buffer[k] = (int32_t) chan_in_word(c_pcm_in);
    
    // Process the samples
    filter_frame(&fir_filter, sample_buffer);

    // Send samples to output thread.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, sample_buffer[k]);
    }
  }
}