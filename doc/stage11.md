
[Prev](stage10.md) | [Home](../intro.md) | [Next](stage12.md)

# Stage 11

In [**Stage 10**](stage10.md), `lib_xcore_math`'s[^1] digital filter
API[^2] was used to implement the filter. **Stage 11** also makes use of the FIR
filter API, but indirectly.

In addition to the digital filter API, `lib_xcore_math` also provides a set of
Python scripts[^3] for converting existing digital filters (with floating-point
coefficients) into a form compatible with xcore, and even generates code which
can be directly compiled into the application.

Specifically, for this example the script `gen_fir_filter_s32.py`[^4] was used,
with [`coef.csv`](coef.csv) as input, to generate [`userFilter.c`](userFilter.c)
and [`userFilter.h`](userFilter.h). The script generates a named filter, where
the function names in the generated API are based on the filter name. In this
example, the filter name (specified when calling the script) is "userFilter".

The filters generated with these scripts allocate and manage their own memory,
resulting in very simple API calls.

## Introduction

```
TODO
```

## Background

```
TODO
```

## Generating Filters

The `userFilter.c` and `userFilter.h` used in this stage were provided with this
tutorial as a convenience. You can generate these files yourself using `gen_fir_filter_s32.py`. This will require Python 3 with numpy.

```sh
$ python gen_fir_filter_s32.py -h
usage: gen_fir_filter_s32.py [-h] [--taps TAPS] [--out-dir OUT_DIR] [--input-headroom INPUT_HEADROOM] [--output-headroom OUTPUT_HEADROOM]
                             filter_name filter_coefficients
```

The script requires 2 arguments: the name of the filter, `filter_name`, and a
path to a `.csv` file containing the filter coefficients. The filter
coefficients must be floating-point values with the coefficients separated by
commas and/or whitespace. The coefficient order is `b[0]`, `b[1]`, `b[2]`, etc.

To generate the filter, starting from your workspace root:

```sh
cd xmath_walkthrough/apps/stage11
python ../../lib_xcore_math/lib_xcore_math/script/gen_fir_filter_s32.py --taps 1024 userFilter coef.csv
```

The output should be similar to the following:

```sh
workspace\xmath_walkthrough\apps\stage11$ python ..\..\lib_xcore_math\lib_xcore_math\script\gen_fir_filter_s32.py --taps 1024 userFilter coef.csv
Filter tap count: 1024
Files to be written:
  .\userFilter.h
  .\userFilter.c
```

The `--taps` option is used to ensure that the converted filter has the expected
number of filter taps.


## Code Changes

There are three notable code changes between **Stage 10** and **Stage 11**.

First, [`stage11.c`](stage11.c#L8) includes `userFilter.h` which declares the
generated filter's API:

```c
#include "userFilter.h"
```

Next, `filter_frame()` is much like that in **Stage 10**, but instead of calling
`filter_fir_s32()` and supplying the filter, `userFilter()` is called.
`userFilter()` is essentially just a wrapped call to `filter_fir_s32()`; it
takes a new input sample and returns a new output sample. The underlying
`filter_fir_s32_t` object is allocated and managed within `userFilter.c`.

```c
void filter_frame(
    int32_t sample_buffer[FRAME_OVERLAP])
{
  for(int s = 0; s < FRAME_OVERLAP; s++){
    timer_start();
    sample_buffer[s] = userFilter(sample_buffer[s]);
    timer_stop();
  }
}
```

Finally, in `filter_thread()`, `userFilter_init()` is used to initialize
`userFilter()`. `userFilter_init()` requires no arguments because the
`filter_fir_s32_t` object and all its required information is managed
internally.

```c
void filter_thread(
    chanend_t c_pcm_in, 
    chanend_t c_pcm_out)
{
  int32_t sample_buffer[FRAME_OVERLAP] = {0};

  userFilter_init();
  
  assert(userFilter_exp_diff == 0);

  while(1) {
    for(int k = 0; k < FRAME_OVERLAP; k++)
      sample_buffer[k] = (int32_t) chan_in_word(c_pcm_in);
    
    filter_frame(sample_buffer);

    for(int k = 0; k < FRAME_OVERLAP; k++){
      chan_out_word(c_pcm_out, sample_buffer[k]);
    }
  }
}
```

## Results

```
TODO
```


[^1]: [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
[^2]: [Digital filter API](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/filter.h)
[^3]: [Filter generation scripts](https://github.com/xmos/lib_xcore_math/tree/v2.1.1/lib_xcore_math/script)
[^4]: [`gen_fir_filter_s32.py`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/script/gen_fir_filter_s32.py)