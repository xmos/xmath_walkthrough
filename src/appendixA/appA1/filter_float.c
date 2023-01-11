// Copyright 2017-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "appA1.h"


static unsigned tap_count = 0;
static unsigned frame_size = 0; // in this app, frame_size is always tap_count/8

static float* filter_state = NULL;
static float* filter_coef = NULL;

static unsigned state_head = 0;


/**
 * 
*/
void filter_float_init(
    const unsigned new_tap_count)
{
  tap_count = new_tap_count;
  frame_size = tap_count >> 3;

  if(filter_state) free(filter_state);
  if(filter_coef) free(filter_coef);

  filter_state = (float*) malloc(tap_count * sizeof(float));
  filter_coef = (float*) malloc(tap_count * sizeof(float));
  state_head = tap_count-1;

  assert(filter_state);
  assert(filter_coef);

  for(int k = 0; k < tap_count; k++){
    filter_state[k] = ldexpf(rand(), -30);
    filter_coef[k] = ldexpf(rand(), -30);
  }
}


/**
 * 
*/
void filter_float(
    float frame[])
{
  assert(filter_coef && filter_state);

  // iterate over new input samples
  for(int k = 0; k < frame_size; k++){

    // Add next sample to the sample history
    filter_state[state_head] = frame[k];

    // Because filter_state is circular and filter_coef is not, we have to 
    // perform the inner product in 2 phases.
    //    phaseA corresponds to the low-lag filter taps (e.g. b[0], b[1], etc)
    //    phaseB corresponds to the remaining taps. 
/*
  The numbers shown in this diagram are the LAG indices. i.e the number k in 
  state[] (at whatever position it appears) corresponds to the same filter tap 
  as k, in coef[].  

  EXAMPLE:  (tap_count = 8; state_head = 4)

                        H          <-- state_head
      state = [ 4 5 6 7 0 1 2 3 ]  <-- lag indices
                B B B B A A A A    <-- phaseA/phaseB

      coef  = [ 0 1 2 3 4 5 6 7 ]
                A A A A B B B B 
    
    And then on the following sample: (tap_count = 8; state_head = 3)

                      H            <-- state_head
      state = [ 5 6 7 0 1 2 3 4 ]  <-- lag indices
                B B B A A A A A    <-- phaseA/phaseB

      coef  = [ 0 1 2 3 4 5 6 7 ]
                A A A A A B B B 
               |---------|        <-- phaseA_length
                         |-----|  <-- phaseB_length
*/

    const unsigned phaseA_length = tap_count - state_head;
    const unsigned phaseB_length = tap_count - phaseA_length; 
    const unsigned phaseA_state = state_head;
    const unsigned phaseB_state = 0;
    const unsigned phaseA_coef = 0;
    const unsigned phaseB_coef = phaseA_length;
    
    float acc = 0.0f;
    acc += vect_f32_dot(&filter_state[phaseA_state], 
                        &filter_coef[phaseA_coef], 
                        phaseA_length);
                        
    acc += vect_f32_dot(&filter_state[phaseB_state], 
                        &filter_coef[phaseB_coef], 
                        phaseB_length);

    // Place result in output frame
    frame[k] = acc;

    // Move head index
    state_head = (state_head? state_head : tap_count) - 1;
  }
}


/**
 * 
*/
void filter_float_deinit()
{
  if(filter_state) free(filter_state);
  if(filter_coef) free(filter_coef);

  filter_coef = NULL;
  filter_state = NULL;
}