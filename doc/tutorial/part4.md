
# Part 4: Miscellaneous

**Part 4** contains several stages which demonstrate various mostly-unrelated
features.

```{toctree}
---
maxdepth: 1
---
./part4A.md
./part4B.md
./part4C.md
```

In this section:

* [Part 4A](./part4A.md) demonstrates how an application can use multiple hardware threads to
implement the FIR filter in a simple, synchronous, parallel way.

* [Part 4B](./part4B.md) introduces the `lib_xcore_math` [Digital Filter
API](https://github.com/xmos/lib_xcore_math/blob/v2.1.1/lib_xcore_math/api/xmath/filter.h),
and how to implement the filter using this API.

* [Part 4C](./part4C.md) shows how one of the digital filter conversion scripts provided with
`lib_xcore_math` can be used to simplify filter implementation and usage even further.