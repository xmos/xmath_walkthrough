
[Prev](stage9.md) | [Home](../intro.md) | [Next](stage11.md)

# Stage 10

**Stage 10** takes a very different approach than previous stages. **Stage 10**
uses the digital filter API [^1] provided by `lib_xcore_math`[^2]. In this
example the FIR filter will be represented by a `filter_fir_s32_t`[^3] object.
The digital filter APIs are highly optimized implementations of 16- and 32-bit
FIR filters and 32-bit biquad filters.

## Introduction

```
TODO
```

## Background

```
TODO
```

## Code Changes

The implementation for **Stage 10** is divided between `filter_task()` and 
`filter_frame()`.

From [`stage10.c`](stage10.c):
```c
void filter_task(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  int32_t filter_state[TAP_COUNT] = {0};
  filter_fir_s32_t fir_filter;
  
  int32_t sample_buffer[FRAME_SIZE] = {0};

  filter_fir_s32_init(&fir_filter, &filter_state[0], 
                      TAP_COUNT, &filter_coef[0], filter_shr);

  while(1) {
    for(int k = 0; k < FRAME_SIZE; k++)
      sample_buffer[k] = (int32_t) chan_in_word(c_pcm_in);
    
    filter_frame(&fir_filter, sample_buffer);

    for(int k = 0; k < FRAME_SIZE; k++){
      chan_out_word(c_pcm_out, sample_buffer[k]);
    }
  }
}
```

`filter_task()` looks much like that in prior stages. Notably, the
`sample_history` buffer has been removed. In its place are `filter_state` and
`fir_filter`. `fir_filter` is the `filter_fir_s32_t` object which represents our
FIR filter. `filter_state` is a buffer required by `fir_filter` to manage the
filter's state.

Before applying the filter using a `filter_fir_s32_t`, the object must be
initialized with a call to `fir_filter_s32_init()`[^4]. The primary job of
`fir_filter_s32_init()` is to tie the filter to a user-provided state buffer and
to the user-provided coefficient array.

From [`stage10.c`](stage10.c):
```c
void filter_frame(
    filter_fir_s32_t* filter,
    int32_t sample_buffer[FRAME_SIZE])
{
  for(int s = 0; s < FRAME_SIZE; s++){
    timer_start();
    sample_buffer[s] = filter_fir_s32(filter, sample_buffer[s]);
    timer_stop();
  }
}
```

`filter_frame()` is especially simple. `filter_fir_s32()`[^5] is the library function which actually applies the filter. Its prototype is:

```c
int32_t filter_fir_s32(
    filter_fir_s32_t* filter,
    const int32_t new_sample);
```

`filter` is the `filter_fir_s32_t` object representing the filter to be applied.
`new_sample` is the newly received input sample. The new output sample is
returned.

In `filter_frame()`, each new input sample of the newly received frame is
supplied to the filter, and is replaced in `sample_buffer[]` by the returned
output sample.

## Results

```
TODO
```

[^1]: [Digital filter API](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/filter.h)
[^2]: [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
[^3]: [`filter_fir_s32_t`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/filter.h#L19-L275)
[^4]: [`filter_fir_s32_init()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/filter.h#L278-L307)
[^5]: [`filter_fir_s32()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/filter.h#L329-L350)