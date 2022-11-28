
[Prev](stage1.md) | [Home](../intro.md) | [Next](stage3.md)


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

The only code change between **Stage 1** and **Stage 2** is the implementation
of the `filter_sample()` function.

From `stage1.c`:
```c
float filter_sample(
    const float sample_history[TAP_COUNT])
{
  float acc = 0.0;
  for(int k = 0; k < TAP_COUNT; k++)
    acc += sample_history[k] * filter_coef[k];
  return acc;
}
```

From `stage2.c`:
```c
float filter_sample(
    const float sample_history[TAP_COUNT])
{
  return vect_f32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT);
}
```

Instead of computing the result directly in C, we rely on the optimized function
to do the work for us. The
[implementation](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/src/arch/xs3/vect_f32/vect_f32_dot.S)
of `vect_f32_dot()` itself is decidedly more complicated than the plain C
implementation, however it runs much faster.

## Results



## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
* [`vect_f32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_f32.h#L115-L140)