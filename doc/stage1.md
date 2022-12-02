
[Prev](stage0.md) | [Home](../intro.md) | [Next](stage2.md)

# Stage 1

Like [**Stage 0**](stage0.md), **Stage 1** implements the FIR filter using
floating-point logic written in a plain C loop. Instead of using
double-precision floating-point values (`double`) like **Stage 0**, **Stage 1**
uses single-precision floating-point values (`float`).

In this stage we'll see that we get a large performance boost just by using
single-precision floating-point arithmetic (for which xcore.ai has hardware
support) instead of double-precision arithmetic.

## Introduction

## Background

* Single-precision vs double-precision
* xcore.ai support for floating-point

## Implementation

Comparing the code in `stage1.c` to the code from `stage0.c`, we see that the
only real difference is that all `double` values have been replaced with `float`
values.

Similarly, the **Stage 1** application includes `filter_coef_float.c` (rather
than `filter_coef_double.c`) which defines `filter_coef[]` as a vector of
`float` values.


### Stage 1 `filter_task()` Implementation

From [`stage1.c`](TODO):
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
  // History of received input samples, stored in reverse-chronological order
  float sample_history[HISTORY_SIZE] = {0};

  // Buffer used to hold output samples
  float frame_output[FRAME_SIZE] = {0};
  
  // Loop forever
  while(1) {

    // Read in a new frame
    rx_frame(&sample_history[0], 
             c_audio);

    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start();
      frame_output[s] = filter_sample(&sample_history[FRAME_SIZE-s-1]);
      timer_stop();
    }

    // Send out the processed frame
    tx_frame(c_audio, 
             &frame_output[0]);

    // Make room for new samples at the front of the vector
    memmove(&sample_history[FRAME_SIZE], 
            &sample_history[0], 
            TAP_COUNT * sizeof(double));
  }
}
```

### Stage 1 `filter_sample()` Implementation

From [`stage1.c`](TODO):
```c
//Apply the filter to produce a single output sample
float filter_sample(
    const float sample_history[TAP_COUNT])
{
  // Compute the inner product of sample_history[] and filter_coef[]
  float acc = 0.0;
  for(int k = 0; k < TAP_COUNT; k++)
    acc += sample_history[k] * filter_coef[k];
  return acc;
}
```

### Stage 1 `rx_frame()` Implementation

From [`stage1.c`](TODO):
```c
// Accept a frame of new audio data 
static inline 
void rx_frame(
    float buff[],
    const chanend_t c_audio)
{    
  // The exponent associated with the input samples
  const exponent_t input_exp = -31;

  for(int k = 0; k < FRAME_SIZE; k++){
    // Read PCM sample from channel
    const int32_t sample_in = (int32_t) chan_in_word(c_audio);
    // Convert PCM sample to floating-point
    const float samp_f = ldexpf(sample_in, input_exp);
    // Place at beginning of history buffer in reverse order (to match the
    // order of filter coefficients).
    buff[FRAME_SIZE-k-1] = samp_f;
  }
}
```

### Stage 1 `tx_frame()` Implementation

From [`stage1.c`](TODO):
```c
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    const float buff[])
{    
  // The exponent associated with the output samples
  const exponent_t output_exp = -31;

  // Send FRAME_SIZE new output samples at the end of each frame.
  for(int k = 0; k < FRAME_SIZE; k++){
    // Get float sample from frame output buffer (in forward order)
    const float samp_f = buff[k];
    // Convert float sample back to PCM using the output exponent.
    const q1_31 sample_out = roundf(ldexpf(samp_f, -output_exp));
    // Put PCM sample in output channel
    chan_out_word(c_audio, sample_out);
  }
}
```

## Results