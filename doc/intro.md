
Prev | Home | [Next](building.md)

## Introduction

This repository is a hands-on tutorial for developers interested in writing
efficient DSP or other compute-heavy logic on xcore devices.

We focus on a particular use case: implementing a digital FIR filter. The filter
will initially be implemented in a highly straight-forward but inefficient way,
and then we will 

* explore various approaches that can be taken to improve the implementation, 
* see some of the APIs in the `lib_xcore_math` library which can be used to
help, and
* introduce and discuss some of the theoretical and conceptual elements
encountered along the way.

## Tutorial Organization

This tutorial is organized into 4 parts, each with several stages. All stages
implement the same FIR filter, each using a different approach.

Each stage 

* Explains our motivation for using that implementation
* Introduces any new concepts or background required for understanding that
  stage's implementation
* Introduces any new `lib_xcore_math` library calls
* Explains the code changes (usually from the previous stage) required for the
  new implementation
* Discusses the observed performance of that stage's implementation

### Part A -- Floating-point

The first three stages (0-2) implement the FIR filter using floating-point
arithmetic. Floating-point arithmetic is the easiest way to implement the
digital filter because the application developer need not worry (much) about the
dynamic range of variables and need not manually manage the book-keeping of
exponents and headroom.

Ultimately floating-point arithmetic will not yield the most performant
implementation, as the xcore device only has a scalar floating-point unit.

### Part B -- Fixed-point

The next three stages (3-5) implement the FIR filter using fixed-point
arithmetic. Once an application developer overcomes the conceptual hurdle of
reasoning about exponents and conversion between fixed-point representations,
fixed-point arithmetic is a convenient and fast alternative to floating-point
arithmetic.

Fixed-point arithmetic, while fast and easy, has some short-comings. In
particular, because a specific exponent and bit-depth must be chosen at compile
time, the resolution and range of the fixed-point values are also fixed. In some
situations this may lead to a lack of precision, and in others it may lead to
overflows or saturation. In the end fixed-point is ideal only when the system's
input values or internal states do not vary over a wide dynamic range.

### Part C -- Block Floating-point

The next three stages (6-8) implement the FIR filter using block floating-point
(BFP) arithmetic. Unlike fixed-point, BFP allows the relevant objects to vary
greatly in scale, but unlike floating-point, BFP's arithmetic operations at
bottom operate on arrays of integers, allowing the xcore VPU to accelerate
computation.

The down side of BFP arithmetic is that it requires some additional overhead in
the book-keeping of vector exponents and headroom. Managing these can be rather
cumbersome. In this part we'll also see how `lib_xcore_math`'s BFP API will
simplify much of this book-keeping work for us.

### Part D -- Miscellaneous

The last 4 stages (9-12) comprise Part D. Each of these stages addresses a
related topic of interest, such as parallelizing the work across cores, and
using `lib_xcore_math`'s provided digital filter API.

## Stages

| Part           | Stage                        | Details
|----------------|------------------------------| -----------
| Floating-point | [Stage 0](stage0.md)   | Plain C; Double-precision
|                | [Stage 1](stage1.md)   | Plain C; Single-precision
|                | [Stage 2](stage2.md)   | `lib_xcore_math` vector API
| Fixed-point    | [Stage 3](stage3.md)   | Plain C
|                | [Stage 4](stage4.md)   | Optimized assembly; scalar unit only
|                | [Stage 5](stage5.md)   | `lib_xcore_math` vector API
| BFP            | [Stage 6](stage6.md)   | Plain C
|                | [Stage 7](stage7.md)   | `lib_xcore_math` vector API
|                | [Stage 8](stage8.md)   | `lib_xcore_math` BFP API
| Misc           | [Stage 9](stage9.md)   | Multi-threaded BFP
|                | [Stage 10](stage10.md) | `lib_xcore_math` digital filter API
|                | [Stage 11](stage11.md) | `lib_xcore_math` filter generation script
|                | [Stage 12](stage12.md) | `float`-wrapped BFP.

