
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
