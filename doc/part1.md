
# Part 1: Floating-Point Arithmetic

**Part 1** deals with floating-point arithmetic.

```{toctree}
---
maxdepth: 1
---
./part1A.md
./part1B.md
./part1C.md
```

It is useful to start with a floating-point implementation, and particularly one
written in plain C, because many users approach xcore.ai with their algorithms
implented in floating-point and plain C for the sake of portability.

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

* TODO - IEEE 754 Floating-Point Values and Representation
* TODO - Single-Precision and Double-Precision
* xcore.ai support for floating-point

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
with a value of `0x2000000`. In **Part 1A** `ldexp()` is used to perform the conversion as

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

Whereas $-1.0$ was converted to $\mathtt{INT32\_MIN}$, the least possible `int32_t` value, $+1.0$ was converted to $(\mathtt{INT32\_MAX}+1) = -\mathtt{INT32\_MIN}$, which cannot be represented by a signed 32-bit integer.

And so, using an output exponent of $-31$, the range of floating-point values which can be converted to a 32-bit integer without overflows is $[-1.0, 1.0)$.


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

