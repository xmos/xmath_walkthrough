# Using `lib_xcore_math` for BFP arithmatic

## This page makes references the following operations from `lib_xcore_math`:

```{eval-rst}
.. doxygenfunction:: vect_s32_dot
   :project: XCORE.AI MATH
   :outline:

.. doxygenfunction:: vect_s32_mul
   :project: XCORE.AI MATH
   :outline:

.. doxygenfunction:: vect_s32_headroom
   :project: XCORE.AI MATH
   :outline:

.. doxygenfunction:: bfp_s32_headroom
   :project: XCORE.AI MATH
   :outline:

Detailed documentation:
:ref:`vect_s32_dot<anchor_vect_s32_dot>` | 
:ref:`vect_s32_mul<anchor_vect_s32_mul>` | 
:ref:`vect_s32_headroom<anchor_vect_s32_headroom>` | 
:ref:`bfp_s32_headroom<anchor_bfp_s32_headroom>`

and the utility function *HR_S32()* which gets the headroom of an int32_t

```

## `*_prepare()` Functions in `lib_xcore_math`

To faciliate BFP operations, many of the functions in `lib_xcore_math`'s Vector
API have a companion "prepare" function. Usually this is the function's name with `_prepare` appended to the end.

For example, along with `vect_s16_mul()` is a function `vect_s16_mul_prepare()`:

```{eval-rst}
.. doxygenfunction:: vect_s16_mul_prepare
   :project: XCORE.AI MATH
   :outline:
```

Notice that `vect_s16_mul_prepare()` takes in the exponents and headrooms
associated with vectors `b[]` and `c[]`, but does _not_ take `b[]` or `c[]`
directly. The idea of the prepare functions is to incorporate available
information (aside from actual input element values) to select an output
exponent, and any other accessory input parameters required by the function it
is preparing for.

In the case of `vect_s16_mul_prepare()`, `a_exp` and `a_shr` are _output_ parameters. The output exponent is output through `a_exp`, and the right-shift parameter `acc_shr` required by `vect_s16_mul()` is output through `a_shr`.

Contrast this with `vect_s32_mul_prepare()`:

```{eval-rst}
.. doxygenfunction:: vect_s32_mul_prepare
   :project: XCORE.AI MATH
   :outline:
```

This function outputs 2 shift parameters, `b_shr` and `c_shr`, which match those required by `vect_s32_mul()`:

Finally, let's take a look at `vect_s32_dot_prepare()`, which we will encounter
in **Part 3B**:

```{eval-rst}
.. doxygenfunction:: vect_s32_dot_prepare
   :project: XCORE.AI MATH
   :outline:
```

s closely mirrors `vect_s32_mul_prepare()`, except it also takes a `length`
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

Several BFP operations will be encountered in **Part 3** when we get to [**Part 3C**](part3C.md).

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