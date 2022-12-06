
[Prev](part4A.md) | [Home](intro.md) | [Next](part4C.md)

# Part 4B

**Part 4B** takes a very different approach than all previous stages. **Part 4B** uses the [digital filter API](TODO) provided by `lib_xcore_math`. In this
example the FIR filter will be represented by a [`filter_fir_s32_t`](TODO)
object. The digital filter APIs are highly optimized implementations of 16- and
32-bit FIR filters and 32-bit biquad filters.

The filtering API in `lib_xcore_math` is essentially a fixed-point API, though,
being linear, the actual exponents associated with the input and output samples
are not fixed, only their _relationship_ is. Specifically, it is the
_difference_ between the input and output exponents that is fixed.

### From `lib_xcore_math`

This stage makes use of the following operations from `lib_xcore_math`:

* [`filter_fir_s32_init()`](TODO)
* [`filter_fir_s32()`](TODO)


## Implementation

The implementation for **Part 4B** is divided between 3 functions,
`rx_frame()`, `tx_frame()` and `filter_task()`.

### **Part 4B** `rx_frame()` Implementation

From [`part4B.c`](TODO):
```C
// Accept a frame of new audio data 
static inline 
void rx_frame(
    int32_t buff[],
    const chanend_t c_audio)
{    
  for(int k = 0; k < FRAME_SIZE; k++)
    buff[k] = (q1_31) chan_in_word(c_audio);
}
```

This is nearly identical to the `tx_frame()` found in **Part 2**. The only
difference is that the newly received input samples are placed into `buff[]` in
order, instead of reverse order. We'll see this is because the `filter_s32_t`
object representing the filter handles its own state internally -- we don't need
to account for the ordering of samples ourselves.


### **Part 4B** `tx_frame()` Implementation

From [`part4B.c`](TODO):
```C
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    const int32_t buff[])
{    
  for(int k = 0; k < FRAME_SIZE; k++)
    chan_out_word(c_audio, buff[k]);
}
```

This is identical to the `tx_frame()` found in **Part 2**.


### **Part 4B** `filter_task()` Implementation

From [`part4B.c`](TODO):
```C
/**
 * This is the thread entry point for the hardware thread which will actually 
 * be applying the FIR filter.
 * 
 * `c_audio` is the channel over which PCM audio data is exchanged with tile[0].
 */
void filter_task(
    chanend_t c_audio)
{
  // Exponent associated with input samples
  const exponent_t input_exp = -31;
  // Exponent associated with output samples
  const exponent_t output_exp = -31;
  // Exponent associatd with the filter coefficients
  const exponent_t coef_exp = -30;
  // Right-shift applied by the VPU for 32-bit multiplies
  const right_shift_t vpu_shr = 30;
  // Exponent associated with the accumulator
  const exponent_t acc_exp = input_exp + coef_exp + vpu_shr;

  // The arithmetic right-shift required by the `filter_fir_s32_t` object. This
  // is the number of bits by which the filter's accumulator will be
  // right-shifted to obtain the output.
  const right_shift_t acc_shr = output_exp - acc_exp;
  
  // Buffer used to hold filter state. We do not need to manage the filter state
  // ourselves, but we must give it a buffer. Initializing the filter does not
  // clear the filter state to zeros, so we must do that here.
  int32_t filter_state[TAP_COUNT] = {0};

  // The filter object itself
  filter_fir_s32_t fir_filter;
  
  // This buffer is where input/output samples will be placed.
  int32_t sample_buffer[FRAME_SIZE] = {0};

  // The filter needs to be initialized before supplying it with samples.
  filter_fir_s32_init(&fir_filter, 
                      &filter_state[0], 
                      TAP_COUNT, 
                      &filter_coef[0], 
                      filter_shr);

  // Loop forever
  while(1) {

    // Read in a new frame
    rx_frame(&sample_buffer[0], 
             c_audio);
    
    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start(TIMING_SAMPLE);
      // We can overwrite the data in sample_buffer[] because the filter object
      // will keep track of its own history. So, once we've supplied it with a
      // sample, we can overwrite that memory, allowing us to use the same array
      // for input and output.
      sample_buffer[s] = filter_fir_s32(&fir_filter, 
                                        sample_buffer[s]);
      timer_stop(TIMING_SAMPLE);
    }

    // Send out the processed frame
    tx_frame(c_audio, 
             &sample_buffer[0]);
  }
}
```

There are a couple things to notice before `filter_task()` gets to its loop.

First, we `sample_buffer[]`, where we store input _and_ output samples, is only
`FRAME_SIZE` elements, instead of `HISTORY_SIZE`. That is because the filter
stores the sample history itself.

Next, we now have a `filter_fir_s32_t` object called `fir_filter`.
[`filter_fir_s32_t`](TODO) is defined in `lib_xcore_math` and represents our
32-bit digital FIR filter. It must be initialized before it can be used.

Initialization of the filter is accomplished with a call to `filter_fir_s32_init()`:

> From [`filter.h`](TODO):
> ```c
> C_API
> void filter_fir_s32_init(
>     filter_fir_s32_t* filter,
>     int32_t* sample_buffer,
>     const unsigned tap_count,
>     const int32_t* coefficients,
>     const right_shift_t shift);
> ```

This requires pointers to the filter object itself, the coefficient array, and a
buffer that the filter can use for maintaining the filter state. The filter coefficients we provide are the `Q2.30` coefficients from [`filter_coef_q2_30.c`](TODO), and we declared `sample_buffer[]` to serve as the state buffer.

The parameter `tap_count` is the number of filter taps, and the final parameter
`shift` is an arithmetic right-shift that is applied to the accumulator to
produce a 32-bit output sample.

What output shift value should be used for the filter?  Here we follow the same logic as in [**Part 2**](part2.md).

$$
\begin{aligned}
  \mathtt{input\_exp} &= \mathtt{output\_exp} = -31 \\
  \mathtt{acc\_exp} &= \mathtt{input\_exp} + \mathtt{coef\_exp} + \mathtt{vpu\_shr} \\
  \mathtt{output\_exp} &= \mathtt{acc\_exp} + \mathtt{acc\_shr} \\
  \\
  \mathtt{acc\_shr} &= \mathtt{output\_exp} - \mathtt{acc\_exp} \\
    &= -31 - (\mathtt{input\_exp} + \mathtt{coef\_exp} + \mathtt{vpu\_shr}) \\
    &= -31 - (-31 + -30 + 30) \\
    &= 0
\end{aligned}
$$

Here, because `input_exp = output_exp` and `coef_exp = -vpu_shr`, the exponents
and shifts all cancel out.

The inside of `filter_task()`'s loop is much like it was in **Part 2A** with two
key differences. First, there is no call at the end to `memmove()` to shift the
sample history. Not only does the `filter_fir_s32_t` object handle the filter
state for us, it also internally uses a circular buffer to store samples, so no
shift is ever required.

> **Note**: Of course, we could have been using a circular buffer for our sample
> history in all the previous stages. This would have avoided the expense of
> shifting `TAP_COUNT` samples at the end of every loop iteration, but it also
> noticeably complicates the actual filter implementation, which is why we
> avoided it.

The other difference is that `filter_fir_s32()` is called inside the `for` loop, instead of `filter_sample()`.


> From [`filter.h`](TODO):
> ```c
> C_API
> int32_t filter_fir_s32(
>     filter_fir_s32_t* filter,
>     const int32_t new_sample);
> ```

`filter_fir_s32()` is the function which actually performs the filtering for us.
It takes a pointer to the filter object and the newest input sample value, and
returns the next output sample. We call this with out new input samples in order and we get out our output samples.

## Results

### Timing

| Timing Type       | Measured Timing
|-------------------|-----------------------
| Per Filter Tap    | 4.73 ns
| Per Output Sample | 4845.99 ns
| Per Frame         | 1294021.62 ns

### Output Waveform

![**Part 4B** Output](img/part4B.png)