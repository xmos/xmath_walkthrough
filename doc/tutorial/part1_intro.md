
## Stages

**Part 1A** implements the FIR using floating-point arithmetic with
double-precision floating-point values. It is implemented in plain C so that
nothing is hidden from the reader.

**Part 1B** is nearly identical to **Part 1A**, except single-precision
floating-point values are used. It is also implemented in plain C.

**Part 1C** uses single-precision floating-point values like **Part 1B**, but
will use a library function from `lib_xcore_math` to do the heavy lifting, both
simplifying the implementation and getting a hefty performance boost.

## Floating-Point Background

When non-integral values need to be represented in code, a
[floating-point](https://en.wikipedia.org/wiki/Floating-point_arithmetic)
representation is typically used. In C this is most often accomplished using the
standard `float` or `double` primitive types. The full details of this
representation (in particular the [IEEE
754](https://en.wikipedia.org/wiki/IEEE_754) formats) is beyond the scope of
this tutorial. This section will instead focus on the relevant general concepts.

A floating-point representation (whether a standard type like `float`, or a
non-standard type like
[`float_s32_t`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/types.h#L105-L126))
approximates real numeric values by encoding it using a kind of scientific
notation. Each representable value $x$ gets encoded using a pair of integers
$m$ and $p$ where

$$
  x = m \cdot 2^{p}
$$ 

Here, the (signed) integer $m$ is the mantissa, and the (signed) integer $p$ is
the exponent. Thus, the value $x$ is some integer scaled by some power of 2. 

Floating-point representations are fixed size encodings in that every
representable value is stored in memory using the same number of bits. On
xcore.ai, single-precision (`float`) values are 32-bit objects and
double-precision (`double`) values are 64-bit objects. Typically, the mantissa
and exponent are themselves given a fixed number of bits. 32-bit IEEE 754 floats
allocate 8 bits for the exponent and 24 bits for the mantissa, including a sign
bit. `double`s get 53 bits of mantissa and 11 bits of exponent.

The `float_s32_t` non-standard floating-point scalar type from `lib_xcore_math`,
on the other hand, uses a total of 64 bits, for 32 bits of mantissa and 32 bits
of exponent.

```{note}
More than 8 bits of exponent are almost never needed in practice. However, there
are architecture-specific advantages of using a whole word for the exponent.
Where non-standard float types are used in `lib_xcore_math`, it is typically 
meant as bridge between different _block_ floating-point entities, as the 
`float_s32_t` type is essentially equivalent to a 32-bit BFP vector with only a 
single element.

Where operations in the 32-bit BFP API return `float_s32_t` values, the 
corresponding functions in the 16-bit API typically return standard `float` 
values. The reason for this is that the `float` type has 24 bits of mantissa. 
Returning this from the 32-bit API gives an immediate loss of precision of up to 
8 bits, while no such loss is incurred in the 16-bit API. 

Additionally, while `double` values are much easier to use than `float_s32_t`,
and wouldn't result in loss of precision, xcore.ai has no hardware support for
`double`. `double` arithmetic implemented in software is far too costly (as will
be seen in [**Part 1A**](part1A.md)) in terms of compute, and has the same
memory footprint as `float_s32_t`.

Because of their size arrays of `float_s32_t` (and related types) are wasteful
of memory, and are not recommended to be used. If you find yourself doing this, 
consider using the Block Floating-Point API or `float` vector API instead.
```

A convenient way of thinking about this (which fits together cleanly with the
_block_ floating-point concept which will be introduced in [**Part
3**](part3.md)) is to consider the range and spacing of values which are
representable in a floating-point representation with a _fixed_ exponent $p$.

For $x$ represented as a `float_s32_t` with an exponent $p = 0$, the 32-bit
mantissa $m$ may take the standard range of `int32_t` values, from `INT32_MIN`
($-2^{31}$) to `INT32_MAX` ($2^{31}-1$). And because $2^0 = 1$, $x$ may
represent any of the ordinary 32-bit integers. Notice also that the gap between
representable values (with $p=0$) is $1 = 2^p$.

The upper-bound, lower-bound and spacing of representable values are exponential
in $p$, and thus linear in $2^p$. So, every increment of $p$ by $1$ means
doubling all three of those properties. Likewise, every decrement by $1$ means
halving them.

This is a particularly important mental model for block floating-point
arithmetic because (as will be seen in **Part 3**) for BFP operations, the
output _exponent_ is usually chosen _before_ computing _any_ output mantissas
(and usually with only meta-information _about_ the input mantissas in the form
of _headroom_).

This framing also bridges the gap between four broad _flavors_ of discrete 
arithmetic, namely integer, fixed-point, floating-point, and block
floating-point. All four of these can be seen as specializations of a more
general arithmetic, where the details of manipulating the values are
representation-specific, but the overall mathematical logic is unified.

### Unified Logic

Suppose we have a _vector_ of $N$ real values $\bar{x}$ to be represented, where
the elements are $x_k$ for $k \in \{0,1,...,(N-1)\}$. We can describe an
abstract representation for particular value $x_k$ in a generalized way as:

$$
    x_k \approx  m_k \cdot 2^{p_{\lfloor \frac{kL}{N} \rfloor }} \\
    \text{for }k \in \{0,1,...,(N - 1)\}
$$

That is, there is a vector of mantissas (of some bit-depth) $\bar{m}$, as well
as a _vector_ of $L$ exponents $\bar{p}$. Here, each mantissa $m_k$ corresponds 
to only one exponent, namely $p_{\lfloor\frac{kL}{N}\rfloor}$

To simplify, we'll include an additional constraint $L \in \{1,N\}$. If $L=1$,
a single exponent is used for _all_ mantissas, and if $L=N$ then each mantissa
gets _its own_ exponent.

```{note}
That additional constraint is not _necessary_. Sometimes it is useful to use 
different exponents for different ranges of mantissas. This is particularly the 
case when the elements of $\bar{x}$ cover a large dynamic range.  

For example, audio signals often have the vast majority of their power at lower 
frequencies, and this manifests in their frequency spectra where the spectral 
magnitude of frequency components near DC is many orders of magnitude larger 
than that of frequency components near the Nyquist rate.

In such a case it can be useful to use 2 or more exponents corresponding to 
different frequency ranges. This helps preserve the arithmetic precision of the
higher frequency components.
```

If we take "floating-point" to mean "exponents _aren't_ necessarily _fixed_",
rather than meaning "exponents _are_ necessarily _dynamically_ determined"
(which we may have implicitly been assuming so far), we can say:

* All arithmetic can be considered vector arithmetic, regardless of $N$
* Scalar arithmetics are vector arithmetics with $N = L = 1$
* Integer arithmetics are those with $\bar{p} = 0$
* Fixed-point arithmetics are those with constant $\bar{p}$
* Block floating-point arithmetic has $L = 1$
* Ordinary (non-block) floating-point arithmetic corresponds to $L = N$

Each of these flavors of arithmetic has their own advantages and disadvantages.

On xcore.ai, single-precision `float` operations are accelerated by a hardware
FPU, including a single cycle fused multiply-accumulate.

### PCM-Floating-point Conversion

In each stage of **Part 1**, PCM samples receives from `tile[0]` are converted
to floating-point, and floating-point output samples are converted to PCM
samples before being sent to `tile[0]`. These to steps happen in the
`frame_rx()` and `frame_tx()` functions respectively.

The actual input and output sample values going between tiles are all raw
integer values -- just as they're found in the `wav` files. When converting to
`double`, we are not compelled to keep the same scaling. So, if we treat these
PCM values as fixed-point, the exponent associated with the values is more or
less arbitrary. We've chosen `-31` as `input_exp` and `output_exp`, which
means the range of logical sample values is `[-1.0, 1.0)`.

For the sake of allowing output waveforms to be directly compared, we continue
to use  implicit or explicit input and output exponents of `-31` throughout this
tutorial.

```{note} 
The reason the assumed exponent is arbitrary is because we're implementing a 
_linear_ digital filter. If the filter was not linear (e.g. if there was a 
square root somewhere in the logic) the output values would differ depending 
upon our choice of input exponent.
```


To see the logic of this conversion, consider a 32-bit PCM sample being received
with a value of `0x2000000`. In **Part 1A** `ldexp()` is used to perform the
conversion as

$$
\begin{aligned}
  \mathtt{samp\_f} &\gets \mathtt{ldexp(0x20000000, input\_exp)}  \\
   &= (\mathtt{0x20000000} \cdot 2^{\mathtt{input\_exp}})         \\
   &= 2^{29} \cdot 2^{\mathtt{input\_exp}}                        \\
   &= 2^{(29 + \mathtt{input\_exp})}                              \\
   &= 2^{(29 - 31)}                                               \\
   &= 2^{-2}                                                      \\
   &= 0.25
\end{aligned}
$$

And so the 32-bit PCM value `0x20000000` becomes the `double` value `0.25`.
**Parts** **1B** and **1C** use `ldexpf()` instead of `ldexp()`.

---

The logic of converting the floating-point values to 32-bit PCM values via the
output exponent in `tx_frame()` is the reverse of that in `rx_frame()`.
Consider the case where we need to convert the `double` value `0.123456` into a
32-bit fixed-point value with `31` fractional bits.

$$
\begin{aligned}
  \mathtt{output\_exp} &= -31                                     \\
  \mathtt{samp\_f} &= 0.123456                                    \\
  \\
  \mathtt{sample\_out} &\gets \mathtt{round}
               (\mathtt{ldexp}(0.123456, -\mathtt{output\_exp}))  \\
    &= \mathtt{round}(\mathtt{ldexp}(0.123456, 31))               \\
    &= \mathtt{round}(0.123456 \cdot 2^{31})                      \\
    &= \mathtt{round}(265119741.247488)                           \\
    &= 265119741                                                  \\
\end{aligned}
$$

Now consider the same with the floating-point value $-1.0$.

$$
\begin{aligned}
  \mathtt{output\_exp} &= -31                                     \\
  \mathtt{samp\_f} &= -1.0                                        \\
  \\
  \mathtt{sample\_out} &\gets \mathtt{round}
               (\mathtt{ldexp}(-1.0, -\mathtt{output\_exp}))      \\
    &= \mathtt{round}(-1.0 \cdot 2^{31})                          \\
    &= -2147483648                                                \\
    &= \mathtt{INT32\_MIN}                                        \\
\end{aligned}
$$

Finally, consider the same with the floating-point value $+1.0$.

$$
\begin{aligned}
  \mathtt{output\_exp} &= -31                                     \\
  \mathtt{samp\_f} &= 1.0                                         \\
  \\
  \mathtt{sample\_out} &\gets \mathtt{round}
               (\mathtt{ldexp}( 1.0, -\mathtt{output\_exp}))      \\
    &= \mathtt{round}( 1.0 \cdot 2^{31})                          \\
    &= 2147483648                                                 \\
    &= \mathtt{INT32\_MAX} + 1                                    \\
\end{aligned}
$$

Whereas $-1.0$ was converted to $\mathtt{INT32\_MIN}$, the least possible
`int32_t` value, $+1.0$ was converted to $(\mathtt{INT32\_MAX}+1) =
-\mathtt{INT32\_MIN}$, which cannot be represented by a signed 32-bit integer.

And so, using an output exponent of $-31$, the range of floating-point values
which can be converted to a 32-bit integer without overflows is $[-1.0, 1.0)$.


## Component Functions

In **Part 1**, each stage's behavior is defined by 4 component functions:

* `rx_frame()`
* `tx_frame()`
* `filter_sample()`
* `filter_task()`

These functions will be defined in most of the following stages as well.
Structuring the stages in this way allows the different implementations to be
more easily compared.

When looking at the code for each stage, these are the functions we'll examine.


```{note} 
The _signatures_ for these functions are _not_ the same for all stages.
```

### `filter_task()`

This is the thread entry-point for the filtering thread. This function will
generally declare needed buffers for input and output sample data, initialize
them, and then go into an infinite loop.

In each loop iteration, `filter_task()` will read in a new frame of input audio
samples (using `rx_frame()`), compute one frame's worth of output audio samples
(using `filter_sample()`) and then send out the frame of output samples back to
the `wav_io` thread on tile 0 (using `tx_frame()`).

### `filter_sample()`

Each call to `filter_sample()` will compute exactly one output sample using the
history of received samples. The implementation of this function will change for
every stage throughout this tutorial.

### `rx_frame()`

This function accepts a frame of input audio samples from the `wav_io` thread
running on tile 0 via a channel. In most cases, this function will store the
received samples into the sample history _in reverse chronological order_, to
ensure the ordering of sample data matches the order of filter coefficients.

```{note} 
We happen to be using a symmetric filter, so the ordering isn't actually
important in our case. However, in general the order does matter.
```

### `tx_frame()`

This function uses a channel to send a frame of output audio samples back to the
`wav_io` thread on tile 0.

