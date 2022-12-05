
Prev | Home | [Next](building.md)

## Introduction

This repository is a hands-on tutorial for developers interested in writing
efficient DSP or other compute-heavy logic on xcore devices.

We focus on one particular use case: implementing a digital FIR filter. The
filter will initially be implemented in a highly straight-forward but
inefficient way, and then we will 

* explore various approaches that can be taken to improve the implementation, 
* see some of the APIs in the `lib_xcore_math` library which can be used to
help, and
* introduce and discuss some of the theoretical and conceptual elements
encountered along the way.

## Tutorial Organization

This tutorial is organized into 4 parts, each with 3 stages. All stages
implement the same FIR filter, each using a different approach.

Each stage 

* Explains our motivation for using that implementation
* Introduces any new concepts or background required for understanding that
  stage's implementation
* Introduces any new `lib_xcore_math` library calls
* Explains the stage-specific implementation
* Discusses the observed performance of that stage's implementation

### Part 1 -- Floating-point

The part implements the FIR filter using floating-point arithmetic.
Floating-point arithmetic is the easiest way to implement the digital filter
because the application developer need not worry (much) about the dynamic range
of variables and need not manually manage the book-keeping of exponents and
headroom.

Ultimately floating-point arithmetic will not yield the most performant
implementation, as the xcore device only has a scalar floating-point unit.

### Part 2 -- Fixed-point

The part implements the FIR filter using fixed-point arithmetic. Once an
application developer overcomes the conceptual hurdle of reasoning about
exponents and conversion between fixed-point representations, fixed-point
arithmetic is a convenient and fast alternative to floating-point arithmetic.

Fixed-point arithmetic, while fast and easy, has some short-comings. In
particular, because a specific exponent and bit-depth must be chosen at compile
time, the resolution and range of the fixed-point values are also fixed. In some
situations this may lead to a lack of precision, and in others it may lead to
overflows or saturation. In the end fixed-point is ideal only when the system's
input values or internal states do not vary over a wide dynamic range.

### Part 3 -- Block Floating-point

The third part implements the FIR filter using block floating-point (BFP)
arithmetic. Unlike fixed-point, BFP allows the relevant objects to vary greatly
in scale, but unlike floating-point, BFP's arithmetic operations at bottom
operate on arrays of integers, allowing the xcore VPU to accelerate computation.

The down side of BFP arithmetic is that it requires some additional overhead in
the book-keeping of vector exponents and headroom. Managing these can be rather
cumbersome. In this part we'll also see how `lib_xcore_math`'s BFP API will
simplify much of this book-keeping work for us.

### Part 4 -- Miscellaneous

Each of the stages in part 4 addresses a related topic of interest, such as
parallelizing the work across cores, and using `lib_xcore_math`'s provided
digital filter API.

## Stages

| Part           | Stage                        | Details
|----------------|------------------------------| -----------
| Floating-point | [Part 1A](part1A.md)   | Double-precision; Plain C
|                | [Part 1B](part1B.md)   | Single-precision; Plain C
|                | [Part 1C](part1C.md)   | Single-precision; `lib_xcore_math` vector API
| Fixed-point    | [Part 2A](part2A.md)   | Fixed-point in plain C
|                | [Part 2B](part2B.md)   | Optimized assembly; scalar unit only
|                | [Part 2C](part2C.md)   | `lib_xcore_math` vector API
| BFP            | [Part 3A](part3A.md)   | Block floating-point in plain C
|                | [Part 3B](part3B.md)   | `lib_xcore_math` vector API
|                | [Part 3C](part3C.md)   | `lib_xcore_math` BFP API
| Misc           | [Part 4A](part4A.md)   | Multi-threaded BFP
|                | [Part 4B](part4B.md) | `lib_xcore_math` digital filter API
|                | [Part 4C](part4C.md) | `lib_xcore_math` filter generation script

