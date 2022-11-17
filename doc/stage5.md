
[Prev](stage4.md) | [Home](../intro.md) | [Next](stage6.md)

# Stage 5

Like [**Stage 4**](stage4.md), **Stage 5** implements the FIR filter
using fixed-point arithmetic. 

In **Stage 4**, we called a function named `int32_dot()` to perform the inner
product for us. While `int32_dot()` was much faster than the compiler-generated
inner product we wrote in C in [**Stage 3**](stage3.md), `int32_dot()`
still only uses the xcore device's scalar arithmetic unit.

**Stage 5** replaces `int32_dot()` with a call to
[`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480),
one of the library functions from
[`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)'s low-level vector
API. Unlike `int32_dot()`, `vect_s32_dot()` does use the VPU to do its work.

## Introduction

## Background

## Code Changes

Moving from **Stage 4** to **Stage 5**, we see two minor changes in the code.

From `stage4.c`:
```c
const exponent_t acc_exp = input_exp + coef_exp;

const right_shift_t acc_shr = output_exp - acc_exp;

...

q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  int64_t acc = int32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT);

  return ashr64(acc, acc_shr);
}
```

From `stage5.c`:
```c
const exponent_t acc_exp = input_exp + coef_exp + 30;

const right_shift_t acc_shr = output_exp - acc_exp;

q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  int64_t acc = vect_s32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT, 0, 0);

  return ashr64(acc, acc_shr);
}
```

In **Stage 5** 30 has been added to `acc_exp`. `acc_exp` is the 
exponent associated with the 64-bit accumulator `acc` in `filter_sample()`. In
**Stage 4**, `acc` contains the direct sum of 64-bit products of the sample
history and filter coefficients, and so `acc_exp` is the sum of exponents
associated with those vectors.

However, the XS3 VPU always includes a 30-bit right-shift when multiplying
32-bit numbers, so in **Stage 5**, rather than `acc` holding the direct sum of
64-bit products, it contains the sum of products, each right-shifted 30 bits, so
the associated exponent (implied by the math, whether or not we have an explicit
variable for it) must be adjusted.

Apart from quantization error, shifting an integer right one bit is equivalent
to division by 2. So, to maintain the same logical value of `acc`, the implied
exponent must by incremented by 30.

$$
  P = p \cdot 2^{\hat{p}}             \\
  r = p \gg 30 = \frac{p}{2^{30}}    \\
  P = r \cdot 2^{\hat{p}} \cdot 2^{30} = r \cdot 2^{\hat{p} + 30} \\
  \hat{r} = \hat{p} + 30
$$

The other minor change is that `vect_s32_dot()` is called in `filter_sample()`
instead of `int32_dot()`. 

The prototype for `vect_s32_dot()` is:
```c
int64_t vect_s32_dot(
    const int32_t b[],
    const int32_t c[],
    const unsigned length,
    const right_shift_t b_shr,
    const right_shift_t c_shr);
```

This looks much like  `int32_dot()`, but with two extra parameters, `b_shr` and
`c_shr`. These two parameters a right-shifts which are applied to each element
of the vectors `b[]` and `c[]` respectively _prior_ to multiplication. Many of
the functions in `lib_xcore_math`'s vector API will also take a shift parameter
to be applied to the _results_, but that is usually not the case for functions
which output a scalar.

Pre-shift parameters like `b_shr` and `c_shr` are usually _signed_ arithmetic
right-shifts, meaning negative values can be used to right- or left-shift the
vector elements. This is often important for maintaining arithmetic precision,
for avoiding accumulator saturation, and can also be used to achieve a
particular output exponent. In our case we happen to know that we do not need to
shift any of the inputs.

For full details about `vect_s32_dot()` see `lib_xcore_math`'s [documentation](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480).

## Results

## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
* [`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480)