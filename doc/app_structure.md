
[Prev](building.md) | [Home](intro.md) | [Next](stage0.md)

Each of the firwares built in this tutorial are variations of the same basic
application. The application runs on an xcore Explorer board[^1], reads a `wav`
file in from the host machine, processes the `wav` file audio through a digital
FIR filter, and writes out the processed audio to a new `wav` file on the host
machine.

> TODO: Add diagram showing data flow between host and xcore.

## Host Application

File access between the xcore device and the host machine is achieved using the
`xscope_fileio` library[^2], which builds on top of the XMOS xscope probe system
for device-host communication. `xscope_fileio` will require a host application
to be built and run on the host machine to interface with the xcore device.

The design and usage of `xscope_fileio` and the host-side application is beyond
the scope of this tutorial.

## Xcore Application

The firmware run on the xcore device is a relatively simple application which,
in broad strokes, does the following:

1. Waits for host application to establish an xscope connection.
2. Remotely opens two `wav` files on the host, one for input and one for output.
3. Reads a frame of audio data from the input `wav` file.
4. Passes the frame of audio data to another thread for processing.
5. Waits for a frame of processed audio from the other thread.
6. Write the processed audio to the output `wav` file.
7. Loops back to step 3 until there is no more input audio.

The application is a two-tile application with (in most stages) two threads
running on each tile.

| Tile      | Entry Point           | Description |
|-----------|-----------------------|-------------|
| `tile[0]` | `xscope_host_data()`  | Part of `xscope_fileio`; Responsible for handling file communication between the xcore and host.
| `tile[0]` | `wav_io_thread()`     | Responsible for decoding and encoding the input and output `wav` files and framing the audio data.
| `tile[1]` | `filter_thread()`     | Responsible for producing output audio from input audio.
| `tile[1]` | `timer_report_task()` | Responsible for reporting performance info back `tile[0]`.


> TODO: Diagram showing data flow in xcore firmware.

This tutorial does not explain the implementation of the `xscope_host_data()`,
`wav_io_thread()` and `timer_report_task()` threads. Instead, it focuses on the
behavior and implementation of only `filter_thread()`.

The next page will jump right into the details of [**Stage 0**](stage0.md), the
initial filter implementation. Many aspects of the implementation are shared
between multiple stages of this tutorial, and each will be described as it is
encountered.

[^1]: Explorer board
[^2]: `xscope_fileio`