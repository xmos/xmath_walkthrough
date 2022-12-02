
[Prev](partB.md) | [Home](../intro.md) | [Next](stage4.md)

# Stage 3

In **Stage 3** we dropped floating-point arithmetic in favor of fixed-point
arithmetic. **Stage 3**, like [**Stage 1**](stage1.md), implements the inner
product using plain C. Like **Stage 1** (compared to [**Stage 2**](stage2.md)),
this will not be a highly optimized implementation. Optimization will come in
the next stage.

Ultimately we will find that fixed-point written in plain C is slower than the
optimized floating-point from [**Stage 2**](stage2.md).

## Introduction

## Background

* Fixed-point arithmetic.

## Implementation

### **Stage 3** `filter_sample()` Implementation

From [`stage3.c`](TODO):
```c
//Apply the filter to produce a single output sample
q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  //The exponent associated with the filter coefficients.
  const exponent_t coef_exp = -28;
  // The exponent associated with the input samples
  const exponent_t input_exp = -31;
  // The exponent associated with the output samples
  const exponent_t output_exp = input_exp;
  // The exponent associated with the accumulator.
  const exponent_t acc_exp = input_exp + coef_exp;
  // The arithmetic right-shift applied to the filter's accumulator to achieve
  // the correct output exponent.
  const right_shift_t acc_shr = output_exp - acc_exp;

  // Accumulator
  int64_t acc = 0;

  // For each filter tap, add the 64-bit product to the accumulator
  for(int k = 0; k < TAP_COUNT; k++){
    const int64_t smp = sample_history[k];
    const int64_t coef = filter_coef[k];
    acc += (smp * coef);
  }

  // Apply a right-shift, dropping the bit-depth back down to 32 bits.
  return sat32(ashr64(acc, 
                     acc_shr));
}
```

Here `filter_sample()` takes in the `sample_history[]` vector, just as in the
previous stages. The inner product is computed by multiplying the 32-bit input
samples by the corresponding 32-bit filter coefficients to get a 64-bit product.
The 64-bit products are summed into a 64-bit accumulator.

Finally, [`ashr64()`](TODO) and [`sat32()`](TODO) are used. `ashr64()` applies an arithmetic right-shift of `acc_shr` bits to the 64-bit accumulator and returns the 64-bit result. 

> From [`misc_func.h`](TODO):
> ```c
> // Signed, arithmetic right-shift of a 64-bit integer
> static inline
> int64_t ashr64(int64_t x, right_shift_t shr)
> {
>   int64_t y;
>   if(shr >= 0) y = (x >> ( shr) );
>   else         y = (x << (-shr) );
>   return y;
> }
> ```
> `ashr64()` applies an arithmetic right-shift of `shr` bits to 64-bit integer
> `x`. It does not apply any saturation logic.

`sat32()` takes a 64-bit value and clamps it to the 32-bit range. This way
values that can't be represented by an `int32_t` are saturated to their nearest
representable value.

> From [`misc_func.h`](TODO):
> ```c
> // Saturate 64-bit integer to 32-bit bounds
> static inline
> int32_t sat32(int64_t x)
> {
>   if(x <= INT32_MIN) return INT32_MIN;
>   if(x >= INT32_MAX) return INT32_MAX;
>   return x;
> }
> ```

Let's dig into the logic used in **Stage 3**'s `filter_sample()`.

### Stage 3 fixed-point logic

In this stage our task was to convert 

$$
  y_k = \sum_{n=0}^{N-1}{ x_{k-n} \cdot b_n }
$$

into fixed-point logic. That is, $\bar{y}$, $\bar{x}$ and $\bar{b}$, each of
which represents a vector of real numbers, need to be replaced with values
representable as integer variables and constants.

Rewriting the equation by unpacking our logical values into objects which can be
used in code, we get

$$
  \mathtt{y}[k] (\cdot 2^{\hat{y}}) = \sum_{n=0}^{N-1} ({\mathtt{x}[k-n] 
                \cdot 2^{\hat{x}}) \cdot (\mathtt{b}[n]\cdot 2^{\hat{b}}})
$$

In this case we already know 
* $N = $ `TAP_COUNT` $ = 1024$
* $\hat{y} = \hat{x} = -31$
* $\hat{b} = -28$

But we will work out the logic of the more general case where these values
aren't known.

> **Note**: We're working out the logic of cases where the input and output
> exponent are _unknown_, but they are still _fixed_. When working with block
> floating-point we'll encounter cases where the output exponent is not fixed.
> When not fixed, the output exponent is an extra degree of freedom we have in
> choosing how to implement the operation.

First, some simplification:

$$
\begin{aligned} 
\mathtt{y}[k] \cdot 2^{\hat{y}} 
  &= \sum_{n=0}^{N-1} {\mathtt{x}[k-n] \cdot 2^{\hat{x}} \cdot \mathtt{b}[n]\cdot 2^{\hat{b}}}   \\
  &= \sum_{n=0}^{N-1} {\mathtt{x}[k-n] \cdot \mathtt{b}[n]\cdot 2^{\hat{x}+\hat{b}}}   \\
  &= 2^{\hat{x}+\hat{b}} \sum_{n=0}^{N-1} {\mathtt{x}[k-n] \cdot \mathtt{b}[n]}   \\
  &= 2^{\hat{x}+\hat{b}} \cdot \mathtt{P}   \\
\end{aligned}
$$

where

$$
  \mathtt{P} = \sum_{n=0}^{N-1} {\mathtt{x}[k-n] \cdot \mathtt{b}[n]} 
$$

Notice that $\mathtt{P}$ is precisely what is computed in the loop of
`filter_sample()` -- $\mathtt{P}$ is our accumulator. This tells us that
$\left(\hat{x}+\hat{b}\right) = \hat{P}$ is the exponent associated with the
accumulator. When multiplying, exponents add.

Next, because $\hat{y}$ is fixed, we're just solving for $\mathtt{y}[k]$

$$
\begin{aligned}
\mathtt{y}[k] \cdot 2^{\hat{y}} &= 2^{\hat{x}+\hat{b}} \cdot \mathtt{P}   \\
\mathtt{y}[k] \cdot 2^{\hat{y}} \cdot 2^{-\hat{y}} &= 2^{-\hat{y}} \cdot 2^{\hat{P}} \cdot \mathtt{P}   \\
\mathtt{y}[k]  &= 2^{\hat{P}-\hat{y}} \cdot \mathtt{P}   \\
\mathtt{y}[k]  &= 2^{\hat{P}-\hat{y}} \cdot \mathtt{P}   \\
\mathtt{y}[k]  &= \mathtt{P} \cdot 2^{-(\hat{y}-\hat{P})}  \\
\mathtt{y}[k]  &= \mathtt{P} \,\gg\,(\hat{y}-\hat{P}) \\
\mathtt{y}[k]  &= \mathtt{P} \,\gg\,\vec{r} \\
\end{aligned}
$$

where $\vec{r}=(\hat{y}-\hat{P})$ and we're using the $\mathtt{\gg}$ to mean
$\mathtt{P}$ gets a signed, arithmetic right-shift of $(\hat{y}-\hat{P})$ bits.

> **Notation**: Similar to how $\,\,\hat{}\,\,$ is used in this tutorial to hint
> to the reader that the variable (e.g. $\hat{y}$ ) is an exponent, the
> $\,\,\vec{}\,\,$ symbol will be used as a hint to the reader that the variable
> (e.g. $\vec{r}$ ) represents a right-shift.
>
> Vectors or arrays will always be represented using either the $\,\,\bar{}\,\,$ symbol, or using square brackets. e.g. $\bar{x}$ or $\mathtt{x}[\,]$.

Now we can find $\vec{r}$.

$$
\begin{aligned}
  \vec{r} &= \hat{y} - \hat{P} \\
          &= \hat{y} - (\hat{x}+\hat{b})  \\
          &= -31 - (-31 + -28)  \\
          &= 28
\end{aligned}
$$

Therefore a right-shift of `28` bits must be applied to `acc` in `filter_sample()`, which is what `acc_shr` is.

> **Note**: It doesn't affect our filter, but in general when the output
> bit-depth and exponent are fixed we must be aware that overflows or saturation
> might be possible.  This is independent of whether the input exponent is fixed
> as well.  The reason for this is implied directly by the math and by the
> definition of the output.
>
> $$
> \begin{aligned}
>   y_k &= \mathtt{y}[k]\cdot 2^{\hat{y}} \\
>   \mathtt{y}[k] &= y_k \cdot 2^{-\hat{y}} \\
> \end{aligned}
> $$
>
> The range of $\mathtt{y}[k]$ is fixed by its bit-depth. For 32-bit values, the
> range is $[-2^{31},2^{31}-1)$. Then, the range for $y_k$ which avoids any
> overflow or saturation is
>
> $$
>   -2^{31} \le \mathtt{y}[k] \lt 2^{31} \\
>   -2^{31} \le y_k \cdot 2^{-\hat{y}} \lt 2^{31} \\
>   -2^{31} \cdot 2^{\hat{y}} \le y_k \cdot 2^{-\hat{y}} \cdot 2^{\hat{y}} \lt 2^{31} \cdot 2^{\hat{y}} \\
>   -2^{31+\hat{y}} \le y_k \lt 2^{31+\hat{y}} \\
> $$
>
> Thus, if the underlying math dictates that $y_k$ is outside that range (for
> some given set of inputs), there is no way to avoid overflows.
>
> This isn't a problem for our FIR filter because it is computing a simple
> average, and the average of a list of numbers can never be beyond the range of
> those numbers.


## Results