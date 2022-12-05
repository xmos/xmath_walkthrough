
#include "common.h"

// The number of threads that will be used to compute the result.
#define THREADS     4

extern "C" {
  // Represents the filter coefficients as a BFP vector
  extern struct {
    int32_t* data;
    exponent_t exp;
    headroom_t hr; 
  } filter_bfp; 

  // Apply the filter to produce a single output sample.
  // Defined in stage9.c
  C_API
  int64_t filter_sample(
      const int32_t * unsafe sample_history,
      const right_shift_t b_shr,
      const right_shift_t c_shr);
}


// Required for using unsafe (i.e. C-style) pointers in XC
unsafe {


// Calculate entire output frame
void filter_frame(
    int32_t* unsafe frame_out,
    exponent_t* frame_out_exp,
    headroom_t* frame_out_hr,
    const int32_t history_in[HISTORY_SIZE],
    const exponent_t history_in_exp,
    const headroom_t history_in_hr)
{
  // First, determine output exponent and required shifts.
  right_shift_t b_shr, c_shr;
  vect_s32_dot_prepare(frame_out_exp, &b_shr, &c_shr, 
                       history_in_exp, filter_bfp.exp,
                       history_in_hr, filter_bfp.hr, 
                       TAP_COUNT);
  // vect_s32_dot_prepare() ensures the result doesn't overflow the 40-bit VPU
  // accumulators, but we need it in a 32-bit value.
  right_shift_t s_shr = 8;
  *frame_out_exp += s_shr;

  // Compute FRAME_SIZE output samples.
  for(int s = 0; s < FRAME_SIZE; s+=THREADS){
    timer_start(TIMING_SAMPLE);
    par(int tid = 0; tid < THREADS; tid++) {
      frame_out[s+tid] = sat32(
        ashr64(filter_sample(
                &history_in[FRAME_SIZE-(s+tid)-1], 
                b_shr, 
                c_shr), 
          s_shr));
    }
    timer_stop(TIMING_SAMPLE);
  }

  //Finally, calculate the headroom of the output frame.
  *frame_out_hr = vect_s32_headroom((int32_t*)frame_out, FRAME_SIZE);
}

}