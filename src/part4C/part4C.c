
#include "common.h"

/**
 * The files userFilter.c and userFilter.h were generated using one of the FIR
 * filter conversion scripts from lib_xcore_math.
 */
#include "userFilter.h"


//// +rx_frame
// Accept a frame of new audio data 
static inline 
void rx_frame(
    int32_t buff[],
    const chanend_t c_audio)
{    
  for(int k = 0; k < FRAME_SIZE; k++)
    buff[k] = (q1_31) chan_in_word(c_audio);

  timer_start(TIMING_FRAME);
}
//// -rx_frame


//// +tx_frame
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    const int32_t buff[])
{    
  timer_stop(TIMING_FRAME);

  for(int k = 0; k < FRAME_SIZE; k++)
    chan_out_word(c_audio, buff[k]);
}
//// -tx_frame


//// +filter_task
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

    // Read in a new frame
    rx_frame(&sample_buffer[0], 
             c_audio);
    
    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start(TIMING_SAMPLE);
      // userFilter() is the generated function to add a new input sample and get
      // back the filtered result.
      sample_buffer[s] = userFilter(sample_buffer[s]);
      timer_stop(TIMING_SAMPLE);
    }

    // Send out the processed frame
    tx_frame(c_audio, 
             &sample_buffer[0]);
  }
}
//// -filter_task
