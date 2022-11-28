
[Prev](building.md) | [Home](intro.md) | [Next](stage0.md)

# Software Organization

Each of the firwares built in this tutorial are variations of the same basic
application. The application runs on an xcore Explorer board[^1], reads a `wav`
file in from the host machine, processes the `wav` file audio through a digital
FIR filter, and writes out the processed audio to a new `wav` file on the host
machine.

> TODO: Add diagram showing data flow between host and xcore.

## Repositories

There are four Git repositories associated with this tutorial.

The [`xmath_walkthrough`](TODO) repository has the content of the tutorial (both
documentation and application logic) and defines the workspace layout (through
[`west.yml`](TODO)).

The [`lib_xcore_math`](TODO) repository is a library of optimized functions for
fast arithmetic on xcore XS3. Much of this tutorial is about demonstrating how
pieces of this library are used.

The [`xscope_fileio`](TODO) repository is a library for accessing the host
machine's filesystem from an application running on an xcore device.

The [`xmos_cmake_toolchain`](TODO) repository contains boilerplate `.cmake`
files which tell CMake how to use the xcore toolchain.

The `xmath_walkthrough` and `xmos_cmake_toolchain` repositories are cloned into
the root of the tutorial workspace. The `lib_xcore_math` and `xscope_fileio`
repositories are cloned into `xmath_walkthrough` (this simplifies the CMake
project).

## Libraries

Two external libraries are used in this tutorial, `lib_xcore_math` and
`xscope_fileio`.

### `xscope_fileio`

The `xscope_fileio` library works are a bridge between the firmware running on
an xcore device and the host system. Its implementation uses XMOS's xscope
technology for bi-directional communication between the host system and
firmware. On the firmware it presents and simple API for reading and writing
files on the host system.

### `lib_xcore_math`

The `lib_xcore_math` library contains lots of functions meant to accelerate
arithmetic on xcore.ai. xcore.ai devices use the XMOS XS3 architecture, which
introduces the XMOS proprietary Vector Processing Unit (VPU). The VPU is a
dedicated piece of hardware on xcore providing SIMD-type parallelism of data
manipulation. 

The library contains many low-level functions for very quickly processing
vectors of data (primarily 16- and 32-bit) in various ways -- its [vector
API](TODO). The VPU is very fast, but it implements only integer arithmetic. To
that end, the library also provides a block floating-point [(BFP) API](TODO)
which allows the VPU to be used for fixed-precision (rather than fixed-range)
arithmetic.

`lib_xcore_math` also has APIs for [Fast Fourier Transforms](TODO), [linear
digial filters](TODO) and a small [scalar arithmetic API](TODO).

This purpose this tutorial is not to demonstrate everything available in
`lib_xcore_math`. Rather, it is meant to demonstrate how floating-point
arithmetic can be implemented and optimized on xcore.ai, and to help the user
understand the arithmetic logic involved in the optimizations. So, ultimately
this tutorial will only present a small fraction of the APIs provided by
`lib_xcore_math`, focused on the particular case of a digital FIR filter.

## CMake Projects

Two CMake projects are used in this tutorial. One for the host application and
one for the firmware that is run on the xcore device.

To simplify the coordination of the various components (scripts, data, build
artifacts, application output), this tutorial requires that the CMake build
directory for each CMake project sit in the tutorial workspace's root.

The following sections briefly describe the CMake projects' structure. For
instructions on configuring the CMake projects and building the binaries for
this tutorial, see [Building](building.md).

### Host Application

The CMake source directory for the host application is the `host/` directory
within the cloned `xscope_fileio` repository. The application is defined
entirely by the `xscope_fileio` library and doesn't require customization for
this tutorial.

A single executable target is defined, which is the application itself. The
`install` target of this CMake project will create a `xscope_fileio` directory
in the workspace root, which is where the `xscope_fileio` Python module will
expect to find it.

### Firmware

The CMake source directory for the xcore.ai firmware is the root of the cloned
`xmath_walkthrough` repository. 

It defines an executable target for each stage of this tutorial. These are
called `stage0`, `stage1`, ..., `stage12`. The `lib_xcore_math` and
`xscope_fileio` libraries are also added to the CMake project, as the firmware
depends on each of these.

## Applications

### Host Application

When the device firmware is run with `xrun` (with the appropriate options),
`xrun` opens a socket on the host which allows interfacing with the xscope
probes (defined in the firmware) using the xscope endpoint API provided with the
xcore toolchain. The host application connects to that socket and then uses the
`xscope_fileio` library to act as a sort of host-side filesystem driver. 

The details of how the host application is implemented are beyond the scope of
this tutorial.

```
  TODO -- diagram showing 
    (wav files) <-> (xscope_fileio bridge) <-> (wav thread) <-> (filter thread)
    (xscope_fileio bridge) =   (host app) <-> (xrun xscope socket) <-> (xscope on device)
```

### Xcore Firmware

The firmware run on the xcore.ai device is a relatively simple application
which, in broad strokes, does the following:

1. Waits for host application to establish an xscope connection.
2. Remotely opens two `wav` files on the host, one for input and one for output.
3. Reads a frame of audio data from the input `wav` file.
4. Passes the frame of audio data to the filtering thread for processing.
5. Waits for a frame of processed audio from the filtering thread.
6. Write the processed audio to the output `wav` file.
7. Loops back to step 3 until there is no more input audio.

Steps 2, 3 and 6 above use the `xscope_fileio` API.

The application is a two-tile application with (in most stages) two threads
running on each tile.

| Tile      | Entry Point           | Description |
|-----------|-----------------------|-------------|
| `tile[0]` | `xscope_host_data()`  | Part of `xscope_fileio`; Responsible for handling file communication between the xcore and host.
| `tile[0]` | `wav_io_thread()`     | Responsible for decoding and encoding the input and output `wav` files and framing the audio data.
| `tile[1]` | `filter_thread()`     | Responsible for producing output audio from input audio.
| `tile[1]` | `timer_report_task()` | Small thread responsible only for reporting performance info back `tile[0]` at the end of execution.

> TODO: Diagram showing data flow in xcore firmware.

This tutorial does not explain the implementation of the `xscope_host_data()`,
`wav_io_thread()` and `timer_report_task()` threads. Instead, it focuses on the
behavior and implementation of only `filter_thread()`.

The next page will jump right into the details of [**Stage 0**](stage0.md), the
initial filter implementation. Many aspects of the implementation are shared
between multiple stages of this tutorial, and each will be described as it is
encountered.

[^1]: Explorer board