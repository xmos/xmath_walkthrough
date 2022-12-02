
[Prev](stage6.md) | [Home](../intro.md) | [Next](stage8.md)

# Stage 7

Like [**Stage 6**](stage6.md), **Stage 7** implements the FIR filter 
using block floating-point arithmetic.

In **Stage 6** we implemented the block floating-point FIR filter using plain C.
We had to manage exponents and headroom ourselves, as well as the logic for
computing the inner product, which was similar to [**Stage
3**](stage3.md).

In **Stage 7** we will replace our C code which calculates and manages headroom
and exponents with calls to functions from
[`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)'s low-level vector
API to do some of this work for us. We also replace our `for` loop where the
inner product is computed with a call to
[`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480),
which we encountered in [**Stage 5**](stage5.md).

This will also mean using the VPU to do the bulk of the work, rather than the
scalar unit.

## Introduction

## Background

## Implementation

### **Stage 7** `calc_headroom()` Implementation

From [`stage7.c`](TODO):
```c
// Compute headroom of int32 vector.
static inline
headroom_t calc_headroom(
    const int32_t vec[],
    const unsigned length)
{
  return vect_s32_headroom(vec, length);
}
```

### **Stage 7** `filter_task()` Implementation

From [`stage7.c`](TODO):
```c
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

    // Send out the processed frame
    tx_frame(c_audio, 
             &frame_output.data[0], 
             frame_output.exp, 
             frame_output.hr, 
             FRAME_SIZE);

    // Make room for new samples at the front of the vector.
    memmove(&sample_history.data[FRAME_SIZE], 
            &sample_history.data[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
```

### **Stage 7** `filter_frame()` Implementation

From [`stage7.c`](TODO):
```c
// Calculate entire output frame
void filter_frame(
    int32_t frame_out[FRAME_SIZE],
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
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    int64_t samp = filter_sample(&history_in[FRAME_SIZE-s-1], 
                                 b_shr, c_shr);
    frame_out[s] = sat32(ashr64(samp, s_shr));
    timer_stop();
  }

  //Finally, calculate the headroom of the output frame.
  *frame_out_hr = calc_headroom(frame_out, FRAME_SIZE);
}
```

### **Stage 7** `filter_sample()` Implementation

From [`stage7.c`](TODO):
```c
// Apply the filter to produce a single output sample.
int64_t filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const right_shift_t b_shr,
    const right_shift_t c_shr)
{
  // Compute the inner product's mantissa using the given shift parameters.
  return vect_s32_dot(&sample_history[0], 
                      &filter_bfp.data[0], TAP_COUNT,
                      b_shr, c_shr);
}
```

### **Stage 7** `tx_frame()` Implementation

From [`stage7.c`](TODO):
```c
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

  for(int k = 0; k < frame_size; k++){
    int32_t sample = frame_out[k];
    sample = ashr32(sample, samp_shr);
    chan_out_word(c_audio, sample);
  }
}
```

### **Stage 7** `rx_frame()` Implementation

From [`stage7.c`](TODO):
```c
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
  // TODO -- Randomize the exponent to simulate receiving differently-scaled
  // frames?
  *frame_in_exp = -31;

  for(int k = 0; k < FRAME_SIZE; k++)
    frame_in[k] = chan_in_word(c_audio);
  
  // Make sure the headroom is correct
  calc_headroom(frame_in, FRAME_SIZE);
}
```

### **Stage 7** `rx_and_merge_frame()` Implementation

From [`stage7.c`](TODO):
```c
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
```

















---

### Headroom Calculation in `filter_task()`

The first change from **Stage 6** to **Stage 7** we will look at is how the 
headroom of the input audio frame is calculated.

In `stage6.c`, we calculated the input headroom in a `for` loop in
`filter_task()` with a little help from the
[`HR_S32()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/util.h#L145-L154)
macro:

```c
  // Compute headroom of input frame
  sample_history_hr = 32;
  for(int k = 0; k < HISTORY_SIZE; k++)
    sample_history_hr = MIN(sample_history_hr, HR_S32(sample_history[k]));
```

In `stage7.c`, we're instead calling
[`vect_s32_headroom()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L554-L591) from `lib_xcore_math`:

```c
  // Compute headroom of input frame
  sample_history_hr = vect_s32_headroom(&sample_history[0], HISTORY_SIZE);
```

Though not included in the timing performance measured by the apps, this new
implementation is a much, much faster way to compute the vector's headroom.

### `filter_sample()`

The other key change from **Stage 6** to **Stage 7** is, of course, the
implementation of `filter_sample()`.

From `stage6.c`:
```c
const right_shift_t p_shr = 10;

...

q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr)
{
  float_s64_t acc;
  acc.mant = 0;

  acc.exp = (history_exp + coef_exp) + p_shr;

  for(int k = 0; k < TAP_COUNT; k++){
    int32_t b = sample_history[k];
    int32_t c = filter_coef[k];
    int64_t p =  (((int64_t)b) * c);
    acc.mant += ashr64(p, p_shr);
  }

  q1_31 sample_out = float_s64_to_fixed(acc, output_exp);

  return sample_out;
}
```

From `stage7.c`:
```c
q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr)
{
  float_s64_t acc;
  acc.mant = 0;

  const headroom_t coef_hr = HR_S32(filter_coef[0]);

  right_shift_t b_shr, c_shr;
  vect_s32_dot_prepare(&acc.exp, &b_shr, &c_shr, 
                        history_exp, coef_exp,
                        history_hr, coef_hr, TAP_COUNT);

  acc.mant = vect_s32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT, 
                        b_shr, c_shr);

  q1_31 sample_out = float_s64_to_fixed(acc, output_exp);

  return sample_out;
}
```

In **Stage 7** we make use of 2 functions from `lib_xcore_math`. One we saw in
**Stage 5**, `vect_s32_dot()`. New here is [`vect_s32_dot_prepare()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32_prepare.h#L182-L252).

To facilitate block floating-point logic, `lib_xcore_math`'s vector API provides
a number of "prepare" functions. A "prepare" function is a simple function
(written in C) which selects the output exponent and any shift parameters
required for a subsequent call to one of the optimized vector operations. By
convention, these "prepare" functions are simply the name of the function being
prepared for, with `_prepare` appended to the end, as with `vect_s32_dot()` and
`vect_s32_dot_prepare()`.

The exponent associated with the output of `vect_s32_dot()` depends upon 4
things. Those are the exponents associated with each of the two input vectors
`b[]` and `c[]`, and the `b_shr` and `c_shr` shift parameters.

`vect_s32_dot_prepare()` has the following prototype:
```c
void vect_s32_dot_prepare(
    exponent_t* a_exp,
    right_shift_t* b_shr,
    right_shift_t* c_shr,
    const exponent_t b_exp,
    const exponent_t c_exp,
    const headroom_t b_hr,
    const headroom_t c_hr,
    const unsigned length);
```

> Note: A convention used widely in `lib_xcore_math`'s APIs is that the output
> (vector or scalar) from an arithmetic operation is usually named `a`, and then
> the input operands (vectors or scalars) are named `b`, `c`, etc. Understanding
> this will greatly simplify interpretation of API functions.
>
> With this convention, the 'inputs' and 'outputs' are those of the _arithmetic
> operation_ rather than function parameters or return values, though they may
> often coincide.

Here `a_exp`, `b_shr` and `c_shr` are all outputs selected by the function.
`b_exp` and `b_hr` are the exponent and headroom associated with input vector
`b[]` -- in our case, the sample history. Likewise, `c_exp` and `c_hr` are the
exponent and headroom associated with input vector `c[]` -- in our case, the
filter coefficients. `length` is the number of elements in `b[]` and `c[]`.

`vect_s32_dot_prepare()` will use those values to determine the minimum exponent
`a_exp` for which the result of `vect_s32_dot()` is guaranteed not to saturate.
The minimum exponent will also minimize the headroom of the result, allowing for
maximal precision.

> Note: While the return type of `vect_s32_dot()` is `int64_t`, the accumulators
> on the VPU in 32-bit mode are only 40 bits wide. `vect_s32_dot_prepare()`
> takes that into account when selecting an output exponent.

## Results

## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
* [`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480)
* [`HR_S32()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/util.h#L145-L154)
* [`vect_s32_headroom()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L554-L591)
* [`vect_s32_dot_prepare()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32_prepare.h#L182-L252)