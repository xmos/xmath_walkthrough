
[Prev](partA.md) | [Home](intro.md) | [Next](stage1.md)

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

## Implementation

The [Software Organization](sw_organization.md) page discussed the broad
structure of the firmware. This page will zoom in and take the point-of-view of
the `filter_task` thread which actually does the filtering.

The code specific to **Stage 0** is found in [`stage0.c`](TODO). There are two
main functions involved in the `filter_task` thread and we'll take a look at
each in a moment.

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


### Stage 0 `filter_task()` Implementation


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
  // History of received input samples, stored in reverse-chronological order
  double sample_history[HISTORY_SIZE] = {0};

  // Buffer used to hold output samples
  double frame_output[FRAME_SIZE] = {0};

  // Loop forever
  while(1) {

    // Read in a new frame
    rx_frame(&sample_history[0], 
             c_audio);

    // Calc output frame
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

This is the filtering thread's entry point in **Stage 0**. After initialization
this function loops forever, receiving a frame of input audio, processing it to
get output audio, and then transmitting it back to `tile[0]` to be written to
the output `wav` file.

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
reverse order) (see `rx_frame()` below).

After receiving (and converting) the frame of new samples, `filter_task`
computes `FRAME_SIZE` new output samples each the result of a call to
`filter_sample()`.

Notice the calls to `timer_start()` and `timer_stop()` which happen in that
loop.

[`timer_start()`](TODO) and [`timer_stop()`](TODO) are how the application (and
all subsequent stages) measures the filter performance. For full details, see
[`timing.c`](TODO). The brief explanation is that it uses the device's 100 MHz
reference clock is used to capture timestamps immediately before and after the
new output sample is calculated. This tells us how many 100 MHz clock ticks
elapsed while computing the output sample. The code in `timing.c` ultimately
just computes the average time elapsed across all output samples.

Once the frame of output samples is sent back to `wav_io_task` on `tile[0]` (see
`tx_frame()`), the last thing to do is shift the contents of the
`sample_history[]` buffer to make room for the next frame of input audio. This
allows the samples to stay fully ordered.




### Stage 0 `filter_sample()` Implementation

From [`stage0.c`](TODO):
```c
//Apply the filter to produce a single output sample
double filter_sample(
    const double sample_history[TAP_COUNT])
{
  // Compute the inner product of sample_history[] and filter_coef[]
  double acc = 0.0;
  for(int k = 0; k < TAP_COUNT; k++)
    acc += sample_history[k] * filter_coef[k];
  return acc;
}
```

`filter_sample()` is the function where individual output sample values are
computed. It is where most of the work is actually done.

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



### Stage 0 `rx_frame()` Implementation


From [`stage0.c`](TODO):
```c
// Accept a frame of new audio data 
static inline 
void rx_frame(
    double buff[],
    const chanend_t c_audio)
{    
  // The exponent associated with the input samples
  const exponent_t input_exp = -31;

  for(int k = 0; k < FRAME_SIZE; k++){
    // Read PCM sample from channel
    const int32_t sample_in = (int32_t) chan_in_word(c_audio);
    // Convert PCM sample to floating-point
    const double samp_f = ldexp(sample_in, input_exp);
    // Place at beginning of history buffer in reverse order (to match the
    // order of filter coefficients).
    buff[FRAME_SIZE-k-1] = samp_f;
  }
}
```

`rx_frame()` accepts a frame of audio data over a channel. Note that we would ideally be directly receiving our sample data as `double` values, but that would complicate things for little benefit, so instead we are just converting the received PCM samples into `double` values as we receive them.

`rx_frame()` places the (`double`) sample values into the buffer it was given in
reverse chronological order, as is expected by `filter_sample()`.

The actual input and output sample values going between tiles are all raw
integer values -- just as they're found in the `wav` files. When converting to
`double`, we are not compelled to keep the same scaling. So, if we treat these
PCM values as fixed-point, the exponent associated with the values is more or
less arbitrary. We've chosen `-31` as `input_exp` and `output_exp`, which
means the range of logical sample values is `[-1.0, 1.0)`.

For the sake of allowing output waveforms to be directly compared, we continue
to use an implicit or explicit output exponent of `-31` throughout this
tutorial.

> **Note**: The reason the assumed exponent is arbitrary is because we're
> implementing a _linear_ digital filter. If the filter was not linear (e.g. if
> there was a square root somewhere in the logic) the output values would differ
> depending upon our choice of input exponent.

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




### Stage 0 `tx_frame()` Implementation


From [`stage0.c`](TODO):
```c
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    const double buff[])
{    
  // The exponent associated with the output samples
  const exponent_t output_exp = -31;

  // Send FRAME_SIZE new output samples at the end of each frame.
  for(int k = 0; k < FRAME_SIZE; k++){
    // Get double sample from frame output buffer (in forward order)
    const double samp_f = buff[k];
    // Convert double sample back to PCM using the output exponent.
    const q1_31 sample_out = round(ldexp(samp_f, -output_exp));
    // Put PCM sample in output channel
    chan_out_word(c_audio, sample_out);
  }
}
```

`tx_frame()` mirrors `rx_frame()`. It takes in a frame's worth of
`double`-valued  output audio samples, converts each to the appropriate 32-bit
PCM value, and then sends them to the `wav_io` thread via a channel. Note that unlike `rx_frame()` this function does _not_ reorder the samples being sent.

The logic of converting the `double` values to 32-bit PCM values via the output
exponent is the reverse of that in `rx_frame()`.  Consider the case where we
need to convert the `double` value `0.123456` into a 32-bit fixed-point value
with `31` fractional bits.

$$
\begin{aligned}
  \mathtt{output\_exp} &= -31                                     \\
  \mathtt{samp\_f} &= 0.123456                                    \\
  \\
  \mathtt{sample\_out} &\gets \mathtt{round}
               (\mathtt{ldexp}(0.123456, -\mathtt{output\_exp}))  \\
    &= \mathtt{round}(\mathtt{ldexp}(0.123456, 31))               \\
    &= \mathtt{round}(0.123456 \cdot 2^{31})                      \\
    &= \mathtt{round}(265119741.247488)                           \\
    &= 265119741                                                  \\
\end{aligned}
$$

Now consider the same with the `double` value $-1.0$.

$$
\begin{aligned}
  \mathtt{output\_exp} &= -31                                     \\
  \mathtt{samp\_f} &= -1.0                                        \\
  \\
  \mathtt{sample\_out} &\gets \mathtt{round}
               (\mathtt{ldexp}(-1.0, -\mathtt{output\_exp}))      \\
    &= \mathtt{round}(-1.0 \cdot 2^{31})                          \\
    &= -2147483648                                                \\
    &= \mathtt{INT32\_MIN}                                        \\
\end{aligned}
$$

Finally, consider the same with the `double` value $+1.0$.

$$
\begin{aligned}
  \mathtt{output\_exp} &= -31                                     \\
  \mathtt{samp\_f} &= 1.0                                         \\
  \\
  \mathtt{sample\_out} &\gets \mathtt{round}
               (\mathtt{ldexp}( 1.0, -\mathtt{output\_exp}))      \\
    &= \mathtt{round}( 1.0 \cdot 2^{31})                          \\
    &= 2147483648                                                 \\
    &= \mathtt{INT32\_MAX} + 1                                    \\
\end{aligned}
$$

Whereas $-1.0$ was converted to $\mathtt{INT32\_MIN}$, the least possible `int32_t` value, $+1.0$ was converted to $(\mathtt{INT32\_MAX}+1) = -\mathtt{INT32\_MIN}$, which cannot be represented by a signed 32-bit integer.

And so, using an output exponent of $-31$, the range of floating-point values which can be converted to a 32-bit integer without overflows is $[-1.0, 1.0)$.




## Results