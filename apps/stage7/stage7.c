
#include "common.h"

/**
 * The number of FRAME_OVERLAPs that fit into FRAME_SIZE.
 * 
 * The filter coefficients and input samples will be split into this many BFP
 * vectors, allowing each new set of FRAME_OVERLAP samples received by this
 * thread to use a different BFP exponent.
 */
#define OVERLAP_COUNT   ((FRAME_SIZE) / (FRAME_OVERLAP))

/**
 * The filter coefficient array. Note that the coefficients for the 1024-tap
 * filter are padded on both sides by `FRAME_OVERLAP` zeros. This will be 
 * important for how this stage is implemented.
 */
extern 
const q4_28 filter_coef[TAP_COUNT+(2*(FRAME_OVERLAP))];

/**
 * The exponent associatd with the filter coefficients.
 * 
 * The value represented by the k'th coefficient is 
 * `ldexp(filter_coef[k], coef_exp)`.
 */
const exponent_t coef_exp = -28;

/**
 * The exponent associated with the input signal.
 * 
 * This exponent implies 32-bit PCM samples represent a value in the range
 * `[-1.0, 1.0)`.
 */
const exponent_t input_exp = -31;

/**
 * The exponent associated with the output signal.
 * 
 * This exponent implies 32-bit PCM samples represent a value in the range
 * `[-1.0, 1.0)`.
 */
const exponent_t output_exp = input_exp;


/**
 * Apply the filter to produce a single output sample.
 * 
 * `filter_block[]` is an array of OVERLAP_COUNT BFP vectors, each containing
 * FRAME_OVERLAP elements. Each of these BFP vectors corresponds to a portion
 * of the filter_coef[] vector, but that portion will 'slide' along the 
 * filter_coef[] vector as each sample is processed.
 * 
 * `overlap_segment[]` is an array of OVERLAP_COUNT BFP vectors, each containing
 * FRAME_OVERLAP input samples. Each of these BFP vectors corresponds to one
 * new frame of FRAME_OVERLAP samples received from the input thread.
 * 
 * STAGE 12 implements this filter by just adding together the inner products of 
 * the corresponding filter blocks and overlap segments.
 * Because each overlap segment is converted from an array of `float` values,
 * each segment may contain a different BFP exponent.
 */
float_s32_t filter_sample(
    const bfp_s32_t filter_block[OVERLAP_COUNT],
    const bfp_s32_t overlap_segment[OVERLAP_COUNT])
{
  float_s32_t acc = {0,0};
  for(int k = 0; k < OVERLAP_COUNT; k++){
    // Compute the inner product between one overlap segment and one filter block
    float_s64_t tmp = bfp_s32_dot(&overlap_segment[k], &filter_block[k]);
    // Add it to the accumulator
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

/**
 * BFP vectors representing the filter coefficients. Each block corresponds to
 * FRAME_OVERLAP coefficients from `filter_coef[]`.
 */
bfp_s32_t filter_blocks[OVERLAP_COUNT];

/**
 * Apply the filter to a frame with `FRAME_OVERLAP` new input samples, producing
 * one output sample for each new sample.
 * 
 * Computed output samples are placed into `output` with the oldest samples
 * first (forward time order).
 * 
 * `segments` contains the last `OVERLAP_COUNT` frame advances worth of input
 * samples, each converted from a vector of `OVERLAP_COUNT` `float`s.
 */
void filter_frame(
    bfp_s32_t* output,
    const bfp_s32_t segments[OVERLAP_COUNT])
{

  // Set each filter sub-vector to its initial position.
  for(int k = 0; k < OVERLAP_COUNT; k++){
    filter_blocks[k].data = (int32_t*) &filter_coef[k*(FRAME_OVERLAP)+1];
  }

  // Set the output exponent for the output BFP vector
  output->exp = output_exp;
  
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();

    // Get the next output sample
    float_s32_t sample_out = filter_sample(filter_blocks, segments);

    // Convert output sample to use correct exponent
    output->data[s] = float_s32_to_fixed(sample_out, output->exp);

    // Shift each filter block's data pointer up one word, to give the effect
    // that we're 'sliding' the filter coefficient vector along the input sample
    // vectors to effect our convolution.
    for(int k = 0; k < OVERLAP_COUNT; k++){
      filter_blocks[k].data = &filter_blocks[k].data[1];
    }
    timer_stop();
  }

  // Update the headroom of the output vector.
  bfp_s32_headroom(output);
}

/**
 * BFP vector into which results are placed.
 */
bfp_s32_t result;

/**
 * BFP vectors representing the last `OVERLAP_COUNT` frames of samples we've
 * received from the input thread.
 */
bfp_s32_t overlap_segments[OVERLAP_COUNT];

/**
 * Function which wraps the filter to provide a convenient `float` API for
 * applying the filter.
 * 
 * `sample_buff[]` is used for both input and output samples.
 * 
 * Each new frame of audio received is converted into a `FRAME_OVERLAP`-length
 * BFP vector and stored in `overlap_segments[]`, which are cycled as new frames
 * are received.
 */
void filter_frame_wrap(
    float sample_buff[FRAME_OVERLAP])
{
  // Convert float vector to BFP vector and place in the first overlap segment
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
  
  // Buffer used for input/output samples
  float sample_buff[FRAME_OVERLAP];

  while(1) {
    for(int k = 0; k < FRAME_OVERLAP; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      const float sample_in_flt = ldexpf(sample_in, input_exp);
      sample_buff[FRAME_OVERLAP-k-1] = sample_in_flt;
    }

    // Process the samples
    filter_frame_wrap(sample_buff);

    // Convert samples back to int32_t and send them to next thread.
    for(int k = 0; k < FRAME_OVERLAP; k++){
      const float sample_out_flt = sample_buff[k];
      const int32_t sample_out = roundf(ldexpf(sample_out_flt, -output_exp));
      chan_out_word(c_pcm_out, sample_out);
    }
  }
}