
#include "common.h"

extern 
const q2_30 filter_coef[TAP_COUNT];


// Block floating-point vector representing the filter coefficients.
bfp_s32_t bfp_filter_coef;

//// +calc_headroom 
// Compute headroom of int32 vector.
static inline
headroom_t calc_headroom(
    bfp_s32_t* vec)
{
  return bfp_s32_headroom(vec);
}
//// -calc_headroom


//// +rx_frame
static inline 
void rx_frame(
    bfp_s32_t* frame_in,
    const chanend_t c_audio)
{
  // We happen to know a priori that samples coming in will have a fixed 
  // exponent of input_exp, and there's no reason to change it, so we'll just
  // use that.
  frame_in->exp = -31;

  for(int k = 0; k < FRAME_SIZE; k++)
    frame_in->data[k] = chan_in_word(c_audio);

  timer_start(TIMING_FRAME);
  
  // Make sure the headroom is correct
  calc_headroom(frame_in);
}
//// -rx_frame


//// +rx_and_merge_frame
// Accept a frame of new audio data and merge it into sample history
static inline 
void rx_and_merge_frame(
    bfp_s32_t* sample_history,
    const chanend_t c_audio)
{    
  // BFP vector into which new frame will be placed.
  int32_t frame_in_buff[FRAME_SIZE];
  bfp_s32_t frame_in;
  bfp_s32_init(&frame_in, frame_in_buff, 0, FRAME_SIZE, 0);

  // Accept a new input frame
  rx_frame(&frame_in, c_audio);

  // Rescale BFP vectors if needed so they can be merged
  const exponent_t min_frame_in_exp = frame_in.exp - frame_in.hr;
  const exponent_t min_history_exp = sample_history->exp - sample_history->hr;
  const exponent_t new_exp = MAX(min_frame_in_exp, min_history_exp);

  bfp_s32_use_exponent(sample_history, new_exp);
  bfp_s32_use_exponent(&frame_in, new_exp);
  
  // Now we can merge the new frame in (reversing order)
  for(int k = 0; k < FRAME_SIZE; k++)
    sample_history->data[FRAME_SIZE-1-k] = frame_in.data[k];

  // And just ensure the headroom is correct
  calc_headroom(sample_history);
}
//// -rx_and_merge_frame


//// +tx_frame
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    bfp_s32_t* frame_out)
{
  const exponent_t output_exp = -31;
  
  // The output channel is expecting PCM samples with a *fixed* exponent of
  // output_exp, so we'll need to convert samples to use the correct exponent
  // before sending.
  bfp_s32_use_exponent(frame_out, output_exp);

  timer_stop(TIMING_FRAME);

  // And send the samples
  for(int k = 0; k < FRAME_SIZE; k++)
    chan_out_word(c_audio, frame_out->data[k]);
  
}
//// -tx_frame


//// +filter_sample
/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` is a BFP vector representing the `TAP_COUNT` history
 * samples needed to compute the current output.
 */
float_s64_t filter_sample(
    const bfp_s32_t* sample_history)
{
  // Compute the dot product
  return bfp_s32_dot(sample_history, &bfp_filter_coef);
}
//// -filter_sample


//// +filter_frame
// Calculate entire output frame
void filter_frame(
    bfp_s32_t* frame_out,
    const bfp_s32_t* sample_history)
{ 
  // Initialize a new BFP vector which is a 'view' onto a TAP_COUNT-element
  // window of the sample_history[] vector. The history_view[] window will
  // 'slide' along sample_history[] for each output sample. This isn't something
  // you'd typically need to do.
  bfp_s32_t history_view;
  bfp_s32_init(&history_view, &sample_history->data[FRAME_SIZE],
                sample_history->exp, TAP_COUNT, 0);
  history_view.hr = sample_history->hr; // Might not be precisely correct, but is safe

  // Compute FRAME_SIZE output samples.
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start(TIMING_SAMPLE);
    // Slide the window down one index, towards newer samples
    history_view.data = history_view.data - 1;
    // Get next output sample
    float_s64_t samp = filter_sample(&history_view);

    // Because the exponents and headroom are the same for every call to
    // filter_sample(), the output exponent will also be the same (it's computed
    // before the dot product is computed). So we'll use whichever exponent the 
    // first result says to use... plus 8, just like in the previous stage, for
    // the same reason (int40 -> int32)
    if(!s)
      frame_out->exp = samp.exp + 8;

    frame_out->data[s] = float_s64_to_fixed(samp, 
                                            frame_out->exp);
    timer_stop(TIMING_SAMPLE);
  }
}
//// -filter_frame


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

  // Initialize BFP vector representing filter coefficients
  const exponent_t coef_exp = -30;
  bfp_s32_init(&bfp_filter_coef, (int32_t*) &filter_coef[0], 
               coef_exp, TAP_COUNT, 1);

  // Represents the sample history as a BFP vector
  int32_t sample_history_buff[HISTORY_SIZE] = {0};
  bfp_s32_t sample_history;
  bfp_s32_init(&sample_history, &sample_history_buff[0], -200, 
               HISTORY_SIZE, 0);

  // Represents output frame as a BFP vector
  int32_t frame_output_buff[FRAME_SIZE] = {0};
  bfp_s32_t frame_output;
  bfp_s32_init(&frame_output, &frame_output_buff[0], 0, 
               FRAME_SIZE, 0);

  // Loop forever
  while(1) {

    // Read in a new frame
    rx_and_merge_frame(&sample_history, 
                       c_audio);

    // Calc output frame
    filter_frame(&frame_output, 
                 &sample_history);

    // Make room for new samples at the front of the vector.
    memmove(&sample_history.data[FRAME_SIZE], 
            &sample_history.data[0], 
            TAP_COUNT * sizeof(int32_t));

    // Send out the processed frame
    tx_frame(c_audio,
             &frame_output);
  }
}
//// -filter_task
