
[Prev](stage1.md) | [Home](../intro.md) | [Next](partB.md)


# Stage 2

Like [**Stage 1**](stage1.md), **Stage 2** implements the FIR filter using
single-precision floating-point arithmetic. Instead of implementing the inner
product directly with a plain C loop, however, **Stage 2** makes a call to a
library function,
[`vect_f32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_f32.h#L115-L140)
provided by [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math).

Rather than relying on the compiler to produce fast floating-point logic,
`vect_f32_dot()` was hand-written in optimized XS3 assembly to go as quickly as
possible.

Here we will see another significant performance boost.

## Introduction

## Background

* Discuss dual-issue instructions
* Avoiding any unnecessary floating-point error checking and similar

## Code Changes


Instead of computing the result directly in C, we rely on the optimized function
to do the work for us. The
[implementation](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/src/arch/xs3/vect_f32/vect_f32_dot.S)
of `vect_f32_dot()` itself is decidedly more complicated than the plain C
implementation, however it runs much faster.

## Implementation

**Stage 2** uses the exact same implementation as **Stage 1** for
`filter_task()`, `rx_frame()` and `tx_frame()`. The only change is the
implementation of `filter_sample()`.


### Stage 2 `filter_sample()` Implementation

From [`stage2.c`](TODO):
```c
//Apply the filter to produce a single output sample
float filter_sample(
    const float sample_history[TAP_COUNT])
{
  // Return the inner product of sample_history[] and filter_coef[]
  return vect_f32_dot(&sample_history[0], 
                      &filter_coef[0], 
                      TAP_COUNT);
}
```

Whereas **Stage 1** implemented `filter_sample()` by looping over the filter
taps, **Stage 2** instead makes a call to `vect_f32_dot()` from
`lib_xcore_math`.

```C
C_API
float vect_f32_dot(
    const float b[],
    const float c[],
    const unsigned length);
```

`vect_s32_dot()` takes two `float` vectors, `b[]` and `c[]` (each with `length`
elements), multiplies them together element-wise, and then sums up the products.
The result should be the same as the C implementation from **Stage 1**, only a
lot faster.

## Results



## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
* [`vect_f32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_f32.h#L115-L140)