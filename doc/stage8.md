
[Prev](stage7.md) | [Home](../intro.md) | [Next](stage9.md)

# Stage 8

In **Stage 8** we finally begin using `lib_xcore_math`'s block floating-point
(BFP) API.

Here we don't expect any particularly noticeable performance boost relative to
[**Stage 7**](stage7.md)'s implementation. At bottom, both stages
ultimately use the same function (`vect_s32_dot()`) to do the bulk of the work
computing the filter output. Instead, in this stage we will see how using the
`lib_xcore_math` BFP API can simplify our code by doing much of the book-keeping
for us.

That being said, this is not the ideal application of BFP arithmetic,
particularly because of our application's requirement for fixed-point output
samples. We'll see a somewhat more intrinsically BFP implementation when we get
to [**Stage 12**](stage12.md).

## Introduction

## Background


## Implementation

### **Stage 8** `calc_headroom()` Implementation

From [`stage8.c`](TODO):
```c
// Compute headroom of int32 vector.
static inline
headroom_t calc_headroom(
    bfp_s32_t* vec)
{
  return bfp_s32_headroom(vec);
}
```

### **Stage 8** `filter_task()` Implementation

From [`stage8.c`](TODO):
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

  // Initialize BFP vector representing filter coefficients
  const exponent_t coef_exp = -30;
  bfp_s32_init(&bfp_filter_coef, (int32_t*) &filter_coef[0], 
               coef_exp, TAP_COUNT, 1);

  // Represents the sample history as a BFP vector
  int32_t sample_history_buff[HISTORY_SIZE] = {0};
  bfp_s32_t sample_history;
  bfp_s32_init(&sample_history, &sample_history_buff[0], -200, 
               HISTORY_SIZE, 0);
  bfp_s32_set(&sample_history, 0, -31);

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

    // Send out the processed frame
    tx_frame(c_audio,
             &frame_output);

    // Make room for new samples at the front of the vector.
    memmove(&sample_history.data[FRAME_SIZE], 
            &sample_history.data[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
```

### **Stage 8** `filter_frame()` Implementation

From [`stage8.c`](TODO):
```c
// Calculate entire output frame
void filter_frame(
    bfp_s32_t* frame_out,
    const bfp_s32_t* history_in)
{ 
  //We're faking an exponent here, just for the sake of demonstrating the BFP
  // behavior.
  frame_out->exp = -28;

  // Initialize a new BFP vector which is a 'view' onto a TAP_COUNT-element 
  // window of the history_in[] vector.
  // The sample_history[] window will 'slide' along history_in[] for each output
  // sample. This isn't something you'd typically need to do.
  bfp_s32_t sample_history;
  bfp_s32_init(&sample_history, &history_in->data[FRAME_SIZE],
                history_in->exp, TAP_COUNT, 0);
  sample_history.hr = history_in->hr; // Might not be precisely correct, but is safe

  // Compute FRAME_SIZE output samples.
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    // Slide the window down one index, towards newer samples
    sample_history.data = sample_history.data - 1;
    // Get next output sample
    float_s64_t samp = filter_sample(&sample_history);

    // Because the exponents and headroom are the same for every call to
    // filter_sample(), the output exponent will also be the same (it's computed
    // before the dot product is computed). So we'll use whichever exponent the 
    // first result says to use... plus 8, just like in the previous stage, for
    // the same reason (int40 -> int32)
    if(!s)
      frame_out->exp = samp.exp + 8;

    frame_out->data[s] = float_s64_to_fixed(samp, frame_out->exp);
    timer_stop();
  }
}
```

### **Stage 8** `filter_sample()` Implementation

From [`stage8.c`](TODO):
```c
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
```

### **Stage 8** `tx_frame()` Implementation

From [`stage8.c`](TODO):
```c
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

  // And send the samples
  for(int k = 0; k < FRAME_SIZE; k++)
    chan_out_word(c_audio, frame_out->data[k]);
  
}
```

### **Stage 8** `rx_frame()` Implementation

From [`stage8.c`](TODO):
```c
static inline 
void rx_frame(
    bfp_s32_t* frame_in,
    const chanend_t c_audio)
{
  // We happen to know a priori that samples coming in will have a fixed 
  // exponent of input_exp, and there's no reason to change it, so we'll just
  // use that.
  // TODO -- Randomize the exponent to simulate receiving differently-scaled
  // frames?
  frame_in->exp = -31;

  for(int k = 0; k < FRAME_SIZE; k++)
    frame_in->data[k] = chan_in_word(c_audio);
  
  // Make sure the headroom is correct
  calc_headroom(frame_in);
}
```

### **Stage 8** `rx_and_merge_frame()` Implementation

From [`stage8.c`](TODO):
```c
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
```




















---

## Code Changes

### `filter_sample()`

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

From `stage8.c`:
```c
bfp_s32_t bfp_filter_coef;

int32_t filter_sample(
    const bfp_s32_t* sample_history
    const exponent_t out_exp)
{
  float_s64_t acc = bfp_s32_dot(sample_history, &bfp_filter_coef);
  
  int32_t sample_out = float_s64_to_fixed(acc, out_exp);

  return sample_out;
}
```

In **Stage 8**, the filter coefficients are represented by `bfp_filter_coef` of
type `bfp_s32_t`. `bfp_s32_t` is a struct type from `lib_xcore_math` which
represents a BFP vector with 32-bit mantissas. The `bfp_s32_t` type has 4
important fields:

Field             | Description
-----             | -----------
`int32_t* data`   | A pointer to an `int32_t` array; the buffer which backs the mantissa vector.
`unsigned length` | Indicates the number of elements in the BFP vector. 
`exponent_t exp`  | The exponent associated with the vector's mantissas.
`headroom_t hr`   | The headroom of the vector's mantissas.

> Note: in `lib_xcore_math`, unless otherwise specified, a vector's "length" is
> always the number of elements, rather than its Euclidean length.

We also here for the first time encounter
[`bfp_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L498-L522).
Like `vect_s32_dot()`, `bfp_s32_dot()` computes the inner product between two
BFP vectors, but unlike `vect_s32_dot()`, it takes care of all the management of
exponents and headroom for us. `bfp_s32_dot()` basically encapsulates all the
logic being performed in **Stage 7**'s `filter_sample()`, apart from converting
the output to the proper fixed-point representation.

Because our application is constrained to outputting the audio data in a
fixed-point format, **Stage 8**'s `filter_sample()` still has to convert the
`float_s64_t` result into the proper fixed-point format with a call to
`float_s64_to_fixed()`.


### `filter_frame()`

From `stage7.c`:
```c
void filter_frame(
    q1_31 frame_out[FRAME_SIZE],
    const int32_t history_in[HISTORY_SIZE],
    const exponent_t history_in_exp,
    const headroom_t history_in_hr)
{
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    frame_out[s] = filter_sample(&history_in[FRAME_SIZE-s-1], 
                                  history_in_exp, 
                                  history_in_hr);
    timer_stop();
  }
}
```

From `stage8.c`:
```c
void filter_frame(
    bfp_s32_t* frame_out,
    const bfp_s32_t* history_in)
{
  frame_out->exp = output_exp+2;

  bfp_s32_t sample_history;
  bfp_s32_init(&sample_history, &history_in->data[FRAME_SIZE],
                history_in->exp, TAP_COUNT, 0);

  sample_history.hr = history_in->hr;

  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    sample_history.data = sample_history.data - 1;
    frame_out->data[s] = filter_sample(&sample_history);
    timer_stop();
  }
}
```

Aside from the function parameters, **Stage 8**'s `filter_frame()` function
largely resembles that of **Stage 7**. There are a couple points here in **Stage
8** that are worth noticing.

First, in **Stage 8** we directly specify the exponent associated with the
`frame_out` BFP vector to be `output_exp + 2`. This isn't something we would
ordinarily do for BFP arithmetic. Normally the BFP functions would select
exponents for us. However, `bfp_s32_dot()` is already doing that work, for each computed sample. So instead here we arbitrarily choose an exponent, just as an excuse to use another BFP function we haven't seen before in `filter_task()`.

Next, and more interestingly, in **Stage 8**'s `filter_frame()` we create a new
`bfp_s32_t` object, `sample_history`, which points to the same buffer as
`history_in`. This is because we can only compute the inner product of two
vectors if they have the same number of elements. `bfp_filter_coef` has length `TAP_COUNT` (1024), whereas `history_in` has length `HISTORY_SIZE` (1280).

To 'trick' `bfp_s32_dot()` into computing the inner product for us, we use
`sample_history` as a `TAP_COUNT`-element window into `history_in`. For each
subsequent output sample we slide that window over one element.

Note that we set `sample_history.hr` to `history_in->hr`. Because the headroom
of a vector is defined as the minimum headroom among its elements, we're
guaranteed that `sample_history.data` has _at least_ as much headroom as
`history_in->data`. In some cases `sample_history.hr` may be _less_ headroom
than we actually have in `sample_history.data`, but recomputing it for each
output sample would be unreasonably expensive.

### `filter_task()`

From `stage7.c`:
```c
void filter_task(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  int32_t sample_history[HISTORY_SIZE] = {0};
  exponent_t sample_history_exp;
  headroom_t sample_history_hr;
  q1_31 frame_output[FRAME_SIZE] = {0};

  while(1) {
    for(int k = 0; k < FRAME_SIZE; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      sample_history[FRAME_SIZE-k-1] = sample_in;
    }

    sample_history_exp = -31;
    sample_history_hr = vect_s32_headroom(&sample_history[0], HISTORY_SIZE);

    filter_frame(frame_output, sample_history, 
                 sample_history_exp, sample_history_hr);

    for(int k = 0; k < FRAME_SIZE; k++)
      chan_out_word(c_pcm_out, frame_output[k]);
    
    memmove(&sample_history[FRAME_SIZE], &sample_history[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
```

From `stage8.c`:
```c
void filter_task(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  bfp_s32_init(&bfp_filter_coef, (int32_t*) &filter_coef[0], 
               coef_exp, TAP_COUNT, 1);

  int32_t sample_history_buff[HISTORY_SIZE] = {0};
  bfp_s32_t sample_history;
  bfp_s32_init(&sample_history, &sample_history_buff[0], -31, HISTORY_SIZE, 0);
  bfp_s32_set(&sample_history, 0, -31);

  int32_t frame_output_buff[FRAME_SIZE] = {0};
  bfp_s32_t frame_output;
  bfp_s32_init(&frame_output, &frame_output_buff[0], 0, FRAME_SIZE, 0);

  while(1) {
    for(int k = 0; k < FRAME_SIZE; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      sample_history.data[FRAME_SIZE-k-1] = sample_in;
    }

    sample_history.exp = -31;
    bfp_s32_headroom(&sample_history);

    filter_frame(&frame_output, &sample_history);

    bfp_s32_use_exponent(&frame_output, output_exp);

    for(int k = 0; k < FRAME_SIZE; k++)
      chan_out_word(c_pcm_out, frame_output.data[k]);
    

    memmove(&sample_history.data[FRAME_SIZE], &sample_history.data[0], 
            TAP_COUNT * sizeof(int32_t));
  }
}
```

Here, the most notable difference between **Stage 7**'s `filter_task()` and
**Stage 8**'s `filter_task()` is that **Stage 8**'s starts by initializing the
BFP vectors that are used.

Generally speaking, all `bfp_s32_t` must be initialized prior to being used as
an operand in an arithmetic operation. The most important two things initializing the BFP vector does are to attach its mantissa buffer and to set its length.

Initialization of a `bfp_s32_t` (32-bit BFP vector) is done with a call to [`bfp_s32_init()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L17-L45). The final parameter, `calc_hr` indicates 
whether the headroom should be calculated during initialization. This is 
unnecessary when the element buffer has not been filled with data.

As in **Stage 7**, each time a new input frame is received `sample_history`'s
headroom is recomputed, however in **Stage 8**
[`bfp_s32_headroom()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L190-L218)
is used instead. `bfp_s32_headroom()` is basically a thin wrapper for
`vect_s32_headroom()` except that it also sets the `hr` field of the BFP vector.

Finally, in **Stage 8** after the new output samples are computed,
[`bfp_s32_use_exponent()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L134-L187)
is used to force the BFP vector to use `output_exp` as its exponent. A call to
`bfp_s32_use_exponent()` with a constant for its `exp` parameter is essentially
a conversion to a fixed-point format. In this case, the fixed-point format is a
constraint imposed by the application.

## Results

## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
* [`bfp_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L498-L522)
* [`bfp_s32_init()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L17-L45)
* [`bfp_s32_headroom()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L190-L218)
* [`bfp_s32_use_exponent()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L134-L187)