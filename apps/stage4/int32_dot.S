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