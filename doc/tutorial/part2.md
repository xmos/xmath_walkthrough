
# Part 2: Fixed-Point Arithmetic

**Part 2** migrates our digital FIR filter to use fixed-point arithmetic instead
of floating-point arithmetic.

```{toctree}
---
maxdepth: 1
---
./part2_intro.md
./part2A.md
./part2B.md
./part2C.md
```

With fixed-point arithmetic we must be mindful about the possible range of
(logical) values that the data may take. Knowing the range of possible values
allows us to select exponents for our input, output and intermediate
representations of the data which avoid problems like overflows, saturation or
excessive precision loss.

In this section:

* [Part 2A](./part2A.md) migrates the filter implementation to use fixed-point arithmetic.
Like **Parts** **1A** and **1B**, the implementation will be written in plain C
so the reader sees everything that is going on.

* [Part 2B](./part2B.md) replaces the plain C implementation of `filter_sample()` from **Part
2A** with a custom assembly routine which uses the scalar arithmetic unit much
more efficiently.

* [Part 2C](./part2C.md) replaces the custom assembly routine with a call to a library
function from `lib_xcore_math` which uses the VPU to greatly accelerate the
arithmetic.