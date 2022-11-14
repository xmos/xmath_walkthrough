
#include "common.h"

// The exponent associated with the integer coefficients
const exponent_t coef_exp = -28;

// The exponent associated with the input samples.
// Note that this is somewhat arbitrary. -31 is convenient because it means the
// samples are in the range [-1.0, 1.0)
const exponent_t input_exp = -31;

// We want the output exponent to be the same as the input exponent for this,
// so that the result is comparable to other stages.
const exponent_t output_exp = input_exp;

// The result will be accumulated in a 64-bit accumulator.
// The right-shift we must apply to that accumulator depends on the exponents
// we've chosen. Because input_exp == output_exp, those terms ultimately cancel
// out, and all that matters is the coefficient exponent.
const right_shift_t acc_shr = output_exp - (input_exp + coef_exp);

// The filter coefficients
extern const int32_t filter_coef[TAP_COUNT];

// This is the function with an optimized int32_t dot product.
int64_t int32_dot(
    const int32_t x[],
    const int32_t y[],
    const unsigned length);

int32_t filter_sample(
    int32_t sample_history[TAP_COUNT])
{
  int64_t acc = 0;
  acc = int32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT);
  return ashr64(acc, acc_shr);
}


void filter_frame(
    int32_t frame_out[FRAME_OVERLAP],
    int32_t frame_in[FRAME_SIZE])
{
  // We've received FRAME_OVERLAP new samples which are in frame_in[] in 
  // reverse chronological order. We need to produce FRAME_OVERLAP output
  // samples.
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    frame_out[s] = filter_sample(&frame_in[FRAME_OVERLAP-s-1]);
    timer_stop();
  }
}


void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  // This buffer stores the audio data received from the previous thread. It is
  // large enough to fit FRAME_OVERLAP samples plus TAP_COUNT historical samples
  // so the filter can be run on each of the newly received samples.
  int32_t frame_data[FRAME_SIZE] = {0};

  // This buffer is where output samples will be placed.
  int32_t frame_output[FRAME_OVERLAP] = {0};

  // Note that we will fill the frame_data[] buffer backwards so that coefficients
  // are stored with lowest lag first. frame_output[] are stored in forward-time
  // order.
  while(1) {

    // Receive FRAME_OVERLAP samples and place them in the frame buffer.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      frame_data[FRAME_OVERLAP-k-1] = sample_in;
    }

    // Process the samples
    filter_frame(frame_output, frame_data);

    // Send samples to output thread.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, frame_output[k]);
    }

    // Shift the frame_data[] buffer up FRAME_OVERLAP samples
    memmove(&frame_data[FRAME_OVERLAP], &frame_data[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
