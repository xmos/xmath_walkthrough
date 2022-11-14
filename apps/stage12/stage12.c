
#include "common.h"

#define OVERLAP_COUNT   ((FRAME_SIZE) / (FRAME_OVERLAP))

// The exponent associated with the integer coefficients
const exponent_t coef_exp = -28;

// The exponent associated with the input samples.
// Note that this is somewhat arbitrary. -31 is convenient because it means the
// samples are in the range [-1.0, 1.0)
const exponent_t input_exp = -31;

// We want the output exponent to be the same as the input exponent for this,
// so that the result is comparable to other stages.
const exponent_t output_exp = input_exp;

// The filter coefficients
extern const int32_t filter_coef[TAP_COUNT+(2*(FRAME_OVERLAP))];




float_s32_t filter_sample(
    bfp_s32_t filter_block[OVERLAP_COUNT],
    bfp_s32_t overlap_segment[OVERLAP_COUNT])
{
  float_s32_t acc = {0,-200};
  for(int k = 0; k < OVERLAP_COUNT; k++){
    float_s64_t tmp = bfp_s32_dot(&overlap_segment[k], &filter_block[k]);
    acc = float_s32_add(acc, float_s64_to_float_s32(tmp));
  }

  return acc;
}

// We're going to do something quite unusual here.
// We have the sample history in OVERLAP_COUNT separate BFP vectors, each of
// which (in principle) may have a different exponent. 
// If we were guaranteed the exponents were the same in each segment, and that
// the buffers were adjacent in memory, we could just treat it as one long BFP
// vector. However, because exponents may differ, we need a different strategy.
// The filter coefficients all use the same exponent, if we can create
// OVERLAP_COUNT BFP vectors from the filter coefficients, each starting at the
// right place, we could then just get segment-wise inner products and then sum
// them together. 
// To do that, we needed to pad the filter coefficients with zeros on both ends,
// since all FRAME_SIZE (= TAP_COUNT + FRAME_OVERLAP) samples of history will be
// used. Padding by FRAME_OVERLAP on each side will do.

bfp_s32_t filter_blocks[OVERLAP_COUNT];

void filter_frame(
    bfp_s32_t* output,
    bfp_s32_t segments[OVERLAP_COUNT])
{
  // Both segments and samples within a segment are in reverse chronological
  // order.
  // filter_coef[FRAME_OVERLAP-1] is where the b[0] coefficient resides. For
  // each output sample, we need only slide each filter block's data pointer
  // over one word.

  // Set each filter sub-vector to its initial position.
  for(int k = 0; k < OVERLAP_COUNT; k++){
    filter_blocks[k].data = (int32_t*) &filter_coef[k*(FRAME_OVERLAP)+1];
  }

  output->exp = output_exp;
  
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();

    float_s32_t sample_out = filter_sample(filter_blocks, segments);

    output->data[s] = float_s32_to_fixed(sample_out, output->exp);

    // Shift each filter block's data pointer up one word, so that it is correct
    // for the next output sample
    for(int k = 0; k < OVERLAP_COUNT; k++){
      filter_blocks[k].data = &filter_blocks[k].data[1];
    }
    timer_stop();
  }

  bfp_s32_headroom(output);
}

bfp_s32_t result;
bfp_s32_t overlap_segments[OVERLAP_COUNT];

void proc_next_overlap(
    float sample_buff[FRAME_OVERLAP])
{
  // Convert float vector to BFP vector
  overlap_segments[0].exp = vect_f32_max_exponent(sample_buff, FRAME_OVERLAP);

  vect_f32_to_vect_s32(&(overlap_segments[0].data[0]), &sample_buff[0], 
                       FRAME_OVERLAP, overlap_segments[0].exp);

  // Compute BFP vector headroom
  bfp_s32_headroom(&overlap_segments[0]);

  // Process a frame's worth of data
  filter_frame(&result, overlap_segments);

  // Convert back to floating-point
  vect_s32_to_vect_f32(&sample_buff[0], &result.data[0], 
                       FRAME_OVERLAP, result.exp);

  // Rotate the overlap segments up 1 (note that this doesn't actually move
  // the underlying samples, since the bfp_s32_t struct points to the sample
  // buffer)
  bfp_s32_t tmp = overlap_segments[OVERLAP_COUNT-1];
  for(int k = OVERLAP_COUNT-1; k > 0; k--){
    overlap_segments[k] = overlap_segments[k-1];
  }
  overlap_segments[0] = tmp;


}

void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  int32_t overlaps_buff[OVERLAP_COUNT][FRAME_OVERLAP] = {{0}};

  // Initialize the overlap segments
  for(int k = 0; k < OVERLAP_COUNT; k++)
    bfp_s32_init(&overlap_segments[k], &overlaps_buff[k][0], 
                 0, FRAME_OVERLAP, 1);

  // Initialize the result vector
  int32_t result_buff[FRAME_OVERLAP];
  bfp_s32_init(&result, &result_buff[0], 0, FRAME_OVERLAP, 0);

  // Initialize filter blocks (data pointer doesn't matter)
  for(int k = 0; k < OVERLAP_COUNT; k++){
    bfp_s32_init(&filter_blocks[k], NULL, coef_exp, FRAME_OVERLAP, 0);
    filter_blocks[k].hr = HR_S32(filter_coef[FRAME_OVERLAP]);
  }
  
  float sample_buff[FRAME_OVERLAP];

  while(1) {
    for(int k = 0; k < FRAME_OVERLAP; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      const float sample_in_flt = ldexpf(sample_in, input_exp);
      sample_buff[FRAME_OVERLAP-k-1] = sample_in_flt;
    }

    // Process the samples
    proc_next_overlap(sample_buff);

    // Convert samples back to int32_t and send them to next thread.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      const float sample_out_flt = sample_buff[k];
      const int32_t sample_out = roundf(ldexpf(sample_out_flt, -output_exp));
      chan_out_word(c_pcm_out, sample_out);
    }
  }
}