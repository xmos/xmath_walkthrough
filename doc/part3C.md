
[Prev](part3B.md) | [Home](intro.md) | [Next](PartD.md)

# Stage 8

In **Part 3C** we finally use `lib_xcore_math`'s block floating-point (BFP) API.

Here we don't expect any particularly noticeable performance boost relative to
[**Part 3B**](part3B.md.md)'s implementation. At bottom, both stages
ultimately use the same function (`vect_s32_dot()`) to do the bulk of the work
computing the filter output. Instead, in this stage we will see how using the
`lib_xcore_math` BFP API can simplify our code by doing much of the book-keeping
for us.

That being said, this is not the ideal application of BFP arithmetic,
particularly because of our application's requirement for fixed-point output
samples. We'll see a somewhat more intrinsically BFP implementation when we get
to [**Stage 12**](stage12.md).

### From `lib_xcore_math`

This stage makes use of the following operations from `lib_xcore_math`:

* [`bfp_s32_init()`](TODO)
* [`bfp_s32_headroom()`](TODO)
* [`bfp_s32_use_exponent()`](TODO)
* [`vect_s32_dot_prepare()`](TODO)


## Implementation

### **Part 3C** `calc_headroom()` Implementation

From [`part3C.c`](TODO):
```c
// Compute headroom of int32 vector.
static inline
headroom_t calc_headroom(
    bfp_s32_t* vec)
{
  return bfp_s32_headroom(vec);
}
```

In this stage `calc_headroom()` just calls the `bfp_s32_headroom()` operation on
the provided BFP vector. `bfp_s32_headroom()` _both_ updates the `hr` field of
the `bfp_s32_t` object, and returns that headroom. In this case
`calc_headroom()` also returns that headroom, though in **Part 3C** the returned
value is not needed and not used.

### **Part 3C** `filter_task()` Implementation

From [`part3C.c`](TODO):
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

In **Part 3C** `filter_task()` is quite similar to that in the previous two
stages. This time we have a few calls at the beginning to initialize some of the
BFP vectors. Notably, this time we must intialize `bfp_filter_coef`, the BFP
vector representing the filter coefficients, which we did not need to do in
previous stages.

`bfp_s32_init()` is used to initialize each of the BFP vectors, associating a
`bfp_s32_t` object with an exponent, length, and most importantly, the buffer
used to store the vector's elements. The final argument to `bfp_s32_init()` is a
boolean indicating whether the vector's headroom should be calculated during
initialization.

Calculating the headroom involves iterating over the array's data, which should
be avoided when unnecessary. In particular, it usually does not make sense to
calculate headroom if the element buffer hasn't already been populated with
initial values.

### **Part 3C** `filter_frame()` Implementation

From [`part3C.c`](TODO):
```c
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
  sample_history.hr = sample_history->hr; // Might not be precisely correct, but is safe

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
```

`filter_frame()` in **Part 3C** is tricky. At bottom the issue here is that
`lib_xcore_math`'s BFP API doesn't provide any convolution operations suitable
for this scenario. There _are_ a pair of 32-bit BFP convolution functions,
[`bfp_s32_convolve_valid()`](TODO) and [`bfp_s32_convolve_same()`](TODO),
however these are optimized for (and only support) small convolution kernels of
`1`, `3`, `5` or `7` elements.

In fact, BFP is probably _not_ the right approach for implementing an FIR filter
using `lib_xcore_math` (we'll see better approaches in [**Stage
10**](stage10.md) and [**Part 4C**](stage11.md)).

But in keeping with the spirit of this tutorial, this stage attempt to implement
the filter using the BFP API as best possible.

`filter_frame()` first initializes a new BFP vector called `history_view`.
However, `history_view` does not get its own data buffer. instead it points to
an address somewhere within `sample_history`'s data buffer. Additionally, where
`sample_history`'s `length` is `HISTORY_SIZE` (1280), the length of
`history_view` is `TAP_COUNT` (1024). `history_view` uses the same exponent and
headroom as `sample_history`. Taken together, this makes `history_view`
something like a window onto some portion of the `sample_history` vector (hence
history _"view"_).

For each output sample computed by `filter_sample()`, `history_view`'s `data`
pointer 'slides' back 1 element towards the start of the `sample_history`
vector. Recall that in all previous stages, we actually did something similar --
each call to `filter_sample()` in previous stages passed a pointer to a
different location of the history vector.  This is basically doing the same
thing, in a way that corrects for the fact that **Part 3C**'s `filter_sample()`
needs a BFP vector _the same length as `bfp_filter_coef`_.

The other tricky piece to `filter_frame()` in **Part 3C** is the determination
of the output exponent.  Here we _could have_ just called
`vect_s32_dot_prepare()` to determine the output exponent. For the sake of
sticking with BFP functions it takes a different approach, which takes advantage
of our knowledge of the situation.

In particular, we know that under the surface `bfp_s32_dot()` is itself calling
`vect_s32_dot_prepare()`, and that because the exponent and headroom of both
`history_view` and `bfp_filter_coef` will be the same with each call (for a
given frame), we know that the output exponent will be the same each time. So
instead of calling `vect_s32_dot_prepare()` ourselves, we base the output
exponent on the first exponent returned by `filter_sample()`.

However, `filter_sample()` returns a `float_s64_t`, which, because of how
`vect_s32_dot()` is implemented, won't take up more than 40 bits of the 64-bit
mantissa. We need the result to fit in 32 bits, so we add 8 to the output
exponent, and use `float_s64_to_fixed()` to shift samples before placing them in
`frame_out`.

> **Note**: To be clear, if you find yourself doing these things in an
> actual application, you are probably better off reverting to the lower-level
> Vector API. That doesn't mean getting rid of all `bfp_s32_t` in your
> application or application component; it just means that within the function
> that implements your operation, you may want to destructure the `bfp_s32_t`
> and operate directly on its fields using the Vector API.
> 
> Additionally, if you're trying to use the BFP or Vector API to implement a
> time-domain FIR or IIR filter, you may want to look at the [Digital Filter
> API](TODO) instead.


### **Part 3C** `filter_sample()` Implementation

From [`part3C.c`](TODO):
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

`filter_sample()` in **Part 3C** is quite simple. It just calls `bfp_s32_dot()`
to compute the inner product of the provided `sample_history` BFP vector and the
filter coefficient BFP vector. 

In this stage, the filter coefficients are represented by `bfp_filter_coef` of
type `bfp_s32_t`. This vector uses the same underlying coefficient array as the
previous two stages (from `filter_coef_q2_30.c`), but they are wrapped in a BFP
vector which tracks their length, exponent and headroom for the user. BFP
vectors representd by a `bfp_s32_t` need to be initialized before they can be
used. In this case `bfp_filter_coef` is initialized in `filter_task()` prior to
entering the main thread loop.

### **Part 3C** `tx_frame()` Implementation

From [`part3C.c`](TODO):
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

In **Part 3C** `tx_frame()` is similar to that in **Part 3B**.  Its job is to
ensure output samples are using an exponent of `-31`, and then send them to the
`wav_io` thread.

In **Part 3B** this was accomplished by explicitly calculating a shift to be
applied to each sample value and then applying them before putting the output
sample into the channel. **Part 3C**, however, accomplishes this just by calling `bfp_s32_use_exponent()`.

`bfp_s32_use_exponent()` coerces a BFP vector to use a particular exponent, shifting the elements as necessary to accomplish that. It is an easy way to move from a block floating-point domain (this stage's filter) into a fixed-point domain (the `wav_io` thread).

### **Part 3C** `rx_frame()` Implementation

From [`part3C.c`](TODO):
```c
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
  
  // Make sure the headroom is correct
  calc_headroom(frame_in);
}
```

`rx_frame()` is almost identical to that in **Part 3A** and **Part 3B**. The
only real difference is that the mantissas, exponent and headroom are now
encapsulated inside the `bfp_s32_t` object.

### **Part 3C** `rx_and_merge_frame()` Implementation

From [`part3C.c`](TODO):
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

`rx_and_merge_frame()` in **Part 3C** is somewhat simpler than in the previous
two stages. A temporary BFP vector, `frame_in`, is initialized and `rx_frame()`
is called to populate that with the new frame data. After that, the exponents of
`frame_in` and `sample_history` still need to be reconciled, but this time once
the exponent is chosen we use `bfp_s32_use_exponent()` on both vectors to make
sure their exponents are equal. After that the new data is copied into the
sample history.


## Results

### Timing

| Timing Type       | Measured Timing
|-------------------|-----------------------
| Per Filter Tap    | 16.13 ns
| Per Output Sample | 16515.55 ns
| Per Frame         | 4312690.50 ns

### Output Waveform

![**Part 3C** Output](img/part3C.png)