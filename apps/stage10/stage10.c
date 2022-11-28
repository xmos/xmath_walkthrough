
#include "common.h"

/**
 * The box filter coefficient array.
 */
extern 
const q2_30 filter_coef[TAP_COUNT];

/**
 * The exponent associatd with the filter coefficients.
 */
const exponent_t coef_exp = -30;

/**
 * The arithmetic right-shift required by the `filter_fir_s32_t` object. This
 * is the number of bits by which the filter's accumulator will be right-shifted
 * to obtain the output. The 30 is due to the VPU's 30-bit right-shift when
 * multiplying 32-bit operands.
 */
const right_shift_t filter_shr = -(coef_exp + 30);


/**
 * Apply the filter to a frame with `FRAME_SIZE` new input samples, producing
 * one output sample for each new sample.
 * 
 * `filter` is the `filter_fir_s32_t` object representing the filter 
 * coefficients and state.
 * 
 * When called, `sample_buffer[]` contains the new input samples with the oldest
 * samples first (forward time order). Then `sample_buffer[]` is used to return
 * the output samples, also in forward time order.
 */
void filter_frame(
    filter_fir_s32_t* filter,
    int32_t sample_buffer[FRAME_SIZE])
{
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    // We can overwrite the data in sample_buffer[] because the filter object
    // will keep track of its own history. So, once we've supplied it with a
    // sample, we can overwrite that memory, allowing us to use the same array
    // for input and output.
    sample_buffer[s] = filter_fir_s32(filter, sample_buffer[s]);
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
  
  // Buffer used to hold filter state. We do not need to manage the filter state
  // ourselves, but we must give it a buffer. Initializing the filter does not
  // clear the filter state to zeros, so we must do that here.
  int32_t filter_state[TAP_COUNT] = {0};

  // The filter object itself
  filter_fir_s32_t fir_filter;
  
  // This buffer is where input/output samples will be placed.
  int32_t sample_buffer[FRAME_SIZE] = {0};

  // The filter needs to be initialized before supplying it with samples.
  filter_fir_s32_init(&fir_filter, &filter_state[0], 
                      TAP_COUNT, &filter_coef[0], filter_shr);

  // Loop forever
  while(1) {
    // Receive FRAME_SIZE new input samples at the beginning of each frame.
    for(int k = 0; k < FRAME_SIZE; k++)
      sample_buffer[k] = (int32_t) chan_in_word(c_pcm_in);
    
    // Process the samples
    filter_frame(&fir_filter, sample_buffer);

    // Send FRAME_SIZE new output samples at the end of each frame.
    for(int k = 0; k < FRAME_SIZE; k++){
      chan_out_word(c_pcm_out, sample_buffer[k]);
    }
  }
}