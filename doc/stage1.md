
[Prev](stage0.md) | [Home](../intro.md) | [Next](stage2.md)

# Stage 1

Like [Stage 0](stage0.md), Stage 1 implements the FIR filter using
floating-point logic written in a plain C loop. Instead of using double-precision floating-point values (`double`) like Stage 0, Stage 1 uses single-precision
floating-point values (`float`).

## Introduction

## Background

## Code Changes

Comparing the code in `stage1.c` to the code from `stage0.c`, we see that the only difference is that all `double` values have been replaced with `float` values.

## Results