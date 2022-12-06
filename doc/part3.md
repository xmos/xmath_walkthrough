
# Part 3: Block Floating-Point Arithmetic

In **Part 2** our digital FIR filter was implemented using fixed-point
arithmetic. In **Part 3** we will update it to use block floating-point (BFP)
arithmetic.

```{toctree}
---
maxdepth: 1
---
./part3A.md
./part3B.md
./part3C.md
```

With fixed-point arithmetic we had to be mindful of the range of logical values
our samples may take, so as to avoid overfloating the mantissas used to
represent them. BFP arithmetic will allow us to avoid making those decisions at
compile time. Instead we will have to dynamically choose exponents as we go to
perform an operation, based on what we know about the inputs to that operation.


## Stages

In **Part 3A**, both the BFP vector objects and the BFP vector arithmetic are
implmented in plain C, to demonstrate how the operations actually work under the
hood.

In **Part 3B**, some of the C routines from **Part 3A** are replaced with
optimized library calls to `lib_xcore_math`'s vector API.

In **Part 3C**, the BFP vector objects from **Part 3A** are replaced with BFP
vector types defined in `lib_xcore_math`, and the vector API calls from **Stage
7** are replaced with operations from `lib_xcore_math`'s BFP API.

## From `lib_xcore_math`

This page makes references the following types and operations from
`lib_xcore_math`:

* [`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480)
* [`vect_s32_mul()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L828-L880)

## Component Functions

The stages in **Part 3** are structured much the same as in **Part 2**, each
using their own versions of several functions. Things are a bit more complicated
this time, with 7 different functions implemented by each stage: 

* `filter_task()`
* `filter_frame()`
* `filter_sample()` 
* `calc_headroom()`
* `tx_frame()`
* `rx_frame_bfp()`
* `rx_and_merge_frame()`

`filter_task()` and `filter_sample()`, and `tx_frame()` fill largely the same
roles as in previous stages.

`filter_frame()` sits between `filter_task()` and `filter_sample()` on the call
stack. In BFP, we are typically calling operations which compute a whole vector
of values at once. `filter_frame()` fills that role for **Part 3**. In **Part
3**, `filter_sample()` is used by `filter_frame()` to compute individual output
elements, while `filter_frame()` is used by `filter_task()` to compute an entire
output frame at once.

`calc_headroom()` is a helper function which will be used to calculate the
headroom of our BFP vectors.

`tx_frame()` behaves much as it did in **Part 2**, except the output frame it is
given may not be using the desired output exponent, so `tx_frame()` will need to
coerce sample values into the correct output exponent before sending them to
tile 0.

In **Part 3** `rx_and_merge_frame()` and `rx_frame()` together perform a role
similar to that of `rx_frame()` in previous stages. `rx_frame()` behaves much
like it did before, but does not merge the frame into the sample history.
Instead it just reports the new frame's mantissas, exponent and headroom.

`rx_and_merge_frame()` is an unusual function which takes the BFP vector
representing the sample history as a parameter. It retrieves a BFP vector
representing the new frame with a call to `rx_frame()`, but then it must merge
the new frame of data into the sample history.

This is an unusual BFP operation because before we can merge the new frame of
data into the sample history, we have to ensure that both the new and old sample
data use the same exponent.

## Block Floating-Point

A 32-bit fixed-point vector is represented by an array of `int32_t` mantissa
values, along with an implied or (constant) explicit exponent.

A 32-bit BFP vector is represented by an array of `int32_t` mantissas and an
exponent, however, the exponent must be explicit and is (usually) dynamic. In
addition, BFP vectors have another property, called their _headroom_, which is
tracked along with mantissas and exponent.

Note that there aren't really any "block floating-point" scalars. The "block"
implies a vector of mantissas associated with a single exponent. A BFP vector
containing only 1 element should be thought of as a floating-point value (though
non-standard).

## Headroom 

Headroom is a concept that applies to integer scalars and vectors. The headroom
of an integer scalar is _the number of bits the value can be left-shifted
without losing information_.

Consider the `uint32_t` value `23`. 

```c
uint32_t x = 23;
```

As an unsigned, 32-bit integer, the bit-pattern used to represent the number
`23` is 

    0000 0000 0000 0000 0000 0000 0001 0111

There are 27 `0`'s preceding the first non-zero bit.

So what happens if we do the following (assuming `>>` is _not_ an arithmetic
shift)?

```c
printf("x: %u\n",  ((x<<27)>>27)  );
```

After the 27-bit left-shift, we have:

    1011 1000 0000 0000 0000 0000 0000 0000

And then with the 27-bit right-shift, we're back to where we started:

    0000 0000 0000 0000 0000 0000 0001 0111

And so the call to `printf()` gives 

    x: 23

What about this?

```c
printf("x: %u\n",  ((x<<28)>>28)  );
```

After the 28-bit left-shift, we have:

    0111 0000 0000 0000 0000 0000 0000 0000

The `1` was lost! Now after the right-shift happens we're left with:

    0000 0000 0000 0000 0000 0000 0000 0111

And so `printf()` gives

    x: 7

By left-shifting `28` bits we have lost information, whereas the `27`-bit
left-shift did not lose information. The headroom of the _unsigned_ 32-bit
integer `x` is `27` bits -- the number of bits it can be left-shifted without
losing information.

Equivalently, the headroom of an **unsigned** integer (also sometimes called the
"unsigned headroom") can be defined as _the number of leading zeros_. Every zero
we left-shift out will be replaced with a zero if we right-shift it again.

---

What about for _signed_ integers?

Let's simultaneously consider the pair of `int32_t` values `23` and `-23`.

```c
int32_t x = 23;
int32_t y = -23;
```

Their two's complement representations are

    0000 0000 0000 0000 0000 0000 0001 0111 // x
    1111 1111 1111 1111 1111 1111 1110 1001 // y

This time, let's assume `>>` is an _arithmetic_ right-shift:

```c
printf("x: %d\n",  ((x<<27)>>27)  );
printf("y: %d\n",  ((y<<27)>>27)  );
```

What prints?

Applying the left-shift:

    1011 1000 0000 0000 0000 0000 0000 0000 // x
    0100 1000 0000 0000 0000 0000 0000 0000 // y

Applying the _arithmetic_ right-shift:

    1111 1111 1111 1111 1111 1111 1111 0111 // x
    0000 0000 0000 0000 0000 0000 0000 1001 // y

And so the call to `printf()` gives

    x: -9
    y: 9

The left-shift of `27` bits has clobbered our values!

What about:

```c
printf("x: %d\n",  ((x<<26)>>26)  );
printf("y: %d\n",  ((y<<26)>>26)  );
```

Applying the left-shift:

    0101 1100 0000 0000 0000 0000 0000 0000 // x
    1010 0100 0000 0000 0000 0000 0000 0000 // y

Applying the _arithmetic_ right-shift:

    0000 0000 0000 0000 0000 0000 0001 0111 // x
    1111 1111 1111 1111 1111 1111 1110 1001 // y

And `printf()` gives

    x: 23
    y: -23

This time left-shifting `27` bits lost information, whereas left-shifting `26`
bits did not.

An equivalent definition for the headroom of a _signed_ integer (sometimes
called the "_signed_ headroom") is the number of _redundant_ leading sign bits.

For both `23` and `-23` the 27 most-significant bits were all sign bits. But we
only ever need (at minimum) 1 sign bit. So _26_ of the sign bits were
_redundant_. So the headroom of both `23` and `-23` is `26` bits.

Keeping in mind that the encoding of non-negative values is the same for both
signed and unsigned values, one rule of thumb is that for a non-negative value,
the _signed headroom_ is one bit less than the _unsigned headroom_.

---

Note that for signed integers `x` and `-x` **do not** always have the same
amount of headroom. In particular, if `x` is an integer power of `2`, then `-x`
will have _one more_ bit of headroom.

Let's look at the `int8_t` values `32` and `-32`:

    0010 0000 //  32
    1110 0000 // -32

Here we can see that `32` has only `2` sign bits, whereas `-32` has `3` sign
bits, making their headroom `1` bit and `2` bits respectively.


### Headroom in `lib_xcore_math`

* HR_S32()
* vect_s32_headroom()
* bfp_s32_headroom()

## Choosing Exponents

Generally when performing a BFP operation which outputs a BFP vector, the first
step is to choose which exponent the output vector will use. This is done before
ever looking at the mantissa values -- indeed, it can be done without _access_
to the mantissas. Why? And how?

The _why_ is because we must be sure, before performing the operation, that we
aren't going to overflow the elements of the output vector.

Consider `vect_s16_mul() from `lib_xcore_math`'s Vector API. This
function takes in two `int16_t` arrays and multiplies them element-wise to
produce an `int16_t` output vector.

From [`vect_s16.h`](TODO):
```c
C_API
headroom_t vect_s16_mul(
    int16_t a[],
    const int16_t b[],
    const int16_t c[],
    const unsigned length,
    const right_shift_t a_shr);
```

The operation performed by this function is 

$$
\begin{aligned}
  &\mathtt{a}[k] \gets \frac{(\mathtt{b}[k] \cdot \mathtt{c}[k])}
      {2^{\mathtt{a\_shr}}} \\
  &\qquad\text{for }k = 0,1,2,...(\mathtt{length}-1)
\end{aligned}
$$

Unlike the 32-bit multiplies, in this case the XS3 VPU does not automatically
apply a fixed right-shift to products. So in the particular case where `a_shr`
is `0`, this function attempts to assign each `a[k]` the 32-bit product of
`b[k]` and `c[k]`. The problem is that _most_ pairs of 16-bit numbers don't
yield a product that fits into a 16-bit number!

One solution would be to avoid `vect_s16_mul()` and instead use the 32-bit
products directly. But then what happens when you need the element-wise products
of 32-bit vectors? You could use a 64-bit vector for the results. And then
128-bit vectors?

For the obvious reason, that is generally untenable. In some circumstances it
may be an effective approach, but in other circumstances, the same object must
be recursively updated, doubling in memory each time. The solution is to accept
some quantization error and use BFP vectors, for which `vect_s32_mul()` was
intended.

To make this work with BFP we have to choose an exponent for the output vector
where none of the output elements overflow.

One way we could accomplish this is take a two pass approach. We could compute
each output element on a first pass, only to determine the minimum exponent that
will allow every value to be stored. Then on the second pass we could recompute
each element knowing the correct exponent to use.

But that is wasteful.

---

Still thinking of our 16-bit case with input vectors $\bar{b}$ and $\bar{c}$ and
output vector $\bar{a}$, to determine $\hat{a}$, the exponent associated with
$\bar{a}$, we should figure out the range of output element's _possible_ logical
values. 

$$
\begin{aligned}
& b_k \cdot c_k \\
&= \mathtt{b}[k] \cdot 2^{\hat{b}} 
   \cdot \mathtt{c}[k] \cdot 2^{\hat{c}} \\
&=  2^{\hat{b}+\hat{c}} \cdot \mathtt{b}[k] 
   \cdot \mathtt{c}[k] \\
   \\
\mathtt{INT16\_MIN} \le &\mathtt{b}[k] \le \mathtt{INT16\_MAX} \\
\mathtt{INT16\_MIN} \le &\mathtt{c}[k] \le \mathtt{INT16\_MAX} \\
\mathtt{INT16\_MIN} &= -2^{15} \\
\mathtt{INT16\_MAX} &= 2^{15}-1 \\
\\
-2^{15}\cdot (2^{15}-1) \le \,&\mathtt{b}[k] \cdot \mathtt{c}[k] \le (-2^{15})^2  
\\
-2^{30} + 2^{15} \le \,&\mathtt{b}[k] \cdot \mathtt{c}[k] \le 2^{30}  \\
-2^{30} \lt 
  \,&\mathtt{b}[k] \cdot \mathtt{c}[k] 
\le 2^{30}  
\\
-2^{30} \cdot 2^{\hat{b}+\hat{c}}
  \lt \,2^{\hat{b}+\hat{c}} \cdot &\mathtt{b}[k] \cdot \mathtt{c}[k]
\le 2^{30} \cdot 2^{\hat{b}+\hat{c}}  
\\
-2^{30+\hat{b}+\hat{c}}
  \lt \,2^{\hat{b}+\hat{c}} \cdot &\mathtt{b}[k] \cdot \mathtt{c}[k]
\le 2^{30+\hat{b}+\hat{c}}  
\\
\end{aligned}
$$

Each $a_k$ is within the range $(-2^{30+\hat{b}+\hat{c}},
2^{30+\hat{b}+\hat{c}}]$. Because the upper-bound has equality, we can just
consider that case.

What is the minimum $\hat{a}$ we can use to store $2^{30+\hat{b}+\hat{c}}$ in
the form $\mathtt{a}[k] \cdot 2^{\hat{a}}$?

$\mathtt{a}[k]$ is a signed 16-bit integer, which can store positive values just
shy of $2^{15}$. If we assumed $\mathtt{a}[k]$ was $2^{15}$, then 

$$
\begin{aligned}
  \mathtt{a}[k] \cdot 2^{\hat{a}} &= 2^{30+\hat{b}+\hat{c}} 
  \\
  2^{15}\cdot 2^{\hat{a}} &= 2^{30+\hat{b}+\hat{c}}
  \\
  2^{15+\hat{a}} &= 2^{30+\hat{b}+\hat{c}}
  \\
  15 + \hat{a} &= 30 + \hat{b} + \hat{c}
  \\
  \hat{a} &= 15 + \hat{b} + \hat{c}
\end{aligned}
$$

However, $\mathtt{a}[k]$ **cannot** hold a value of $2^{15}$, it can hold at
most $(2^{15}-1)$. So, we have a choice to make here. We could use $\hat{a} =
15+\hat{b}+\hat{c}$, and saturate any $\mathtt{a}[k]$ that would come out as
$2^{15}$ to $(2^{15}-1)$.  Or we could use $\hat{a} = 14+\hat{b}+\hat{c}$, and
then saturation will not be necessary because it isn't possible.

Both options involve errors on the scale of 1 least significant bit, and may
involve different trade-offs in different applications.

Fortunately, because we've solved this for the general 16-bit case, we don't
have to go through the whole derivation each time we need to do an element-wise
multiplication, we can just use e.g. $\hat{a} = 14+\hat{b}+\hat{c}$.

Once we have $\hat{a}$, we can use that to find $\vec{r}$ the right-shift to be
applied to the accumulator through the `acc_shr` parameter of `vect_s16_mul()`.

To go from the exponent we have ($\hat{b}+\hat{c}$, the result of the 16-bit
multiplication) to the desired exponent $\hat{a}$ through a right-shift
$\vec{r}$ is a simple subtraction:

$$
\begin{aligned}
  \vec{r} &\gets \hat{a} - (\hat{b} + \hat{c}) \\
  \vec{r} &\gets (14+\hat{b}+\hat{c}) - (\hat{b} + \hat{c}) \\
  \vec{r} &\gets 14 
\end{aligned}
$$

---

So, we saw that multiplying 16-bit BFP vectors $\bar{b}$ and $\bar{c}$ resulted
in an increase in our exponent by 14. If we then need to multiply $\bar{a}$ by
another 16-bit BFP vector, won't it increase by another 14? Won't the exponent
just keep going up and up through successive multiplications, irrespective of
the data being multiplied, until all of the elements round to zero?

One way to stop that from happening is to remove headroom from the resulting BFP
vector. We used the maximum possible output to determine the exponent. In most
situations we don't end up with the maximum _possible_ values, and so we may end
up with headroom in our resulting BFP vector.

After computing $\hat{a}$, its headroom $h_a$ can be computed, and then we can
'normalize' the BFP vector by removing the headroom. This is a simple matter of
left-shifting each $\mathtt{a}[k]$ by $h_a$ bits, and then adding $h_a$ to the
exponent $\hat{a}$.

You may have noticed that the return type of `vect_s16_mul()` is `headroom_t` --
the function returns the headroom for you, so there's no need to separately
calculate it.

---

Instead of removing the headroom after calculating $\bar{a}$, however, we can
just _keep track of it_. The structs repesenting BFP vectors in `lib_xcore_math`
have a field `headroom_t hr` used for keeping track of the vector's headroom.

So what if we had kept track of $\bar{b}$ and $\bar{c}$'s headrooms, and we know
that our input mantissa vectors $\mathtt{b}[\,]$ and $\mathtt{c}[\,]$ have
headroom $h_b$ and $h_c$ respectively? How can we incorporate that information.

Recall that one interpretation of headroom is the number of bits the data can be
left-shifted without losing information. This is the same as saying that the
data could theoretically be stored in an integer with a smaller bit-depth, which
in turn implies a smaller range of possible values.

Specifically, if the range of an `int16_t` with `0` bits of headroom is from
`INT16_MIN` to `INT16_MAX`, then the range of possible values of an `int16_t`
value with `n` bits of headroom will be from `(INT16_MIN>>n)` to
`(INT16_MAX>>n)`.

If we have $\bar{b}$ and $\bar{c}$ with exponents $\hat{b}$ and $\hat{c}$ and
headroom $h_b$ and $h_c$, then we could apply logic similar to that above to
find that

$$
-2^{30-(h_b+h_c)}
  \lt \mathtt{b}[k] \cdot \mathtt{c}[k]
\le 2^{30-(h_b+h_c)}  
$$

which then tells us we can find the output exponent as

$$
  \hat{a} = 14 + (\hat{b} + \hat{c}) - (h_b + h_c)
$$

Using this new method for selecting the output exponent we avoid the scenarios
where the exponent grows and grows, irrespective of the headroom of the mantissa
vector.



## BFP in `lib_xcore_math`

### `*_prepare()` Functions in `lib_xcore_math`

To faciliate BFP operations, many of the functions in `lib_xcore_math`'s Vector
API have a companion "prepare" function. Usually this is the function's name with `_prepare` appended to the end.

For example, along with `vect_s16_mul()` is a function `vect_s16_mul_prepare()`:

From [`vect_s16_prepare.h`](TODO):
```C
C_API
void vect_s16_mul_prepare(
    exponent_t* a_exp,
    right_shift_t* a_shr,
    const exponent_t b_exp,
    const exponent_t c_exp,
    const headroom_t b_hr,
    const headroom_t c_hr);
```

Notice that `vect_s16_mul_prepare()` takes in the exponents and headrooms
associated with vectors `b[]` and `c[]`, but does _not_ take `b[]` or `c[]`
directly. The idea of the prepare functions is to incorporate available
information (aside from actual input element values) to select an output
exponent, and any other accessory input parameters required by the function it
is preparing for.

In the case of `vect_s16_mul_prepare()`, `a_exp` and `a_shr` are _output_ parameters. The output exponent is output through `a_exp`, and the right-shift parameter `acc_shr` required by `vect_s16_mul()` is output through `a_shr`.

Contrast this with `vect_s32_mul_prepare()`:

From [`vect_s32_prepare.h`](TODO):
```C
C_API
void vect_s32_mul_prepare(
    exponent_t* a_exp,
    right_shift_t* b_shr,
    right_shift_t* c_shr,
    const exponent_t b_exp,
    const exponent_t c_exp,
    const headroom_t b_hr,
    const headroom_t c_hr);
```

This function outputs 2 shift parameters, `b_shr` and `c_shr`, which match those required by `vect_s32_mul()`:

From [`vect_s32.h`](TODO):
```C
C_API
headroom_t vect_s32_mul(
    int32_t a[],
    const int32_t b[],
    const int32_t c[],
    const unsigned length,
    const right_shift_t b_shr,
    const right_shift_t c_shr);
```

Finally, let's take a look at `vect_s32_dot_prepare()`, which we will encounter
in **Part 3B**:

From [`vect_s32_prepare.h`](TODO):
```C
C_API
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

This closely mirrors `vect_s32_mul_prepare()`, except it also takes a `length`
parameter.

Whereas `vect_s32_mul()` performs element-wise multiplication on two input
vectors, `vect_s32_dot()` performs element-wise multiplication and then _sums up
the products_. Because of that, `vect_s32_dot_prepare()` also needs to know how
many products are being summed together. 

If `2` element-wise products are being added together, then the range of
possible values of the result is _twice_ that of the element-wise products
themselves, implying the exponent will need to be incremented to avoid
overflows. If `16` element-wise products are being added, the result may be 16
times as large, requiring an extra `4` bits ( $4=log_2(16)$ ) to store the result, or an increment of the output exponent by `4`.


### BFP Types


`bfp_s32_t` is a struct type from `lib_xcore_math` which
represents a BFP vector with 32-bit mantissas. The `bfp_s32_t` type has 4
important fields:

Field             | Description
-----             | -----------
`int32_t* data`   | A pointer to an `int32_t` array; the buffer which backs the mantissa vector.
`unsigned length` | Indicates the number of elements in the BFP vector. 
`exponent_t exp`  | The exponent associated with the vector's mantissas.
`headroom_t hr`   | The headroom of the vector's mantissas.

> Note: in `lib_xcore_math`, unless otherwise specified, a vector's "length" is
> always the number of elements, rather than its Euclidean length.

### BFP Operations

Several BFP operations will be encountered in **Part 3** when we get to [**Part 3C**](stage8.md).

#### `bfp_s32_init()`

Initialization of a `bfp_s32_t` (32-bit BFP vector) is done with a call to
[`bfp_s32_init()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L17-L45).
The final parameter, `calc_hr` indicates whether the headroom should be
calculated during initialization. This is unnecessary when the element buffer
has not been filled with data.

#### `bfp_s32_dot()`

[`bfp_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/bfp/bfp_s32.h#L498-L522)
computes the inner product between two BFP vectors, but unlike `vect_s32_dot()`,
it takes care of all the management of exponents and headroom for us.
`bfp_s32_dot()` basically encapsulates all the logic being performed in **Part
3B**'s `filter_sample() (apart from converting the output to the proper
fixed-point representation).

#### `bfp_s32_headroom()`

#### `bfp_s32_use_exponent()`