
[Prev](sw_organization.md) | [Home](intro.md) | [Next](stage1.md)

# Stage 0

**Stage 0** is our first implementation of the digital FIR filter. Much of the
code found in subsequent stages will resemble the code found here, so we will go
through the code in some detail.

### The Filter

The filter to be implemented is a 1024-tap digital FIR box filter in which the
filter output is the simple average of the most recent 1024 input samples. The
behavior of this filter is not particularly interesting, but here we are
concerned with the performance details of each implementation, not the filter
behavior, so a simple filter will do.

The output sample, $y[k]$, of an FIR filter at discrete time step $k$ given
input sample $x[k]$ is defined to be

$$
  y[k] := \sum_{n=0}^{N-1} x[k-n] \cdot b[n]
$$

where $N$ is the number of filter taps and $b[n]$ (with $n \in
\{0,1,2,...,N-1\}$) is the vector of filter coefficients.

The filter coefficient $b[n]$ has lag $n$. That is, the filter coefficient
$b[0]$ is always multiplied by the newest input sample at time $k$, $b[1]$ is
multiplied by the previous input sample, and so on.

The filter used throughout this tutorial has $N$ fixed at $1024$ taps. In our
case, because the filter is a simple box averaging filter, all $b[n]$ will be
the same value, namely $\frac{1}{N}$. So

$$
  b[] = \left[\frac{1}{N}, \frac{1}{N}, \frac{1}{N}, \frac{1}{N}, ..., \frac{1}{N}\right]
$$

## Introduction

In **Stage 0** the FIR filter is implemented in a very simple but inefficient
way. The input samples, filter coefficients and output samples are all `double`
values. All filter arithmetic is done in double-precision, which maximizes the
arithmetic precision, but comes at a steep performance cost, as not much can be
done to accelerate this work in hardware.

Additionally, the filter logic in this stage is implemented in plain C. This is
a useful starting point because many users approach xcore.ai with their
algorithms implemented in plain C for the sake of portability.

This stage is "0" rather than "1" because using `double` arithmetic to implement
the filter on a low cost microcontroller is an unrealistically bad idea. Hence,
it is not an ideal anchor to judge the performance of the implementations in
subsequent stages against (**Stage 1** provides a better reference). However, it
is still useful to demonstrate just how large a penalty the application suffers
for this.

## Background

* IEEE 754 Floating-Point Values and Representation
* Single-Precision and Double-Precision

## Code Overview

The [Software Organization](sw_organization.md) page discussed the broad
structure of the firmware. This page will zoom in and take the point-of-view of
the `filter_task` thread which actually does the filtering.

The code specific to **Stage 0** is found in [`stage0.c`](TODO). There are two
main functions involved in the `filter_task` thread and we'll take a look at
each in a moment.

> Note: Throughout this tutorial, code presented to the reader will sometimes
> have any in-code comments stripped. This is done primarily for the sake of
> brevity.

Before diving into **Stage 0**'s filter implementation, there are several common
bits of code shared by all or most stages. These are found within the
`/xmath_walkthrough/src/common/` directory.

### `main.xc`

[`main.xc`](TODO) defines the firmware application's entry point. `main()` is
defined in an XC file rather than a C file to make use of the XC language's
convenient syntax for allocating channel resources and for bootstrapping the
threads that will be running on each tile.

```c
int main(){
  // Channel used for communicating audio data between tile[0] and tile[1].
  chan c_audio_data;
  // Channel used for reporting timing info from tile[1] after the signal has
  // been processed.
  chan c_timing;

  par {
    // One thread runs on tile[0]
    on tile[0]: 
    {
      // Called so xscope will be used for prints instead of JTAG.
      xscope_config_io(XSCOPE_IO_BASIC);

      printf("Running Application: stage%u\n", STAGE_NUMBER);

      // This is where the app will spend all its time.
      wav_io_task(c_audio_data, 
                  c_timing, 
                  INPUT_WAV,    // These three macros are defined per-target
                  OUTPUT_WAV,   // in the CMake project.
                  OUTPUT_JSON);
      
      // Once wav_io_task() returns we are done.
      _Exit(0);
    }

    // Two threads on tile[1].
    // tiny task which just sits waiting to report timing info back to tile[0]
    on tile[1]: timer_report_task(c_timing);
    // The thread which does the signal processing.
    on tile[1]: filter_task(c_audio_data);
  }
  return 0;
}
```

One thread is spawned on `tile[0]`, `wav_io_task()`, which handles all file IO
and breaking the input and output signal up into frames of audio.

On the second tile two more threads are spawned. The filter thread, with
`filter_task()` as an entry point, is where the actual filtering takes place. It
communicates with `wav_io_task` across tiles using a channel resource.

Also on `tile[1]` is `timer_report_task()`. This is a trivial thread which
simply waits until the application is almost ready to terminate, and then
reports some timing information (collected by `filter_task`) back to
`wav_io_task` where the performance info can be written out to a text file.

### `common.h`

The [`common.h`](TODO) header includes several other boilerplate headers, and
then defines several macros required by every stage.

From [`common.h`](TODO):
```c
#define TAP_COUNT     (1024)
#define FRAME_SIZE    (256)
#define HISTORY_SIZE  (TAP_COUNT + FRAME_SIZE)
```

`TAP_COUNT` is the order of the digital FIR filter being implemented. It is the
number of filter coefficients we have. It is the number of multiplications and
additions necessary (arithmetically speaking) to compute the filter output. And
it is also the minimum input sample history size needed to compute the filter
output for a particular time step.

Rather than `filter_task` receiving input samples one-by-one, it receives new
input samples in frames. Each frame of input samples will contain `FRAME_SIZE`
new input sample values.

Because the filter thread is receiving input samples in batches of `FRAME_SIZE`
and sending output samples in batches of `FRAME_SIZE`, it also makes sense to
process them as a batch. Additionally, when processing a batch, it is more
convenient and more efficient to store the sample history in a single linear
buffer. That buffer must then be `HISTORY_SIZE` elements long.

### `filter_task()`

This is the filtering thread's entry point in **Stage 0**. After initialization
this function loops forever, receiving a frame of input audio, processing it to
get output audio, and then transmitting it back to `tile[0]` to be written to
the output `wav` file.

> **Note**: In **Stage 0** to keep things 'clean' we would prefer to be
> receiving `double`-valued samples directly over the channel from `tile[0]`.
> However, refactoring the application to achieve this would ultimately result
> in more complicated software, rather than less. So, in Part A of this tutorial
> where we work with floating-point arithmetic, we're simply _pretending_ that
> we are receiving the input frame as `double` values by converting each sample
> as it comes in. Likewise, a similar conversion happens with output samples.
> 
> The actual input and output sample values going between tiles are all
> fixed-point integer values -- just as they're found in the `wav` files. The
> exponent associated with these fixed-point values is more or less arbitrary,
> so we've chosen `-31` as `input_exp` and `output_exp`, which means the range
> of logical sample values is `[-1.0, 1.0)`.
>
> For the sake of allowing output waveforms to be directly compared, we continue
> to use an implicit or explicit output exponent of `-31` throughout this
> tutorial.

From [`stage0.c`](TODO):
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
  // History of received input samples, stored in reverse-chronological order.
  double sample_history[HISTORY_SIZE] = {0};

  // Buffer used to hold output samples before they're transferred to tile[0].
  double frame_output[FRAME_SIZE] = {0};

  // Loop forever
  while(1) {
    // Read in a new frame. It is placed in reverse order at the beginning of
    // sample_history[]
    read_frame_as_double(&sample_history[0], c_audio, input_exp, FRAME_SIZE);
    
    // Compute FRAME_SIZE output samples.
    for(int s = 0; s < FRAME_SIZE; s++){
      timer_start();
      frame_output[s] = filter_sample(&sample_history[FRAME_SIZE-s-1]);
      timer_stop();
    }

    // Send out the processed frame
    send_frame_as_double(c_audio, &frame_output[0], output_exp, FRAME_SIZE);

    // Finally, shift the sample_history[] buffer up FRAME_SIZE samples.
    // This is required to maintain ordering of the sample history.
    memmove(&sample_history[FRAME_SIZE], &sample_history[0], 
            TAP_COUNT * sizeof(double));
  }
}
```

`filter_task()` takes a channel end resource as a parameter. This is how it 
communicates with `wav_io_task`.

`sample_history[]` is the buffer which stores the previously received input
samples. The samples in this buffer are stored in reverse chronological order,
with the most recently received sample at `sample_history[0]`.

`frame_output[]` is the buffer into to which computed output samples are placed
before being sent to `wav_io_task`.

The `filter_task` thread loops until the application is terminated (via
`tile[0]`). In each iteration of the its main loop, the first step is to receive
`FRAME_SIZE` new input samples and place them into the sample history buffer (in
reverse order).

The [`read_frame_as_double()`](TODO) and [`send_frame_as_double()`](TODO)
functions, both defined in [`misc_func.h`](TODO), are helper functions which
transfer a frame's worth of samples from/to `wav_io_task`. Here is
`read_frame_as_double()`'s implementation:

From [`misc_func.h`](TODO):
```c
static inline 
void read_frame_as_double(
    double buff[],
    const chanend_t c_audio,
    const exponent_t input_exp,
    const unsigned frame_size)
{    
  for(int k = 0; k < frame_size; k++){
    // Read PCM sample from channel
    const int32_t sample_in = (int32_t) chan_in_word(c_audio);
    // Convert PCM sample to floating-point
    const double samp_f = ldexp(sample_in, input_exp);
    // Place at beginning of history buffer in reverse order (to match the
    // order of filter coefficients).
    buff[frame_size-k-1] = samp_f;
  }
}
```

They also perform the conversion from the PCM sample values to `double` sample
values. Notice that `buff[]` is being filled in reverse order. 

The conversion requires an exponent to establish the scale of the fixed-point
values. Here both `input_exp` and `output_exp` are `-31`.

To see the logic of this conversion, consider a 32-bit PCM sample being received
with a value of `0x2000000`. `ldexp()` is used to perform the conversion as

$$
\begin{aligned}
  \mathtt{samp\_f} &\gets \mathtt{ldexp(0x20000000, input\_exp)}  \\
   &= (\mathtt{0x20000000} \cdot 2^{\mathtt{input\_exp}})         \\
   &= 2^{29} \cdot 2^{\mathtt{input\_exp}}                        \\
   &= 2^{(29 + \mathtt{input\_exp})}                              \\
   &= 2^{(29 - 31)}                                               \\
   &= 2^{-2}                                                      \\
   &= 0.25
\end{aligned}
$$

And so the 32-bit PCM value `0x20000000` becomes the `double` value `0.25`.


After receiving (and converting) the frame of new samples, `filter_task`
computes `FRAME_SIZE` new output samples each the result of a call to
`filter_sample()`, which we'll see shortly.

Notice the calls to `timer_start()` and `timer_stop()` which happen in that
loop.

[`timer_start()`](TODO) and [`timer_stop()`](TODO) are how the application (and
all subsequent stages) measures the filter performance. For full details, see
[`timing.c`](TODO). The brief explanation is that it uses the device's 100 MHz
reference clock is used to capture timestamps immediately before and after the
new output sample is calculated. This tells us how many 100 MHz clock ticks
elapsed while computing the output sample. The code in `timing.c` ultimately
just computes the average time elapsed across all output samples.

Once the frame of output samples is sent back to `wav_io_task` on `tile[0]`, the
last thing to do is shift the contents of the `sample_history[]` buffer to make
room for the next frame of input audio. This allows the samples to stay fully
ordered.


### `filter_sample()`

`filter_sample()` is the function where individual output sample values are
computed. It is where most of the work is actually done.

From [`stage0.c`](TODO):
```c
/**
 * Apply the filter to produce a single output sample.
 * 
 * `sample_history[]` contains the `TAP_COUNT` most-recent input samples, with
 * the newest samples first (reverse time order).
 */
double filter_sample(
    const double sample_history[TAP_COUNT])
{
  // The filter result is the simple inner product of the sample history and the
  // filter coefficients. Because we've stored the sample history in reverse
  // time order, the indices of sample_history[] match up with the indices of
  // filter_coef[].
  double acc = 0.0;
  for(int k = 0; k < TAP_COUNT; k++)
    acc += sample_history[k] * filter_coef[k];
  return acc;
}
```

Each call to `filter_sample()` produces exactly one output sample, and so a call
to `filter_sample()` corresponds to a particular time step, and that requires
the most recent `TAP_COUNT` input sample values leading up to (and including)
that time step. The input parameter `sample_history[]` then needs to point to
the main sample history buffer, offset according to the current sample within
the current frame.

A double-precision accumulator is initialized to zero, and the inner product of
the sample history and filter coefficient vectors is computed in a simple `for`
loop.

`filter_coef[]`, the vector of filter coefficients, is defined for this stage in
[`filter_coef_double.c`](../src/common/filters/filter_coef_double.c). That
coefficient vector is represented by a `TAP_COUNT`-element array of `double`s,
where every element is set to the value $\frac{1}{1024} = 0.0009765625$.

```c
const double filter_coef[TAP_COUNT] = { ... };
```

## Results