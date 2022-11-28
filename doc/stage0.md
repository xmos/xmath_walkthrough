
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
the thread which actually does the filtering.

The code specific to **Stage 0** is found in [`stage0.c`](TODO). There are three functions involved in the filtering thread and we'll take a look at each below.

> Note: Throughout this tutorial, code presented to the reader will typically have any in-code comments stripped. This is done primarily for the sake of brevity.

Before diving into **Stage 0**'s filter implementation, however, there are
several common bits of code shared by all or most stages. These are found within
`/xmath_walkthrough/apps/common/`.

### `main.xc`

[`main.xc`](TODO) defines the firmware application's entry point. `main()` is
defined in an XC file rather than a C file to make use of the XC language's
simple syntax for allocating channel resources and for bootstrapping the threads
that will be running on each tile.

```c
int main(){
  chan xscope_chan;
  chan c_tile0_to_tile1;
  chan c_tile1_to_tile0;
  chan c_timing;

  par {
    xscope_host_data(xscope_chan);
    on tile[0]: 
    {
      xscope_io_init(xscope_chan);

      printf("Running Application: stage%u\n", STAGE_NUMBER);

      char str_buff[100];
      sprintf(str_buff, OUTPUT_WAV_FMT, STAGE_NUMBER);

      wav_io_thread(c_tile0_to_tile1, 
                    c_tile1_to_tile0, 
                    c_timing, 
                    STAGE_NUMBER,
                    INPUT_WAV, 
                    str_buff);
      _Exit(0);
    }

    on tile[1]: timer_report_task(c_timing);
    on tile[1]: filter_thread(c_tile0_to_tile1, c_tile1_to_tile0);
  }
  return 0;
}
```

Two threads are spawned on `tile[0]`. The first is `xscope_host_data()`, used by
`xscope_fileio` for accessing the host's filesystem. Also on `tile[0]` is
`wav_io_thread()`, which will be described shortly.

> Note: Technically the entry point for the `wav` file processing thread is
> right in `main.xc` (the first statement executed in the spawned thread is
> `xscope_io_init(xscope_chan);`). However, the thread spends almost all its
> time in the `wav_io_thread()` function, so for the sake of conceptual clarity
> I refer to it as the `wav_io_thread()` thread.

On the second tile two more threads are spawned. The filter thread, with
`filter_thread()` as an entry point, is where the actual filtering takes place.
It communicates with the `wav_io_thread()` across tiles using two channel
resources.

Also on `tile[1]` is `timer_report_task()`. This is a trivial thread which
simply waits until the application is almost ready to terminate, and then
reports some timing information (collected by `filter_thread()`) back to
`wav_io_thread()` where the performance info can be written out to a text file.

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

Rather than `filter_thread()` receiving input samples one-by-one, it receives
new input samples in frames. Each frame of input samples will contain
`FRAME_SIZE` new input sample values.

Because the filter thread is receiving input samples in batches of `FRAME_SIZE`
and sending output samples in batches of `FRAME_SIZE`, it also makes sense to
process them as a batch. Additionally, when processing a batch, it is more
convenient and more efficient to store the sample history in a single linear
buffer. That buffer must then be `HISTORY_SIZE` elements long.

### `filter_thread()`

This is the filtering thread's entry point. After initialization this function
loops forever, receiving a frame of input audio, processing it to get output
audio, and then transmitting it back to `tile[0]` to be written to the output
`wav` file.

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
void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  double sample_history[HISTORY_SIZE] = {0};

  double frame_output[FRAME_SIZE] = {0};
```

`sample_history[]` is the buffer which stores the previously received input
samples. The samples in this buffer are stored in reverse chronological order,
with the most recently received sample at `sample_history[0]`.

`frame_output[]` is the buffer into to which computed output samples are placed before being sent back to the wav thread.

```c
  while(1) {
    for(int k = 0; k < FRAME_SIZE; k++){
      const int32_t sample_in = (int32_t) chan_in_word(c_pcm_in);
      const double samp_f = ldexp(sample_in, input_exp);
      sample_history[FRAME_SIZE-k-1] = samp_f;
    }
```

This thread loops until the application is terminated (from `tile[0]`). In each
iteration of the main loop, the first step is to receive `FRAME_SIZE` new input
samples and place them into the sample history buffer (in reverse order).

To pretend `double` samples are being received, as mentioned above, the input
sample values are converted from their integer PCM values into `double`s using
the input exponent `input_exp`.

```c
    filter_frame(frame_output, sample_history);
```

Call `filter_frame()` with the updated `sample_history[]`. `frame_output[]` is
where output samples are to be placed.


```c

    for(int k = 0; k < FRAME_SIZE; k++){
      const double samp_f = frame_output[k];
      const q1_31 sample_out = round(ldexp(samp_f, -output_exp));
      chan_out_word(c_pcm_out, sample_out);
    }
```

For every frame of input audio sent from `tile[0]`, a frame of output audio must
be sent to `tile[0]`. Note that trying to receive any more input samples before
sending the output samples would result in a deadlock because `tile[0]` is
currently blocking on a channel input from `c_pcm_out`.

This converts the computed output samples from `double` back to PCM integers
using `output_exp` and then sends them over `c_pcm_out`.

```c

    memmove(&sample_history[FRAME_SIZE], &sample_history[0], 
            TAP_COUNT * sizeof(double));
  }
}
```

Finally, so as to keep `sample_history[]` fully ordered, the first `TAP_COUNT`
elements of `sample_history[]` are shifted to the end of the buffer, effectively
discarding the `FRAME_SIZE` oldest input samples.


### `filter_frame()`

`filter_frame()` is responsible for computing all of the output sample values
for the current audio frame.

From [`stage0.c`](TODO):
```c
void filter_frame(
    double frame_out[FRAME_SIZE],
    const double history_in[HISTORY_SIZE])
{
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    frame_out[s] = filter_sample(&history_in[FRAME_SIZE-s-1]);
    timer_stop();
  }
}
```

The only thing worth noting here are the calls to `timer_start()` and
`timer_stop()`. These are defined in [`timing.c`](TODO) and are how we are
measuring the performance of our implementation. Each time `timer_start()` is
called a timestamp from xcore.ai's 100 MHz reference clock is captured. When the
corresponding `timer_end()` call is made, another timestamp is captured and the
former is subtracted to get the elapsed clock ticks between `timer_start()` and
`timer_end()`. 

This is done for each computed output sample, and just before the application
terminates, an average is computed and sent to `tile[0]` by
`timer_report_task()`.

### `filter_sample()`

`filter_sample()` is the function where output sample values are computed. It is
where most of the work is actually done.

From [`stage0.c`](TODO):
```c
double filter_sample(
    const double sample_history[TAP_COUNT])
{
  double acc = 0.0;
  for(int k = 0; k < TAP_COUNT; k++)
    acc += sample_history[k] * filter_coef[k];
  return acc;
}
```

Each call to `filter_sample()` produces exactly one output sample, and so a call
to `filter_sample()` corresponds to a particular time step, and for that
requires the most recent `TAP_COUNT` input sample values leading up to (and
including) that time step. The input parameter `sample_history[]` then needs to
point to the main sample history buffer, offset according to the current sample
within the current frame.

A double-precision accumulator is initialized to zero, and the inner product of
the sample history and filter coefficient vectors is computed in a simple `for`
loop.

`filter_coef[]`, the vector of filter coefficients, is defined for this stage in
[`filter_coef_double.c`](../apps/common/filters/filter_coef_double.c). That
coefficient vector is represented by a `TAP_COUNT`-element array of `double`s,
where every element is set to the value $\frac{1}{1024} = 0.0009765625$.

```c
const double filter_coef[TAP_COUNT] = { ... };
```

## Results