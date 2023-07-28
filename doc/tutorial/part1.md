
# Part 1: Floating-Point Arithmetic

**Part 1** deals with floating-point arithmetic.

```{toctree}
---
maxdepth: 1
---
./part1_intro.md
./part1A.md
./part1B.md
./part1C.md
```

It is useful to start with a floating-point implementation, and particularly one
written in plain C, because many users approach xcore.ai with their algorithms
implented in floating-point and plain C for the sake of portability.


In this section:

* [Part 1A](./part1A.md) implements the FIR using floating-point arithmetic with
double-precision floating-point values. It is implemented in plain C so that
nothing is hidden from the reader.

* [Part 1B](./part1B.md)  is nearly identical to **Part 1A**, except single-precision
floating-point values are used. It is also implemented in plain C.

* [Part 1C](./part1C.md)  uses single-precision floating-point values like **Part 1B**, but
will use a library function from `lib_xcore_math` to do the heavy lifting, both
simplifying the implementation and getting a hefty performance boost.