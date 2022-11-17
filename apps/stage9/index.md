
[Prev](../stage8/index.md) | [Home](../intro.md) | [Next](../stage10/index.md)

# Stage 9

Up to this point, all of the previous stages do all of their filter computation
in a single hardware thread. Each tile on an xcore XS3 device has 8 hardware
threads available, and most of these threads have so far gone unused. In **Stage
9** the application will be parallelized so that the filter computation will span
several threads to reduce latency.

**Stage 9** will take a block floating-point approach similar to that of
[**Stage 7**](../stage7/index.md). The reason this stage is modeled on **Stage
7** instead of [**Stage 8**](../stage8/index.md) is because `lib_xcore_math`'s
high-level BFP API will not play nicely with this sort of parallelism. For
advanced usage like this, the lower level API is required.

## Introduction

## Background

## Code Changes

The first thing to notice is that **Stage 9** has its implementation split
between two files, `stage9.c` and `stage9.xc`. `filter_thread()` and
`filter_frame()` (in `stage9.c`) are unchanged between **Stage 7** and **Stage
9**. `filter_sample()` is implemented in `stage9.xc`.

The reason `filter_sample()` is implemented in an XC file in **Stage 9** is that
this allows it to take advantage of the XC language's convenient syntax for 
parallel operations, the `par` block.

From `stage7.c`:
```c
q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr)
{
  float_s64_t acc;
  acc.mant = 0;

  const headroom_t coef_hr = HR_S32(filter_coef[0]);

  right_shift_t b_shr, c_shr;
  vect_s32_dot_prepare(&acc.exp, &b_shr, &c_shr, 
                        history_exp, coef_exp,
                        history_hr, coef_hr, TAP_COUNT);

  acc.mant = vect_s32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT, 
                        b_shr, c_shr);

  q1_31 sample_out = float_s64_to_fixed(acc, output_exp);

  return sample_out;
}
```

From `stage9.xc`:
```c
q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr)
{
  float_s64_t acc;

  headroom_t coef_hr = HR_S32(filter_coef[0]);

  right_shift_t b_shr, c_shr;
  vect_s32_dot_prepare(&acc.exp, &b_shr, &c_shr, 
                        history_exp, coef_exp,
                        history_hr, coef_hr, TAP_COUNT);

  int64_t partial[THREADS];

  const unsigned elms = (TAP_COUNT / THREADS);

  par(int tid = 0; tid < THREADS; tid++)
  {
    partial[tid] = vect_s32_dot(&sample_history[elms * tid],
                                &filter_coef[elms * tid],
                                elms, b_shr, c_shr);
  }

  acc.mant = 0;
  for(int k = 0; k < 4; k++)
    acc.mant += partial[k];

  q1_31 sample_out = float_s64_to_fixed(acc, output_exp);

  return sample_out;
}
```

In **Stage 9**, all of the parallelism happens within the `par`-block.
Semantically, and conceptually, this works much like a `for`-loop. The
difference is, of course, that instead of each "iteration" happening
sequentially, with the `par`-block each "iteration" happens simultaneously.

In this example the work of computing the inner product is split across
`THREADS` (4) hardware threads. The filter output is the inner product of the
two 1024-element vectors `sample_history[]` and `filter_coef[]`. Each of the
worker threads in this example will be responsible for computing the inner
product of 256-element sub-vectors, and the results will be summed together when
control is returned to the caller thread.

When parallelizing block floating-point arithmetic, care must be taken to ensure
each thread agrees about the output representation. The call to
`vect_s32_dot_prepare()` in **Stage 9** happens in the caller thread prior to
forking into worker threads. This way each thread is guaranteed to use the same
`b_shr` and `c_shr` parameters, and thus use the same output exponent.

Inside the `par`-block, the threads compute their portion of the result. If each
worker thread were to directly add their partial result to `acc.mant` a race
condition would be created which would likely clobber the result. Instead, the
workers place their partial result in `partial[]`. There is an implicit
synchronization point at the end of the `par`-block, where the caller thread
does not resume execution of `filter_sample()` until all workers have completed
their work. Finally the partial results are summed back together in the caller
thread.

> Note: "Caller" thread and "worker" thread in the above are conceptual
> descriptions. The hardware thread from which `filter_sample()` was called is
> in fact used as one of the "worker" threads inside the `par`-block. That is,
> the caller thread does not *sleep* during the `par`-block, it becomes one of
> the worker threads.

## Results

## Misc

> Note: One alternative approach to parallelizing the filter in **Stage 9**
> would have been to have each worker thread computing a different output
> sample, rather than different portions of the same output sample. While this
> approach should have roughly equivalent throughput to the one actually taken
> in **Stage 9**, the actual per-sample latency would be the same as in **Stage
> 7**.

## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)