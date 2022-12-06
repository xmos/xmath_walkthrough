
# Part 2: Fixed-Point Arithmetic

**Part 2** migrates our digital FIR filter to use fixed-point arithmetic instead
of floating-point arithmetic.

```{toctree}
---
maxdepth: 1
---
./part2A.md
./part2B.md
./part2C.md
```

With fixed-point arithmetic we must be mindful about the possible range of
(logical) values that the data may take. Knowing the range of possible values
allows us to select exponents for our input, output and intermediate
representations of the data which avoid problems like overflows, saturation or
excessive precision loss.

## Stages

**Part 2A** migrates the filter implementation to use fixed-point arithmetic.
Like **Parts** **1A** and **1B**, the implementation will be written in plain C
so the reader sees everything that is going on.

**Part 2B** replaces the plain C implementation of `filter_sample()` from **Part
2A** with a custom assembly routine which uses the scalar arithmetic unit much
more efficiently.

**Part 2C** replaces the custom assembly routine with a call to a library
function from `lib_xcore_math` which uses the VPU to greatly accelerate the
arithmetic.

## Fixed-Point Background

* Fixed-point types

Note that `buff[]` is an array of `q1_31`. [`q1_31`](TODO) is a 32-bit integral
type defined in `lib_xcore_math`. When dealing with 32-bit fixed-point
arithmetic, we can always just use `int32_t` values, but that tells us nothing
about the _logical_ values represented by the data. Because the exponent
associated with fixed-point values is _fixed_, we can encode that information
directly into the type of the variables and constants in which they're stored. 

To that end, `lib_xcore_math` defines `q1_31` and many related type (e.g.
`q2_30`, `q8_24`, etc) for representing the
[_Q-format_](https://en.wikipedia.org/wiki/Q_(number_format)) of the underlying
data. They may then be used to _hint_ to a user that a given value should
understood to be associated with a particular exponent. Equivalently, they may
be understood to contain a particular number of fractional bits.

The type `q1_31` corresponds to the Q-format `Q1.31`. That is a 32-bit value
(`1+31`) with `1` integer bit and `31` fractional bits. Likewise, `q8_24`
indicates the `Q8.24` format with `24` fractional bits, and so on.



* Mantissas and exponents
* Fixed-point Arithmetic
* Fixed-point support in `lib_xcore_math`

## Component Functions

Like in **Part 1**, the behavior of each stage in **Part 2** is defined by 4 component function:

* `rx_frame()`
* `tx_frame()`
* `filter_sample()`
* `filter_task()`

These functions serve essentially the same role in **Part 2** that they did in
**Part 1**.

Three of those functions, `filter_task()`, `rx_frame()` and `tx_frame()` use the exact same implementation for all three stages. The only thing that changes between these stages is the `filter_sample()` implementation.

In this page we will take a look at those functions common to each stage, and
then we will dive into the logic of choosing exponents to represent our
fixed-point values. It will finish up with some in-depth examples.


## **Part 2** Filter Coefficients

Whereas in **Part 1** the filter coefficients were implemented as a constant array of `double` (**Part 1A**) or `float` (**Stages 1** and **2**) values, in **Part 2** the filter coefficients are represented by an array of `q4_28` values (i.e. the `Q4.28` format). With `28` fractional bits, these coefficients have an implied exponent of `-28`.

The filter coefficients in this part come from [`filter_coef_q4_28.c`](TODO).

Being an averaging FIR filter, all coefficients have the same logical value of
$\frac{1}{1024} = 0.0009765625$. Let's convert this to `Q4.28`.

$$
\begin{aligned}
  x &= \frac{1}{1024} \\
  \hat{y} &= -28 \\
  \\
  \mathtt{y}\cdot 2^{\hat{y}} &= x \\
  \mathtt{y}\cdot 2^{-28} &= 2^{-10} \\
  \mathtt{y} &= 2^{-10}\cdot 2^{28} \\
  \mathtt{y} &= 2^{18} = 262144 = \mathtt{0x40000}
\end{aligned}
$$

So in `Q4.28`, each of our filter coefficients should be represented by the integer value `0x40000`.

Using the `Q4.28` format was our choice. Normally when choosing a representation
for a fixed-point data it's a good idea to use the lowest exponent which avoids
losing data. That corresponds to having zero headroom -- a concept that we will
come back to in **Part 3**. Using a minimal exponent generally means maximal
precision. In this particular case, because our coefficients are a power of 2,
any admissible exponent will be equivalent.

## Scientific Notation for Fixed- and Floating-Point Values

We need to take our conceptual, mathematical objects (like filter coefficients)
and represent them in code. In **Part 1** this was simple, because the scalar
floating-point types are abstractions which encapsulate all the pieces we care
about. And so there was no need to reason about the mantissas and exponents of
individual values or vectors.

Beginning in this part, we will find that we need to take those conceptual,
mathematical objects (like real numbers) and reason about them. We need to be able to represent and manipulate them both _abstractly_ (as in the equations you will find in subsequent pages) and in code.

To that end we introduce a new notation which will be used throughout the
remainder of this tutorial.

Suppose $x$ is a real value -- a abstract, _mathematical_ object -- that needs
to be represented explicitly using integers (because _conceptual_ objects can't
appear in _actual_ code). To do this it needs a mantissa and exponent. This is essentially the same thing as [scientific notation](https://en.wikipedia.org/wiki/Scientific_notation), but using powers of $2$ instead of $10$.

Rather than choose arbitrary new letters to represent each of these things,
which would quickly become a confusing alphabet soup, henceforth we will adopt a
convention where $\mathtt{x}$ (fixed-width font) represents the mantissa and
$\hat{x}$ represents its exponent. $x$ will continue to indicate what we refer to as the _logical_ value -- that which is meant to be represented.
 
$$
  x \mapsto \mathtt{x}\cdot 2^{\hat{x}}
$$

This notation is meant to reduce confusion about the relationships between the
symbols in our equations.

Note that these are not strictly _equal_. If $x$ is $\frac13$ then there will
necessarily be a rounding error as we represent $x$ with $\mathtt{x}$ and
$\hat{x}$. However, this detail will generally be ignored in presenting our
equations.

Additionally, to further avoid confusion, we will adopt a convention where
vectors of _conceptual_ objects will use _subscripts_ to denote the element at a
given index, such as $x_k$, whereas for vectors of _mantissas_ we will use
_brackets_ to denote the element at a given index, such as $\mathtt{x}[k]$. This
is also meant to suggest a move from abstract math towards concrete code.  

### An Example (Scalar)

To make this concrete, consider the logical value $p = 6.03125$. Unpacking $p$, we have 

$$
\begin{aligned}
  p &= 6.03125 \\
  \mathtt{p} \cdot 2^{\hat{p}} &= 6.03125 \\
\end{aligned}
$$

Assuming $\mathtt{p}$ must be stored in an `int32_t`, what are $\mathtt{p}$ and
$\hat{p}$? Well, we have options.

First we should always consider the _lower_ bound for $\hat{p}$. The question is, _what is the least value of $\hat{p}$ that allows us to represent $p$_? To figure that out we need to know in what cases we _can't_ represent $p$. 

$$
\begin{aligned}
  p &= \mathtt{p} \cdot 2^{\hat{p}} \\
  p \cdot 2^{-\hat{p}} &= \mathtt{p} \\
\end{aligned}
$$

The `int32_t` representation gives us bounds on $\mathtt{p}$

$$
\begin{aligned}
  \mathtt{INT32\_MIN} \le &\mathtt{p} \le \mathtt{INT32\_MAX} \\
  -2^{31} \le &\mathtt{p} \le 2^{31} - 1 \\
  -2^{31} \le &\mathtt{p} \lt 2^{31} \\
\end{aligned}
$$

So any $\hat{p}$ which would force $\mathtt{p}$ to be greater than or equal to $2^{31}$ is an inadmissible value for $\hat{p}$. 

$$
\begin{aligned}
  \mathtt{p} \ge 2^{31} \\
  p \cdot 2^{-\hat{p}} \ge 2^{31} \\
  \frac{p \cdot 2^{-\hat{p}}}{p} \ge \frac{2^{31}}{p} \\
  2^{-\hat{p}} \ge \frac{2^{31}}{p} \\
  log_2(2^{-\hat{p}}) \ge log_2\left(\frac{2^{31}}{p}\right) \\
  -\hat{p} \ge log_2(2^{31}) - log_2(p) \\
  -\hat{p} \ge 31 - log_2(6.03125) \\
  -\hat{p} \ge 31 - 2.59246 \\
  -\hat{p} \ge 28.40754 \\
  \hat{p} \le -28.40754 \\
\end{aligned}
$$

This tells us that $\hat{p}$ values _less than or equal to_ about $-28.4075$
will force an overflow in $\mathtt{p}$, our `int32_t` value. Note that $\hat{p}$
must be an _integer_, so the condition practically becomes $\hat{p} \lt -28$.

Let's check this. First, let's verify that $\hat{p}=-28$ works.

$$
\begin{aligned}
  \hat{p} &= -28 \\
  \\
  \mathtt{p} &= p \cdot 2^{-\hat{p}} \\
  \mathtt{p} &= 6.03125 \cdot 2^{28} \\
  \mathtt{p} &= 1619001344.0 = \mathtt{0x60800000} \\
\end{aligned}
$$

`0x60800000` is less than `INT32_MAX`, so this works. What about $\hat{p} =
-29$?

$$
\begin{aligned}
  \hat{p} &= -29 \\
  \\
  \mathtt{p} &= p \cdot 2^{-\hat{p}} \\
  \mathtt{p} &= 6.03125 \cdot 2^{29} \\
  \mathtt{p} &= 1619001344.0 = \mathtt{0xC1000000} \\
\end{aligned}
$$

`0xC1000000` is _greater than_ `INT32_MAX`, so we've confirmed $-29$ is not an admissible exponent.

Then, the _minimum_ value for $\hat{p}$ is $-28$. 

Is there a maximum value for $\hat{p}$?  Yes and no -- mostly no.

When decreasing $\hat{p}$, we observed a sudden cutoff between $-28$ and $-29$ where we were immediately failing to represent $p$. The effect going in the other direction is more subtle. 

Attempting to represent a real number using finite information generally
involves rounding errors. In particular, an object $x$ represented by a mantissa
and exponent as $(\mathtt{x}\cdot 2^{\hat{x}})$ will have a resolution of $2^{\hat{x}}$. To see why, consider the effect of adding or subtracting $1$ to the integer $\mathtt{x}$ (note that this is independent of the bit-depth of $\mathtt{x}$) 

$$
\begin{aligned}
  & (\mathtt{x}\pm 1) \cdot 2^{\hat{x}} \\
  &= (\mathtt{x}\cdot 2^{\hat{x}}) \pm (1 \cdot 2^{\hat{x}}) \\
  &= x \pm 2^{\hat{x}}
\end{aligned}
$$

This also implies the smallest-magnitude non-zero value represntable using the exponent $\hat{x}$ is $\pm 2^{\hat{x}}$.  Given these things, it should be clear that the larger the exponent ($\hat{x}$) the lower the resolution and thus the greater the (possible) quantization error.

Going back to our example with $p = 6.03125$, recall that with $\hat{p} = -28$, the value of $\mathtt{p}$ was `0x60800000`. Notice that the 23 least significant bits of that value are all zeros. For every increment of $\hat{p}$, the value of $\mathtt{p}$ must shift to the right 1 bit. But shifting out zeros actually doesn't impact our represented value.

So if we chose $\hat{p} = -5$ 

$$
\begin{aligned}
  \hat{p} &= -5 \\
  \\
  \mathtt{p} &= p \cdot 2^{-\hat{p}} \\
  \mathtt{p} &= 6.03125 \cdot 2^{5} \\
  \mathtt{p} &= 193.0 = \mathtt{0x000000C1} \\
\end{aligned}
$$

And then reversing this to work out the value of $p$ we get $193 \cdot 2^{-5} =
6.03125$ -- we have represented $p$ exactly.

But if we chose $\hat{p} = -4$

$$
\begin{aligned}
  \hat{p} &= -4 \\
  \\
  \mathtt{p} &= p \cdot 2^{-\hat{p}} \\
  \mathtt{p} &= 6.03125 \cdot 2^{4} \\
  \mathtt{p} &= 96.5 \\
\end{aligned}
$$

Here we're forced to round $96.5$ either up or down, because $\mathtt{p}$ is an
integer. Let's see how that works out.. 

$$
\begin{aligned}
  \hat{p} &= -4 \\
  \\
  96 \cdot 2^{-4} &= 6.0 \\
  97 \cdot 2^{-4} &= 6.0625 \\
  \\
  (97 \cdot 2^{-4}) - (96 \cdot 2^{-4}) &= 0.0625 \\
  &= 2^{\hat{p}}
\end{aligned}
$$

Here we see that using $\hat{p}=-4$ begins to introduce quantization error, where again we find that the resolution is given directly by the exponent.

In summary, for our example of $p = 6.03125$, we can represent $p$ exactly so long as 

$$
  -28 \le \hat{p} \le -5
$$

Going below that range immediately corrupts our value, and going above that range gradually corrupts it through quantization error.

### An Example (Vector)

What if we have $\bar{p}$ instead of $p$ -- that is, if we need to choose an
exponent $\hat{p}$ for representing a vector instead of a scalar?  In
floating-point, each element $p_k$ of $\bar{p}$ has its own exponent,
independent of other elements in $\bar{p}$. 

With fixed-point or block floating-point (BFP), we choose a single exponent for
the entire vector.  The difference between the two is that in fixed-point the
exponent is fixed _at compile time_ and does not depend on the actual data (and
often implied rather than explicit), while in BFP the exponent is chosen
dynamically (usually) at runtime based on the data, and _must_ be represented
explicitly.

Consider the vector $\bar{p} = [0.005432, 1.2245, -0.5, -3.2]$. How do we go about choosing an exponent $\hat{p}$ for this vector?

It turns out that even for fixed-point and BFP, the logic of choosing an exponent for a vector ultimately reduces to that of the scalar case:

* We still need to avoid overflows in our mantissas
* We still want as little quantization error as possible

It turns out that the largest magnitude element of the vector will end up
dictating the exponent. Each element will imply its own lower-bound on
$\hat{p}$, but the element with the greatest magnitude will have the _highest_
lower-bound. Note that the largest _magnitude_ element may be either positive
or negative.

In our case, we will call the largest magnitude element $p'$: 

$$
\begin{aligned}
  p' &= p_{k} \\
  k &= \argmax_n |p_n| \\
  k &= 3 \\
  p' &= p_3 \\
  p' &= -3.2 \\
\end{aligned}
$$

This time $p'$ is negative. This still follows the same logic as before, where

$$
  \mathtt{INT32\_MIN} \le \mathtt{p}' \le \mathtt{INT32\_MAX} 
$$

Only, this time we're concerned about $\mathtt{p}'$ going too _negative_, so we look for the bound in the other direction.

$$
\begin{aligned}
  \mathtt{p}' \lt \mathtt{INT32\_MIN} \\
  \mathtt{p}' \lt -2^{31} \\
  p' \cdot 2^{-\hat{p}} \lt -2^{31} \\
  \frac{p' \cdot 2^{-\hat{p}}}{p'} \gt \frac{-2^{31}}{p'} && \text{(division by negative)}\\
  2^{-\hat{p}} \gt 671088640.0 \\
  log_2(2^{-\hat{p}}) \gt log_2\left(671088640.0\right) \\
  -\hat{p} \gt 29.32192 \\
  \hat{p} \lt -29.32192 \\
\end{aligned}
$$

Now we find that we'll have overflows in $\mathtt{p}[\,]$ if $\hat{p}$ is less than $-29$. Then 

$$
\begin{aligned}
  \hat{p} &= -29 \\
  \bar{p} &= [0.005432,1.2245,−0.5,−3.2] \\
  \\
  \mathtt{p}[k] &= p_k \cdot 2^{-\hat{p}} \\
                &= p_k \cdot 2^{29} \\
  \mathtt{p}[\,] &= [2916283, 657398432, -268435456, -1717986918] \\
  \mathtt{p}[\,] &= [\mathtt{0x002C7FBB}, \mathtt{0x272F1AA0}, 
                     \mathtt{-0x10000000}, \mathtt{-0x66666666}] \\
\end{aligned}
$$

We can see here that attempting to left-shift these mantissa values one bit would cause $\mathtt{p}[3]$ to overflow into the sign bit, corrupting the value. So, $-29$ is indeed the minimum exponent.

In [**Part 3**](partC.md) we will see that there is a shortcut for computing exponents using another property we can extract from scalars and vectors called the **headroom**.
