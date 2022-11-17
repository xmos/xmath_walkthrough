
[Prev](app_structure.md) | [Home](intro.md) | [Next](stage1.md)

# Stage 0

**Stage 0** is our first implementation of the digital FIR filter. Much of the
code found in subsequent stages will resemble the code found here, so we will go
through the code in some detail.

### The Filter

The filter to be implemented is a simple 1024-tap FIR box filter in which the
filter output is the simple average of the most recent 1024 input samples. The
behavior of this filter is not particularly interesting. But here we are
concerned with the performance details of each implementation, not the filter
behavior, so a simple filter will do.

The output, $y[k]$, of a digital FIR filter is defined by

$$
  y[k] := \sum_{n=0}^{N-1} x[k-n] \cdot b[n]
$$

where $y[k]$ is the output at discrete time step $k$, $N$ is the number of filter taps, $x[k]$ is the input sample at time step $k$, and $b[n]$ (with $n \in \{0,1,2,...,N-1\}$) is the vector of filter coefficients.

The filter coefficient $b[n]$ has lag $n$. That is, the filter coefficient $b[0]$ is always multiplied by the newest input sample at time step $k$, $b[1]$ is multiplied by the previous input sample, and so on.

The filter used throughout this tutorial has $N$ fixed at $1024$ taps. In our case, because the filter is a simple box averaging filter, all $b[n]$ will be the same value, namely $\frac{1}{N}$. So

$$
  b[] = \left[\frac{1}{N}, \frac{1}{N}, \frac{1}{N}, \frac{1}{N}, ..., \frac{1}{N}\right]
$$

## Introduction

## Background

## Code Overview

### Filter Coefficients

In **Stage 0**, the filter coefficient vector is found in
[`filter_coef_double.c`](../apps/common/filters/filter_coef_double.c). That coefficient vector is represented by a 1024-element array of `double`s, where every element is set to the value $\frac{1}{1024} = 0.0009765625$.

```c
const double filter_coef[TAP_COUNT] = { ... };
```

## Results