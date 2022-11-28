
[Prev](stage6.md) | [Home](../intro.md) | [Next](stage8.md)

# Stage 7

Like [**Stage 6**](stage6.md), **Stage 7** implements the FIR filter 
using block floating-point arithmetic.

In **Stage 6** we implemented the block floating-point FIR filter using plain C.
We had to manage exponents and headroom ourselves, as well as the logic for
computing the inner product, which was similar to [**Stage
3**](stage3.md).

In **Stage 7** we will replace our C code which calculates and manages headroom
and exponents with calls to functions from
[`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)'s low-level vector
API to do some of this work for us. We also replace our `for` loop where the
inner product is computed with a call to
[`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480),
which we encountered in [**Stage 5**](stage5.md).

This will also mean using the VPU to do the bulk of the work, rather than the
scalar unit.

## Introduction

## Background

## Code Changes

### Headroom Calculation in `filter_thread()`

The first change from **Stage 6** to **Stage 7** we will look at is how the 
headroom of the input audio frame is calculated.

In `stage6.c`, we calculated the input headroom in a `for` loop in
`filter_thread()` with a little help from the
[`HR_S32()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/util.h#L145-L154)
macro:

```c
  // Compute headroom of input frame
  sample_history_hr = 32;
  for(int k = 0; k < HISTORY_SIZE; k++)
    sample_history_hr = MIN(sample_history_hr, HR_S32(sample_history[k]));
```

In `stage7.c`, we're instead calling
[`vect_s32_headroom()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L554-L591) from `lib_xcore_math`:

```c
  // Compute headroom of input frame
  sample_history_hr = vect_s32_headroom(&sample_history[0], HISTORY_SIZE);
```

Though not included in the timing performance measured by the apps, this new
implementation is a much, much faster way to compute the vector's headroom.

### `filter_sample()`

The other key change from **Stage 6** to **Stage 7** is, of course, the
implementation of `filter_sample()`.

From `stage6.c`:
```c
const right_shift_t p_shr = 10;

...

q1_31 filter_sample(
    const int32_t sample_history[TAP_COUNT],
    const exponent_t history_exp,
    const headroom_t history_hr)
{
  float_s64_t acc;
  acc.mant = 0;

  acc.exp = (history_exp + coef_exp) + p_shr;

  for(int k = 0; k < TAP_COUNT; k++){
    int32_t b = sample_history[k];
    int32_t c = filter_coef[k];
    int64_t p =  (((int64_t)b) * c);
    acc.mant += ashr64(p, p_shr);
  }

  q1_31 sample_out = float_s64_to_fixed(acc, output_exp);

  return sample_out;
}
```

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

In **Stage 7** we make use of 2 functions from `lib_xcore_math`. One we saw in
**Stage 5**, `vect_s32_dot()`. New here is [`vect_s32_dot_prepare()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32_prepare.h#L182-L252).

To facilitate block floating-point logic, `lib_xcore_math`'s vector API provides
a number of "prepare" functions. A "prepare" function is a simple function
(written in C) which selects the output exponent and any shift parameters
required for a subsequent call to one of the optimized vector operations. By
convention, these "prepare" functions are simply the name of the function being
prepared for, with `_prepare` appended to the end, as with `vect_s32_dot()` and
`vect_s32_dot_prepare()`.

The exponent associated with the output of `vect_s32_dot()` depends upon 4
things. Those are the exponents associated with each of the two input vectors
`b[]` and `c[]`, and the `b_shr` and `c_shr` shift parameters.

`vect_s32_dot_prepare()` has the following prototype:
```c
void vect_s32_dot_prepare(
    exponent_t* a_exp,
    right_shift_t* b_shr,
    right_shift_t* c_shr,
    const exponent_t b_exp,
    const exponent_t c_exp,
    const headroom_t b_hr,
    const headroom_t c_hr,
    const unsigned length);
```

> Note: A convention used widely in `lib_xcore_math`'s APIs is that the output
> (vector or scalar) from an arithmetic operation is usually named `a`, and then
> the input operands (vectors or scalars) are named `b`, `c`, etc. Understanding
> this will greatly simplify interpretation of API functions.
>
> With this convention, the 'inputs' and 'outputs' are those of the _arithmetic
> operation_ rather than function parameters or return values, though they may
> often coincide.

Here `a_exp`, `b_shr` and `c_shr` are all outputs selected by the function.
`b_exp` and `b_hr` are the exponent and headroom associated with input vector
`b[]` -- in our case, the sample history. Likewise, `c_exp` and `c_hr` are the
exponent and headroom associated with input vector `c[]` -- in our case, the
filter coefficients. `length` is the number of elements in `b[]` and `c[]`.

`vect_s32_dot_prepare()` will use those values to determine the minimum exponent
`a_exp` for which the result of `vect_s32_dot()` is guaranteed not to saturate.
The minimum exponent will also minimize the headroom of the result, allowing for
maximal precision.

> Note: While the return type of `vect_s32_dot()` is `int64_t`, the accumulators
> on the VPU in 32-bit mode are only 40 bits wide. `vect_s32_dot_prepare()`
> takes that into account when selecting an output exponent.

## Results

## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)
* [`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480)
* [`HR_S32()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/util.h#L145-L154)
* [`vect_s32_headroom()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L554-L591)
* [`vect_s32_dot_prepare()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32_prepare.h#L182-L252)