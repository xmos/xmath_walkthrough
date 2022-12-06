
[Prev](part1.md) | [Home](intro.md) | [Next](part1B.md)

# Part 1A

**Part 1A** is our first implementation of the digital FIR filter. Much of the
code found in subsequent stages will resemble the code found here.

In **Part 1A** the FIR filter is implemented in a very simple but not very
efficient way. The input samples, filter coefficients and output samples are all
`double` values. All filter arithmetic is done in double-precision, which
maximizes the arithmetic precision, but comes at a steep performance cost, as
not much can be done to accelerate this work in hardware.

Additionally, the filter logic in this stage is implemented in plain C. This is
a useful starting point because many users approach xcore.ai with their
algorithms implemented in plain C for the sake of portability.

Using `double` arithmetic to implement the filter on a low cost microcontroller
is an unrealistically bad idea. Hence, it is not an ideal anchor to judge the
performance of the implementations in subsequent stages against (**Part 1B**
provides a better reference). However, it is still useful to demonstrate just
how large a penalty the application suffers for this.

## Implementation

The [Software Organization](sw_organization.md) page discussed the broad
structure of the firmware. [Common Code](common.md) briefly reviews some of the
elements common to all stages. This page will zoom in and take the point-of-view
of the `filter_task` thread in **Part 1A**, which actually does the filtering.

The code specific to **Part 1A** is found in [`part1A.c`](TODO).

**Part 1A** uses the filter coefficients from [`filter_coef_double.c`](TODO).

### Part 1A `filter_task()` Implementation

From [`part1A.c`](TODO):
```c
/**
 * This is the thread entry-point for the hardware thread which will actually 
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
      timer_start(TIMING_SAMPLE);
      frame_output[s] = filter_sample(&sample_history[FRAME_SIZE-s-1]);
      timer_stop(TIMING_SAMPLE);
    }

    // Make room for new samples at the front of the vector
    memmove(&sample_history[FRAME_SIZE], 
            &sample_history[0], 
            TAP_COUNT * sizeof(double));

    // Send out the processed frame
    tx_frame(c_audio, 
             &frame_output[0]);
  }
}
```

This is the filtering thread's entry-point. After initialization this function
loops forever, receiving a frame of input audio, processing it to get output
audio, and then transmitting it back to `tile[0]` to be written to the output
`wav` file.

`filter_task()` takes a channel end resource as a parameter. This is how it 
communicates with `wav_io_task`.

`sample_history[]` is the buffer which stores the previously received input
samples. The samples in this buffer are stored in reverse chronological order,
with the most recently received sample at `sample_history[0]`.

`frame_output[]` is the buffer into to which computed output samples are placed
before being sent to `wav_io_task`.

The `filter_task` thread loops until the application is terminated (via
`tile[0]`). In each iteration of its main loop, the first step is to receive
`FRAME_SIZE` new input samples and place them into the sample history buffer (in
reverse order) (see `rx_frame()` below).

After receiving (and converting) the frame of new samples, `filter_task`
computes `FRAME_SIZE` new output samples, each the result of a call to
`filter_sample()`.

The last thing to do before sending the frame of computed output samples back to
`wav_io_task` on `tile[0]` (see `tx_frame()`) is to shift the contents of the
`sample_history[]` buffer to make room for the next frame of input audio. This
allows the samples to stay fully ordered.

Finally, the output frame is sent to `tile[0]` and it repeats the loop.



### Part 1A `filter_sample()` Implementation

From [`part1A.c`](TODO):
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

`filter_coef[]`, the vector of filter coefficients, is represented by a
`TAP_COUNT`-element array of `double`s, where every element is set to the value
$\frac{1}{1024} = 0.0009765625$.

```c
const double filter_coef[TAP_COUNT] = { ... };
```

### Stage 0 `rx_frame()` Implementation

From [`part1A.c`](TODO):
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

  timer_start(TIMING_FRAME);
}
```

`rx_frame()` accepts a frame of audio data over a channel. Note that to
demonstrte a fully floating-point implementation, we would ideally be directly
receiving our sample data as `double` values, but that would complicate things
for little benefit, so instead we are just converting the received PCM samples
into `double` values as we receive them.

`rx_frame()` places the (`double`) sample values into the buffer it was given in
reverse chronological order, as is expected by `filter_sample()`.


### Stage 0 `tx_frame()` Implementation


From [`part1A.c`](TODO):
```c
// Send a frame of new audio data
static inline 
void tx_frame(
    const chanend_t c_audio,
    const double buff[])
{    
  // The exponent associated with the output samples
  const exponent_t output_exp = -31;

  timer_stop(TIMING_FRAME);

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
PCM value, and then sends them to the `wav_io` thread via a channel. Note that
unlike `rx_frame()` this function does _not_ reorder the samples being sent.


## Results

### Timing

| Timing Type       | Measured Timing
|-------------------|-----------------------
| Per Filter Tap    | 2469.46 ns
| Per Output Sample | 2528.72525 us
| Per Frame         | 647.43289600 ms

### Output Waveform

![**Part 1A** Output](img/part1A.png)