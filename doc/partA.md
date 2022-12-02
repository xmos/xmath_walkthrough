
[Prev](sw_organization.md) | [Home](intro.md) | [Next](stage0.md)

# Part A: Floating-Point Arithmetic

**Part A** comprises [**Stage 0**](stage0.md), [**Stage 1**](stage1.md) and
[**Stage 2**](stage2.md) and deals with floating-point arithmetic.

Each stage's behavior is defined by 4 functions, `rx_frame()`, `tx_frame()`,
`filter_sample()` and `filter_task()`. These functions will be defined in most
of the following stages as well. In some cases these functions are identical
between stages. In other cases, for example, **Stage 2**, the implementation of
`filter_sample()` will be a single function call whose result is returned. The
reason for this structure is to make the stages within each part easy to
compare.

When looking at the code for each stage, these are the functions we'll examine.

> **Note**: The _signatures_ for these functions are _not_ the same for all
> stages.

> **Note**: The frame size (`FRAME_SIZE`) is `256` samples.

## `filter_task()`

This is the thread entry-point for the filtering thread. This function will
generally declare needed buffers for input and output sample data, initialize
them, and then go into an infinite loop.

In each loop iteration, `filter_task()` will read in a new frame of input audio samples (using `rx_frame()`), compute one frame's worth of output audio samples (using `filter_sample()`) and then send out the frame of output samples back to the `wav_io` thread on tile 0.

## `filter_sample()`

Each call to `filter_sample()` will compute exactly one output sample using the
history of received samples. The implementation of this function will change for
every stage.

## `rx_frame()`

This function accepts a frame of input audio samples from the `wav_io` thread
running on tile 0 via a channel. In most cases, this function will store the
received samples into the sample history _in reverse chronological order_, to
ensure the ordering of sample data matches the order of filter coefficients.

> **Note**: We happen to be using a symmetric filter, so the ordering isn't
> actually important in our case. However, in general the order does matter.

## `tx_frame()`

This function uses a channel to send a frame of output audio samples back to the
`wav_io` thread on tile 0.