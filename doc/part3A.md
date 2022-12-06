
[Prev](part3.md) | [Home](intro.md) | [Next](part3B.md)

# Part 3A

In the previous 3 stages the FIR filter was implemented using fixed-point
arithmetic. In the next three stages the filter will be implemented using block
floating-point (BFP) arithmetic.

In **Part 3A** we will implement the BFP logic in C to ensure the reader sees everything that is happening. In the following two stages we will use functions from the `lib_xcore_math` library to assist us.

## Implementation

### **Part 3A** `calc_headroom()` Implementation

From [`part3A.c`](TODO):
```c
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
```

As mentioned in [**Part 3**](part3.md#headroom), the headroom of a mantissa vector is defined as the least headroom among its elements. Here `hr` is initialized to 32, as a 32-bit integer cannot have more than 32 bits of headroom, and we iterate over elements, finding the least headroom.

[`HR_S32()`](TODO) is a macro defined in `lib_xcore_math` to efficiently get the headroom of a signed 32-bit integer. It uses a native instruction `CLS`, which reports the number of leading sign bits of an integer.

### **Part 3A** `filter_task()` Implementation

From [`part3A.c`](TODO):
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

Here `filter_task()` does basically the same thing as in previous stages, only
now `sample_history` and `frame_output` are `struct`s which track the exponent
(`exp`) and headroom (`hr`) alongside the mantissa vector. 

Notice that pointers to the exponent and headroom are also passed along to each of the functions being called. This is typical in block floating-point where you don't have a common object type representing the BFP vector (such as `lib_xcore_math`'s `bfp_s32_t`). It can get cumbersome, as in the call to `filter_frame()`, where _2_ BFP vectors are taken as arguments. Many operations involve 3 or more BFP vectors.

### **Part 3A** `filter_frame()` Implementation

From [`part3A.c`](TODO):
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
```

`filter_frame()` in this stage must do several things. `frame_out[]`, the output
mantissa vector, `frame_out_exp`, the exponent associated with the output sample
vector, and `frame_out_hr`, the headroom associated with the output sample
vector, are all outputs of this function.

Before `filter_frame()` can compute the output samples it must determine the
appropriate shift to be applied to the accumulator when `filter_sample()` is
called.

$$
\mathtt{INT32\_SQUARE\_MAX\_LOG2} = \log_2(\mathtt{INT32\_MIN}) 
    = \log_2((-2^{31})^2) = \log_2(2^{62}) = 62
$$

This is the largest magnitude integer that can be the result of a signed 32-bit
multiplication. Note that:

$$
  \mathtt{INT32\_MAX}^2 
      \lt \left|\mathtt{INT32\_MAX} \cdot \mathtt{INT32\_MIN}\right| 
      \lt \mathtt{INT32\_MIN}^2
$$

`TAP_COUNT_LOG2` is $\log_2(1024)$. This value is the number of extra bits of
result we may need because we're adding 1024 terms together.

If we had two vectors of `int32_t`, where all elements in each were `INT32_MIN`,
then the inner product would be:

$$
  1024\cdot(\mathtt{INT32\_MIN}^2)=2^{10} \cdot(2^{62}) = 2^{72} \\
$$

Every bit of headroom in either input vector reduces that worst case scenario by
one bit, which is why they are subtracted from the sum of
`INT32_SQUARE_MAX_LOG2` and `TAP_COUNT_LOG2` to get `result_scale`.

To store the result in an `int32_t`, however, the value must not be greater than
$2^31$, which is our `desired_scale`. The difference between those two values (or rather, their `log2()`) is the required bit-shift, `acc_shr`.

With `acc_shr`, the output samples can be computed.

The output exponent is just the accumulator exponent, `acc_exp` with the
accumulator shift, `acc_shr`, added. This is because a right-shift of `acc_shr`
bits is equivalent (up to rounding error) to division by
$2^{\mathtt{acc\_shr}}$. Incrementing the accumulator exponent prevents the
shift from actually changing the _represented_ value.

Note that if `acc_shr` were defined as a left-shift instead of a right-shift, we
would subtract `acc_shr` instead.

Finally, the headroom of the output sample needs to be calculated, which we do with a call to `calc_headroom()`.

### **Part 3A** `filter_sample()` Implementation

From [`part3A.c`](TODO):
```c
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
```

`filter_sample()` looks familiar from **Part 2**. It is does the same thing as in **Part 2**, except that it takes `acc_shr` as a function parameter, instead of computing it based on the input, filter and output exponents.

### **Part 3A** `tx_frame()` Implementation

From [`part3A.c`](TODO):
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

`tx_frame()` also looks much like it did in previous stages, except that this time the exponent associated with `frame_out[]` when `tx_frame()` is called may not be the required output exponent, `-31`. So, `tx_frame()` must coerce the sample mantissas into the `Q1.31` fixed-point format before sending through the channel.

Conversion from some exponent $\hat{z}$ to a desired exponent $\hat{d}$ using a right-shift $\vec{r}$ is always the same:

$$
  \vec{r} = \hat{d} - \hat{z}
$$

### **Part 3A** `rx_frame()` Implementation

From [`part3A.c`](TODO):
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
  *frame_in_exp = -31;

  for(int k = 0; k < FRAME_SIZE; k++)
    frame_in[k] = chan_in_word(c_audio);
  
  // Make sure the headroom is correct
  calc_headroom(frame_in, FRAME_SIZE);
}
```

`rx_frame()` also looks similar to previous stages, except this time it has to
report the exponent associated with the input frame to the calling function.

In our case we are receiving frames of PCM samples which all use the exponent
`-31`.

### **Part 3A** `rx_and_merge_frame()` Implementation

From [`part3A.c`](TODO):
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

`rx_and_merge_frame()` does something a bit complicated. First it declares a new
temporary BFP vector to hold the a received input frame, and makes a call to
`rx_frame()` to retrieve one.

We happen to know _a priori_ (because we just saw `rx_frame()`) that the
exponent associated with the input frame will be `-31`, and so we _could_ just
overwrite the values in `sample_history[]` like we did in previous stages and be
done with it. However, here we are not assuming we know this.

In the case where the exponent associated with `sample_history[]`,
`sample_history_exp`, and the exponent associated with the received frame,
`frame_in.exp`, are _not_ the same, we have an issue. If we were to just
overwrite the values in `sample_history[]`, we would end up with different
portions of the sample history using different exponents, which we can't allow.

To deal with this, we need to coerce both vectors into using the same exponent.

> **Note**: that this is also always the case when _adding_ or _subtracting_ the
> mantissas of BFP vectors. It does not make arithmetic sense to add mantissas
> with different exponents. To see why, let's look at a simple example.
>
> We have two 16-bit mantissas, $\mathtt{x} = 10234$ and $\mathtt{y} = 22243$
> associated with exponents $\hat{x} = -14$ and $\hat{y} = -12$. Recall that the
> negative of the exponent is also the number of fractional bits. So, we can
> rewrite $\mathtt{x}$ and $\mathtt{y}$ in binary with the appropriate radix and
> try to add them:
>    
>       x = 0 0.1 0 0 1 1 1 1 1 1 1 1 0 1 0 
>     + y = 0 1 0 1.0 1 1 0 1 1 1 0 0 0 1 1 
>     --------------------------------------
>           0 1 1 1 1 1 1 0 1 1 0 1 1 1 0 1 ??
>
> But where would we even put the radix? To add them, they must be _aligned_ on
> their radix point, which means using the same exponent.

Deciding which new exponent to use is a matter of determining the maximum
minimum exponent among the vectors. Let me explain.

If a BFP vector $\bar{x}$ has exponent $\hat{x}$ and headroom $h_x$, then we
could also represent $\bar{x}$ using a new exponent $\hat{x^*} = \hat{x} - h_x$.
To use this new exponent, the mantissas $\mathtt{x[k]}$ need to be
_left_-shifted by $h_x$ bits. Then the new exponent is $\hat{x^*}$ with $h_{x^*}
= 0$.  In this case, $\hat{x^*}$ is the _minimum_ exponent that can be used to
represent the vector $\bar{x}$, because any lower exponent would cause an
overflow in the mantissas.

If a vector $\bar{x}$ has minimum exponent $\hat{x^*}$ and a vector $\bar{y}$
has minimum exponent $\hat{y^*}$, and we plan to _replace_ some mantissas of one
of the vector with those of the other, then we can take the _maximum_ of
$\hat{x^*}$ and $\hat{y^*}$ as the new exponent that both can be coerced to
without overflows.

> **Note**: While we are guaranteed $\hat{x^*}$ can be used for $\bar{x}$
> without any quantization error, applying $\hat{x^*}$ to $\bar{y}$ (or vice
> versa) _may_ involve quantization error.

> **Note**: This isn't perfect. In our case we're replacing the first `FRAME_SIZE` samples of `sample_history[]` with
> the values from `frame_in.data[]`. If the largest values of `sample_history[]` were in the first `FRAME_SIZE`
> samples,`sample_history_hr` may underestimate the headroom available. However, because of the way headroom works,
> *under*estimates of headroom are always safe, though they may result in more quantization error than is strictly
> necessary.

Once a new exponent is selected for the sample history, shift values are
calculated for each vector (in the usual way: desired_exponent -
current_exponent), and the vectors' mantissas are shifted. Once the new sampels
are merged into `sample_history[]`, a new headroom for `sample_history[]` is
calculated.

## Results

### Timing

| Timing Type       | Measured Timing
|-------------------|-----------------------
| Per Filter Tap    | 58.82 ns
| Per Output Sample | 60235.73 ns
| Per Frame         | 15604809.00 ns

### Output Waveform

![**Part 3A** Output](img/part3A.png)