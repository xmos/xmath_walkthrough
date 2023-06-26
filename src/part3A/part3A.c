// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "common.h"

extern 
const q2_30 filter_coef[TAP_COUNT];


// Represents the filter coefficients as a BFP vector
struct {
  int32_t* data;
  exponent_t exp;
  headroom_t hr; 
} filter_bfp = {(int32_t*) &filter_coef[0], -30, 10};


//// +calc_headroom
// Compute headroom of int32 vector.
static inline
headroom_t calc_headroom(
    const int32_t vec[],
    const unsigned length)
{
  // An int32 cannot have more than 32 bits of headroom.
  headroom_t hr = 32;
  for(int k = 0; k < length; k++)
    hr = MIN(hr, HR_S32(vec[k]));
  return hr;
}
//// -calc_headroom


//// +rx_frame
// Accept a frame of new audio data 
static inline 
void rx_frame(
    int32_t frame_in[FRAME_SIZE],
    exponent_t* frame_in_exp,
    headroom_t* frame_in_hr,
    const chanend_t c_audio)
{
  // We happen to know a priori that samples coming in will have a fixed 
  // exponent of input_exp, and there's no reason to change it, so we'll just
  // use that.
  *frame_in_exp = -31;

  for(int k = 0; k < FRAME_SIZE; k++)
    frame_in[k] = chan_in_word(c_audio);

  timer_start(TIMING_FRAME);
  
  // Make sure the headroom is correct
  *frame_in_hr = calc_headroom(frame_in, FRAME_SIZE);
}
//// -rx_frame


//// +rx_and_merge_frame
// Accept a frame of new audio data and merge it into sample_history
static inline 
void rx_and_merge_frame(
    int32_t sample_history[HISTORY_SIZE],
    exponent_t* sample_history_exp,
    headroom_t* sample_history_hr,
    const chanend_t c_audio)
{
  // BFP vector into which new frame will be placed.
  struct {
    int32_t data[FRAME_SIZE];   // Sample data
    exponent_t exp;             // Exponent
    headroom_t hr;              // Headroom
  } frame_in = {{0},0,0};

  // Accept a new input frame
  rx_frame(frame_in.data, 
           &frame_in.exp, 
           &frame_in.hr, 
           c_audio);

  // Rescale BFP vectors if needed so they can be merged
  const exponent_t min_frame_in_exp = frame_in.exp - frame_in.hr;
  const exponent_t min_history_exp = *sample_history_exp - *sample_history_hr;
  const exponent_t new_exp = MAX(min_frame_in_exp, min_history_exp);

  const right_shift_t hist_shr = new_exp - *sample_history_exp;
  const right_shift_t frame_in_shr = new_exp - frame_in.exp;

  if(hist_shr) {
    for(int k = FRAME_SIZE; k < HISTORY_SIZE; k++)
      sample_history[k] = ashr32(sample_history[k], hist_shr);
    *sample_history_exp = new_exp;
  }

  if(frame_in_shr){
    for(int k = 0; k < FRAME_SIZE; k++)
      frame_in.data[k] = ashr32(frame_in.data[k], frame_in_shr);
  }
  
  // Now we can merge the new frame in (reversing order)
  for(int k = 0; k < FRAME_SIZE; k++)
    sample_history[FRAME_SIZE-k-1] = frame_in.data[k];

  // And just ensure the headroom is correct
  *sample_history_hr = calc_headroom(sample_history, HISTORY_SIZE);
}
//// -rx_and_merge_frame


//// +tx_frame
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    const int32_t frame_out[],
    const exponent_t frame_out_exp,
    const headroom_t frame_out_hr,
    const unsigned frame_size)
{
  const exponent_t output_exp = -31;
  
  // The output channel is expecting PCM samples with a *fixed* exponent of
  // output_exp, so we'll need to convert samples to use the correct exponent
  // before sending.

  const right_shift_t samp_shr = output_exp - frame_out_exp;  

  timer_stop(TIMING_FRAME);

  for(int k = 0; k < frame_size; k++){
    int32_t sample = frame_out[k];
    sample = ashr32(sample, samp_shr);
    chan_out_word(c_audio, sample);
  }
}
//// -tx_frame


//// +filter_sample
//Apply the filter to produce a single output sample
int32_t filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const right_shift_t acc_shr)
{
  // The accumulator into which partial results are added.
  int64_t acc = 0;

  // Compute the inner product between the history and coefficient vectors.
  for(int k = 0; k < TAP_COUNT; k++){
    int64_t b = sample_history[k];
    int64_t c = filter_bfp.data[k];
    acc += (b * c);
  }

  return sat32(ashr64(acc, acc_shr));
}
//// -filter_sample


//// +filter_frame
// Calculate entire output frame
void filter_frame(
    int32_t frame_out[FRAME_SIZE],
    exponent_t* frame_out_exp,
    headroom_t* frame_out_hr,
    const int32_t history_in[HISTORY_SIZE],
    const exponent_t history_in_exp,
    const headroom_t history_in_hr)
{
  // log2() of max product of two signed 32-bit integers
  //  = log2(INT32_MIN * INT32_MIN) = log2(-2^31 * -2^31)
  const exponent_t INT32_SQUARE_MAX_LOG2 = 62;

  // log2(TAP_COUNT)
  const exponent_t TAP_COUNT_LOG2 = 10;

  // First, determine the output exponent to be used.
  const headroom_t total_hr = history_in_hr + filter_bfp.hr;
  const exponent_t acc_exp = history_in_exp + filter_bfp.exp;
  const exponent_t result_scale = INT32_SQUARE_MAX_LOG2 - total_hr 
                                + TAP_COUNT_LOG2;
  const exponent_t desired_scale = 31;
  const right_shift_t acc_shr = result_scale - desired_scale;

  *frame_out_exp = acc_exp + acc_shr;

  // Now, compute the output sample values using that exponent.
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start(TIMING_SAMPLE);
    frame_out[s] = filter_sample(&history_in[FRAME_SIZE-s-1], 
                                  acc_shr);
    timer_stop(TIMING_SAMPLE);
  }

  //Finally, calculate the headroom of the output frame.
  *frame_out_hr = calc_headroom(frame_out, FRAME_SIZE);
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

  // Represents the sample history as a BFP vector
  struct {
    int32_t data[HISTORY_SIZE]; // Sample data
    exponent_t exp;             // Exponent
    headroom_t hr;              // Headroom
  } sample_history = {{0},-200,0};

  // Represents output frame as a BFP vector
  struct {
    int32_t data[FRAME_SIZE];   // Sample data
    exponent_t exp;             // Exponent
    headroom_t hr;              // Headroom
  } frame_output;

  // Loop forever
  while(1) {

    // Read in a new frame
    rx_and_merge_frame(&sample_history.data[0], 
                       &sample_history.exp,
                       &sample_history.hr, 
                       c_audio);

    // Calc output frame
    filter_frame(&frame_output.data[0], 
                 &frame_output.exp, 
                 &frame_output.hr,
                 &sample_history.data[0], 
                 sample_history.exp, 
                 sample_history.hr);

    // Make room for new samples at the front of the vector.
    memmove(&sample_history.data[FRAME_SIZE], 
            &sample_history.data[0], 
            TAP_COUNT * sizeof(int32_t));

    // Send out the processed frame
    tx_frame(c_audio, 
             &frame_output.data[0], 
             frame_output.exp, 
             frame_output.hr, 
             FRAME_SIZE);
  }
}
//// -filter_task
