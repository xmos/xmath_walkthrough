
# Part 3: Block Floating-Point Arithmetic

In **Part 2** our digital FIR filter was implemented using fixed-point
arithmetic. In **Part 3** we will update it to use block floating-point (BFP)
arithmetic.

```{toctree}
---
maxdepth: 1
---
./part3_intro.md
./part3A.md
./part3B.md
./part3C.md
```

With fixed-point arithmetic we had to be mindful of the range of logical values
our samples may take, so as to avoid overfloating the mantissas used to
represent them. BFP arithmetic will allow us to avoid making those decisions at
compile time. Instead we will have to dynamically choose exponents as we go to
perform an operation, based on what we know about the inputs to that operation.

