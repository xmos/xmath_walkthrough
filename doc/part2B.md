
[Prev](stage3.md) | [Home](../intro.md) | [Next](stage5.md)

# Stage 4

Like [**Stage 3**](stage3.md), **Stage 4** implements the FIR filter
using fixed-point arithmetic. 

In **Stage 3** we implemented the fixed-point arithmetic directly in plain C. As
such, we were left to hope that the compiler generates an efficient sequence of
instructions to do the work. However, there are reasons to doubt the compiler
generates an optimal implementation. 

One reason is that the C compiler will not target the VPU when compiling code. Another is that the compiler will not ordinarily generate dual-issue implementations of functions written in C.

In **Stage 4**, instead of implementing the logic in a plain C loop, we use a
function written in dual-issue assembly to compute the inner product for us.
That function, [`int32_dot()`](TODO), does not use the VPU (we'll use the VPU in
the next stage), but is meant to demonstrate the improved performance we get
just from using a dual-issue function written directly in assembly.

## Introduction

## Background

## Implementation

### **Stage 4** `filter_sample()` Implementation

From [`stage4.c`](TODO):
```C
//Apply the filter to produce a single output sample
q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  // The exponent associatd with the filter coefficients
  const exponent_t coef_exp = -28;
  // The exponent associated with the input signal
  const exponent_t input_exp = -31;
  // The exponent associated with the output signal
  const exponent_t output_exp = input_exp;
  // The exponent associated with the accumulator
  const exponent_t acc_exp = input_exp + coef_exp;
  // Arithmetic right-shift applied to the filter's accumulator to achieve the
  // correct output exponent
  const right_shift_t acc_shr = output_exp - acc_exp;

  // Compute the 64-bit inner product between sample history and filter
  // coefficients
  int64_t acc = int32_dot(&sample_history[0], 
                          &filter_coef[0], 
                          TAP_COUNT);

  // Apply a right-shift, dropping the bit-depth back down to 32 bits
  return ashr64(acc, 
                acc_shr);
}
```

Notice that the only difference compared to **Stage 3** is that instead of a loop it is calling `int32_dot()`.

From [`stage4.c`](TODO):
```C
int64_t int32_dot(
    const int32_t x[],
    const int32_t y[],
    const unsigned length);
```

`int32_dot()` takes two `int32_t` arrays of length `length` and computes a
direct inner product as an `int64_t` output. That is

$$
  \mathtt{int32\_dot} \to \sum_{k=0}^{\mathtt{length}-1} {\mathtt{x}[k] \cdot \mathtt{y}[k]}
$$

If you are interested, take a look at [`int32_dot.S`](TODO) for the
implementation.

One thing to point out is the [inner loop](TODO) (from `.L_loop_top:` to
`.L_loop_bot:`) of `int32_dot.S` is only 4 instructions long. A disassembly of
the **Stage 3** firmware (`xobjdump -D bin/stage3.xe`) shows that in this case
the inner loop of the (single-issue) `filter_frame()` in **Stage 3** is 7
instructions long.

## Results

The ratio of **Stage 4**'s execution time to **Stage 3**'s execution time is
almost exactly `(4/7)`.

### Timing

| Timing Type       | Measured Timing
|-------------------|-----------------------
| Per Filter Tap    | 33.62 ns
| Per Output Sample | 34425.00 ns
| Per Frame         | 8881434.00 ns

### Output Waveform

![**Part 2B** Output](img/part2B.png)