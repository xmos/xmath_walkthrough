# Part 3: Block Floating-Point Arithmetic

With fixed-point arithmetic we had to be mindful of the range of logical values
our samples may take, so as to avoid overfloating the mantissas used to
represent them. BFP arithmetic will allow us to avoid making those decisions at
compile time. Instead we will have to dynamically choose exponents as we go to
perform an operation, based on what we know about the inputs to that operation.

```{toctree}
---
maxdepth: 1
---
./part3_intro.md
./part3_concepts.md
./part3_on_xcore.md
./part3A.md
./part3B.md
./part3C.md
```

In this section:

<details>
    <summary markdown="span">A quick reminder</summary>

In **Part 2** our digital FIR filter was implemented using fixed-point
arithmetic. In **Part 3** we will update it to use block floating-point (BFP)
arithmetic.

</details>

* In [Part 3A](./part3A.md), both the BFP vector objects and the BFP vector arithmetic are
implmented in plain C, to demonstrate how the operations actually work under the
hood.

* In [Part 3B](./part3B.md), some of the C routines from **Part 3A** are replaced with
optimized library calls to `lib_xcore_math`'s vector API.

* In [Part 3C](./part3C.md), the BFP vector objects from **Part 3A** are replaced with BFP
vector types defined in `lib_xcore_math`, and the vector API calls from **Stage
7** are replaced with operations from `lib_xcore_math`'s BFP API.


