
[Prev](filter.md) | [Home](intro.md) | [Next](part1.md)

# Common Code

Before diving into stage-specific code, it is worth taking a brief look at some
of the common code.

All code common to all parts and stages can be found in the
[`src/common/`](TODO) directory. However, note that each stage uses only _one_
of the source files in [`src/common/filters/`](TODO).

Note that each stage of this tutorial will assume that the user has read the
previous parts. This will help avoid reiterating information that was already
said.

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

      printf("Running Application: %s\n", APP_NAME);

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

On the second tile, `tile[1]` two more threads are spawned. The filter thread,
with `filter_task()` as an entry point, is where the actual filtering takes
place. It communicates with `wav_io_task` across tiles using a channel resource.

Also on `tile[1]` is `timer_report_task()`. This is a trivial thread which
simply waits until the application is almost ready to terminate, and then
reports some timing information (collected by `filter_task`) back to
`wav_io_task` where the performance info can be written out to a text file.

The macros `APP_NAME`, `INPUT_WAV`, `OUTPUT_WAV` and `OUTPUT_JSON` are all defined it the particular stage's `CMakeLists.txt`.

### `common.h`

The [`common.h`](TODO) header includes several other boilerplate headers, and
then defines several macros required by most stages.

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

### `misc_func.h`

The [`misc_func.h`](TODO) header contains several simple inline scalar functions required in various places throughout the tutorial. Future versions of `lib_xcore_math` will likely contain implementations of these functions, but for now our application has to define them.

### `filters/`

The [`src/common/filters/`](TODO) directory contains 4 source files, each of which defines the same filter coefficients, but in different formats. Each stage's firmware uses only _one_ of these filters.

#### `filter_coef_double.c` 

[`filter_coef_double.c`](TODO) contains the coefficients as an array of `double` elements:

```C
const double filter_coef[TAP_COUNT] = {...};
```

#### `filter_coef_float.c` 

[`filter_coef_float.c`](TODO) contains the coefficients as an array of `float` elements:

```C
const float filter_coef[TAP_COUNT] = {...};
```

#### `filter_coef_q2_30.c` 

[`filter_coef_q2_30.c`](TODO) contains the coefficients as an array of `q2_30` elements. `q2_30` is a fixed-point type defined in `lib_xcore_math`. It will be described in more detail later.

```C
const q2_30 filter_coef[TAP_COUNT] = {...};
```

#### `filter_coef_q4_28.c` 

[`filter_coef_q4_28.c`](TODO) contains the coefficients as an array of `q4_28` elements. `q4_28` is a fixed-point type defined in `lib_xcore_math`. It will be described in more detail later.

```C
const q4_28 filter_coef[TAP_COUNT] = {...};
```

### Timing

Filter (time) performance is measured through the timing module in [`timing.h`](TODO). The implementation of `timing.c` is not important. What you will find in each stage's implementation are the following four calls:

```c
timer_start(TIMING_SAMPLE);
```
```c
timer_stop(TIMING_SAMPLE);
```
```c
timer_start(TIMING_FRAME);
```
```c
timer_stop(TIMING_FRAME);
```

You'll find the pair `timer_start(TIMING_SAMPLE)` and `timer_stop(TIMING_SAMPLE)` surrounding the calls to `filter_sample()` (more on `filter_sample()` later).

They are how the application (and all subsequent stages) measures the per-sample
filter performance. The device's 100 MHz reference clock is used to capture
timestamps immediately before and after the new output sample is calculated
(when `TIMING_SAMPLE` is used). This tells us how many 100 MHz clock ticks
elapsed while computing the output sample. The code in `timing.c` ultimately
just computes the average time elapsed across all output samples.

Because the per-sample timing misses a lot of the required processing, in each
stage you will also find calls to `timer_start(TIMING_FRAME)` and
`timer_stop(TIMING_FRAME)` in `rx_frame()` and `tx_frame()` respectively. This
pair (when `TIMING_FRAME` is used) measure the time taken to process the entire
frame.
