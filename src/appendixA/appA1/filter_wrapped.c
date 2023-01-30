// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "appA1.h"


static unsigned tap_count = 0;
static unsigned frame_size = 0; // in this app, frame_size is always tap_count/8

static filter_fir_s32_t filter;

static const 
exponent_t fixed_point_exp = -31;


/**
 * 
*/
void filter_wrapped_init(
    const unsigned new_tap_count)
{
  const right_shift_t filter_shr = 0;

  tap_count = new_tap_count;
  frame_size = tap_count >> 3;

  if(filter.state) free(filter.state);
  if(filter.coef) free(filter.coef);

  int32_t* state_buff = (int32_t*) malloc(tap_count * sizeof(int32_t));
  int32_t* filter_coef = (int32_t*) malloc(tap_count * sizeof(int32_t));

  assert(state_buff);
  assert(filter_coef);

  for(int k = 0; k < tap_count; k++){
    state_buff[k] = rand();
    filter_coef[k] = rand();
  }

  filter_fir_s32_init(&filter, state_buff, tap_count, filter_coef, filter_shr);
}

/**
 * 
*/
void filter_wrapped(
    float frame[])
{
  assert(filter.coef && filter.state);

  const q1_31* frame_s32 = (q1_31*) &frame[0];

  // All steps can be done in-place using the input frame

  ////// 1. Convert input frame from float to Q1.31
  vect_f32_to_vect_s32( (q1_31*) &frame[0], 
                       &frame[0], 
                       frame_size, 
                       fixed_point_exp);

  ////// 2. Process Q1.31 samples to get Q1.31 output samples
  for(int k = 0; k < frame_size; k++)
    frame[k] = filter_fir_s32(&filter,  (q1_31)  frame_s32[k]);

  ////// 3. Convert output frame from Q1.31 to float
  vect_s32_to_vect_f32(&frame[0], 
                       (q1_31*) &frame[0], 
                       frame_size, 
                       fixed_point_exp);
}

/**
 * 
*/
void filter_wrapped_deinit()
{
  if(filter.state) free(filter.state);
  if(filter.coef) free(filter.coef);

  filter.coef = NULL;
  filter.state = NULL;
}