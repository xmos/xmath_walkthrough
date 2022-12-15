
# Part 4A

Up to this point, all previous stages do all of their filter computation in a
single hardware thread. Each tile on an xcore XS3 device has 8 hardware threads
available, and most of these threads have so far gone unused. In **Part 4A** the
application will be parallelized so that the filter computation will span
several threads to reduce latency.

**Part 4A** will take a block floating-point approach most similar to that of
[**Part 3B**](part3B.md). The reason this stage is modeled on **Part 3B**
instead of [**Part 3C**](part3C.md) is because `lib_xcore_math`'s high-level BFP
API will not play nicely with this sort of parallelism. For advanced usage like
this, the lower level API is required.

## From `lib_xcore_math`

This page references the following operations from `lib_xcore_math`:

* [`vect_s32_headroom()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L554-L591)
* [`vect_s32_dot()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L399-L480)
* [`vect_s32_dot_prepare()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32_prepare.h#L182-L252)
* [`vect_s32_shr()`](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/vect/vect_s32.h#L1200-L1239)

## Implementation

**Part 4A** has its implementation split across two files, `part4A.c` and `stage9.xc`. The only function implemented in `stage9.xc` is `filter_frame()`, and this is because writing it in XC allows us to take advantage of the XC language's convenient syntax for synchronous parallel operations using a `par` block.

In fact, in **Part 4A**, the only function which is implemented differently than in **Part 3B** is `filter_frame()`.

---

```{literalinclude} ../../src/part4A/part4A.c
---
language: C
start-after: +filter_frame
end-before: -filter_frame
---
```

The internals of **Part 4A**'s `filter_frame()` also closely match that of
**Part 3B**. In **Part 4A**, all of the parallelism happens within the
`par`-block. Semantically, and conceptually, this works much like a `for`-loop.
The difference is, of course, that instead of each "iteration" happening
sequentially, with the `par`-block each "iteration" happens simultaneously.

As is often the case, there are multiple options for how to introduce the
parallelism. For example, one option would have been to move the parallelism
down into `filter_sample()`. In that case we could have had each thread compute
a portion of the inner product and then leave it to the main (calling) thread to
add the partial results together.

Instead, because we're dealing with frames of data, **Part 4A** opts to have
each of the worker threads computing a different output sample. Specifically, on
each iteration of the `for` loop, the next `THREADS` output samples are to be
computed (notice `s` increments by `THREADS` each iteration). On each iteration,
the `par` block starts `THEADS` (4) threads, which each compute a different
output sample. The index of the output sample each thread calculates is based on
the value it gets for `tid`.

Note that _yet another_ way the parallelism could have been implemented would be
to assign each of the worker threads a _range_ of the output samples, each
thread internally iterating over its own assigned output sample range. This
effectively would have just inverted the `par` block and `for` loop. In
practice, this way is most likely better, because there is less overhead from
starting and synchronizing the thread group (done once per frame instead of 64
times). However, `timer_start()` and `timer_stop()` are not thread-safe, and so
this would have bungled our performance information.

## Results

```{note} 
The sample timing info below is per FOUR samples
```

### Timing

| Timing Type       | Measured Timing
|-------------------|-----------------------
| Per Filter Tap    | 15.05 ns
| Per Output Sample | 15413.09 ns
| Per Frame         | 1033652.88 ns

### Output Waveform

![**Part 4A** Output](img/part4A.png)