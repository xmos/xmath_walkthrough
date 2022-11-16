
[Prev](../stage3/index.md) | [Home](../intro.md) | [Next](../stage5/index.md)

# Stage 4

Like [**Stage 3**](../stage3/index.md), **Stage 4** implements the FIR filter
using fixed-point arithmetic. 

In **Stage 3** we implemented the fixed-point arithmetic directly in plain C. As
such, we were left to hope that the compiler generates an efficient sequence of
instructions to do the work. However, there are reasons to doubt the compiler
generates an optimal implementation. 

One reason is that the C compiler will not target the VPU when compiling code. Another is that the compiler will not ordinarily generate dual-issue implementations of functions written in C.

In **Stage 4**, instead of implementing the logic in a plain C loop, we will use
a dual-issue function written in assembly to compute the inner product for us.
That function, `int32_dot()`, will not use the VPU (we'll use the VPU in [**Stage 5**](../stage5/index.md)), but is meant to demonstrate the improved performance we get just from using a dual-issue function written directly in assembly.

## Introduction

## Background

## Code Changes

Moving from **Stage 3** to **Stage 4** we see two differences in the code.

First, **Stage 4** replaces the `for` loop in `filter_sample()` with a call to
`int32_dot()`

From `stage3.c`:
```c
q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  int64_t acc = 0;

  for(int k = 0; k < TAP_COUNT; k++){
    const int64_t smp = sample_history[k];
    const int64_t coef = filter_coef[k];
    acc += (smp * coef);
  }

  return ashr64(acc, acc_shr);
}
```

From `stage4.c`:
```c
q1_31 filter_sample(
    const q1_31 sample_history[TAP_COUNT])
{
  int64_t acc = int32_dot(&sample_history[0], &filter_coef[0], TAP_COUNT);

  return ashr64(acc, acc_shr);
}
```

Second, **Stage 4** includes `int32_dot.S`, containing the implementation of 
`int32_dot()`.

```c
// Copyright (c) 2022, XMOS Ltd, All rights reserved
#if defined(__XS3A__)

.text
.issue_mode  dual

/*
  int64_t int32_dot(
      const int32_t x[],
      const int32_t y[],
      const unsigned length);
*/

#define FUNC_NAME     int32_dot
#define NSTACKWORDS   2

.globl	FUNC_NAME
.type	FUNC_NAME,@function
.cc_top FUNC_NAME.function,FUNC_NAME

#define x         r0
#define y         r1
#define len       r2
#define acc_hi    r3
#define acc_lo    r4
#define tmpA      r5
#define tmpB      r11

.align 16
FUNC_NAME:
  dualentsp NSTACKWORDS
  std r4, r5, sp[0]

{ ldc acc_hi, 0               ; ldc acc_lo, 0               }
{ sub len, len, 1             ; bf len, .done               }

.L_loop_top:
  {                             ; ldw tmpA, x[len]            }
  {                             ; ldw tmpB, y[len]            }
    maccs acc_hi, acc_lo, tmpA, tmpB
  { sub len, len, 1             ; bt len, .L_loop_top         }
.L_loop_bot:

{ mov r1, acc_hi              ; mov r0, acc_lo              }

.done:
  ldd r4, r5, sp[0]
  retsp NSTACKWORDS
    
	.cc_bottom FUNC_NAME.function
	.set	FUNC_NAME.nstackwords,NSTACKWORDS;     .globl	FUNC_NAME.nstackwords
	.set	FUNC_NAME.maxcores,1;                  .globl	FUNC_NAME.maxcores
	.set	FUNC_NAME.maxtimers,0;                 .globl	FUNC_NAME.maxtimers
	.set	FUNC_NAME.maxchanends,0;               .globl	FUNC_NAME.maxchanends
.Ltmp1:
	.size	FUNC_NAME, .Ltmp1-FUNC_NAME
#endif
```

The full details of everything happening here are beyond the scope of this
document, but there are a couple things worth pointing out.

First, `int32_dot()` uses the xcore's _dual-issue_ mode, which allows certain
pairs of 16-bit instructions to be issued and executed together in a single 
thread cycle. The `.issue_mode dual` directive tells the assembler to generate
dual-issue code. Each line containing a `{ ... ; ... }` is a dual-issue 
instruction packet with two instructions bundled together. This allows us to do
more work in fewer instructions.

The second thing to point out is the inner loop, from `.L_loop_top:` to
`.L_loop_bot:` is only 4 instructions long. A disassembly of the **Stage 3**
firmware (`xobjdump -D stage3.xe`) shows that in this case the inner loop of the
(single-issue) `filter_frame()` in **Stage 3** is 7 instructions long.

## Results

The ratio of **Stage 4**'s execution time to **Stage 3**'s execution time is
almost exactly `(4/7)`.

## References

* [`lib_xcore_math`](https://github.com/xmos/lib_xcore_math)