
[Prev](stage0.md) | [Home](../intro.md) | [Next](stage2.md)

# Stage 1

Like [**Stage 0**](stage0.md), **Stage 1** implements the FIR filter using
floating-point logic written in a plain C loop. Instead of using
double-precision floating-point values (`double`) like **Stage 0**, **Stage 1**
uses single-precision floating-point values (`float`).

In this stage we'll see that we get a large performance boost just by using
single-precision floating-point arithmetic (for which xcore.ai has hardware
support) instead of double-precision arithmetic.

## Introduction

## Background

* Single-precision vs double-precision
* xcore.ai support for floating-point

## Code Changes

Comparing the code in `stage1.c` to the code from `stage0.c`, we see that the
only real difference is that all `double` values have been replaced with `float`
values.

Similarly, the **Stage 1** application includes `filter_coef_float.c` (rather
than `filter_coef_double.c`) which defines `filter_coef[]` as a vector of
`float` values.

## Results