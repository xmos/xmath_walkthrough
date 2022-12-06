
#include "common.h"

/**
 * The box filter coefficient array.
 */
extern 
const q4_28 filter_coef[TAP_COUNT];


//// +rx_frame
// Accept a frame of new audio data 
static inline 
void rx_frame(
    q1_31 buff[],
    const chanend_t c_audio)
{    
  for(int k = 0; k < FRAME_SIZE; k++)
    buff[FRAME_SIZE-k-1] = (q1_31) chan_in_word(c_audio);

  timer_start(TIMING_FRAME);
}
//// -rx_frame


//// +tx_frame
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    const q1_31 buff[])
{    
  timer_stop(TIMING_FRAME);
  
  for(int k = 0; k < FRAME_SIZE; k++)
    chan_out_word(c_audio, buff[k]);
}
//// -tx_frame


//// +int32_dot
/**
 * Computes the 64-bit inner product between two 32-bit integer vectors.
 * 
 * This function is implemented directly in assembly in `int32_dot.S`.
 * 
 * This function is optimized but only uses the scalar arithmetic unit, not the
 * VPU.
 */
int64_t int32_dot(
    const int32_t x[],
    const int32_t y[],
    const unsigned length);
//// -int32_dot


//// +filter_sample
//Apply the filter to produce a single output sample
q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  // The exponent associatd with the filter coefficients
  const exponent_t coef_exp = -28;
  // The exponent associated with the input signal
  const exponent_t input_exp = -31;
  // The exponent associated with the output signal
  const exponent_t output_exp = input_exp;
  // The exponent associated with the accumulator
  const exponent_t acc_exp = input_exp + coef_exp;
  // The arithmetic right-shift applied to the filter's accumulator to achieve the
  // correct output exponent
  const right_shift_t acc_shr = output_exp - acc_exp;

  // Compute the 64-bit inner product between the sample history and the filter
  // coefficients
  int64_t acc = int32_dot(&sample_history[0], 
                          &filter_coef[0], 
                          TAP_COUNT);

  // Apply a right-shift, dropping the bit-depth back down to 32 bits
  return ashr64(acc, 
                acc_shr);
}
//// -filter_sample


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
  // Buffer used for storing input sample history
  q1_31 sample_history[HISTORY_SIZE] = {0};

  // Buffer used to hold output samples
  q1_31 frame_output[FRAME_SIZE] = {0};

  // Loop forever
  while(1) {
    // Read in a new frame. It is placed in reverse order at the beginning of
    // sample_history[]
    rx_frame(&sample_history[0], 
             c_audio);

    // Compute FRAME_SIZE output samples
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start(TIMING_SAMPLE);
      frame_output[s] = filter_sample(&sample_history[FRAME_SIZE-s-1]);
      timer_stop(TIMING_SAMPLE);
    }

    // Make room for new samples at the front of the vector
    memmove(&sample_history[FRAME_SIZE], 
            &sample_history[0], 
            TAP_COUNT * sizeof(int32_t));

    // Send out the processed frame
    tx_frame(c_audio, 
             &frame_output[0]);
  }
}
//// -filter_task
