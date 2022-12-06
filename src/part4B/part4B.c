
#include "common.h"

// The box filter coefficient array.
extern 
const q2_30 filter_coef[TAP_COUNT];


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
  // Exponent associated with input samples
  const exponent_t input_exp = -31;
  // Exponent associated with output samples
  const exponent_t output_exp = -31;
  // Exponent associatd with the filter coefficients
  const exponent_t coef_exp = -30;
  // Right-shift applied by the VPU for 32-bit multiplies
  const right_shift_t vpu_shr = 30;
  // Exponent associated with the accumulator
  const exponent_t acc_exp = input_exp + coef_exp + vpu_shr;

  // The arithmetic right-shift required by the `filter_fir_s32_t` object. This
  // is the number of bits by which the filter's accumulator will be
  // right-shifted to obtain the output.
  const right_shift_t acc_shr = output_exp - acc_exp;
  
  // Buffer used to hold filter state. We do not need to manage the filter state
  // ourselves, but we must give it a buffer. Initializing the filter does not
  // clear the filter state to zeros, so we must do that here.
  int32_t filter_state[TAP_COUNT] = {0};

  // The filter object itself
  filter_fir_s32_t fir_filter;
  
  // This buffer is where input/output samples will be placed.
  int32_t sample_buffer[FRAME_SIZE] = {0};

  // The filter needs to be initialized before supplying it with samples.
  filter_fir_s32_init(&fir_filter, 
                      &filter_state[0], 
                      TAP_COUNT, 
                      &filter_coef[0], 
                      acc_shr);

  // Loop forever
  while(1) {

    // Read in a new frame
    rx_frame(&sample_buffer[0], 
             c_audio);
    
    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start(TIMING_SAMPLE);
      // We can overwrite the data in sample_buffer[] because the filter object
      // will keep track of its own history. So, once we've supplied it with a
      // sample, we can overwrite that memory, allowing us to use the same array
      // for input and output.
      sample_buffer[s] = filter_fir_s32(&fir_filter, 
                                        sample_buffer[s]);
      timer_stop(TIMING_SAMPLE);
    }

    // Send out the processed frame
    tx_frame(c_audio, 
             &sample_buffer[0]);
  }
}
//// -filter_task