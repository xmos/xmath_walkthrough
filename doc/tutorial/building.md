
# Project Setup

The first thing to do is to set up a workspace and configure the project for
building. The workspace is any empty directory which will serve as the base
directory for all activity in this tutorial. In this page and throughout this
tutorial your workspace directory will be referred to as the `workspace root`.

## Set Up Workspace

Once you have chosen a directory to serve as your workspace root, the next step
is to clone the git repositories required for this tutorial. Three methods are
available to do this.

### Zip File

If you received this tutorial as a `.zip` file, you can simply extract the
contents of that zip file into your workspace root.

### Using West

If you have the tool West installed[^1], use the following:

```
west init -m https://github.com/astewart-xmos/xmath_walkthrough
west update
```

### Without West

Otherwise, you can use git to clone the repositories directly:

```
git clone https://github.com/astewart-xmos/xmath_walkthrough
git clone https://github.com/xmos/xmos_cmake_toolchain
cd xmath_walkthrough
git clone -b v2.1.1 https://github.com/xmos/lib_xcore_math
cd ..
```

## Configure CMake Build

To simplify usage, this project requires the CMake build directory to be in the
workspace root. The CMake project will build (using the XMOS toolchain) all
device firmware used in this tutorial.

From your workspace root:

```
cmake -B build -S xmath_walkthrough -DCMAKE_TOOLCHAIN_FILE=xmos_cmake_toolchain/xs3a.cmake <PLATFORM_FLAG>
```

Where `<PLATFORM_FLAG>` depends on your operating system and available tools.

| OS      | Build Tool  | `<PLATFORM_FLAG>`     |
| ------- | ----------- | --------------------- |
| Windows | GNU Make    | `-G"Unix Makefiles"`  |
| Windows | NMake       | `-G"NMake Makefiles"` |
| Linux   |             | TBD                   |


## Building

The xcore firmware will be built and installed to the `bin/` directory within
your workspace root. To build and install the xcore firmware, run the following
from your workspace root:

```
cmake --build build
cmake --install build
```

The install command copies the firmware and several other files and directories
into the workspace root.

## Running The Application

Each stage of this tutorial has an associated firmware, all of which should be
in the `bin/` directory within your workspace root.

When running the firmware, the workspace root must be your working directory,
because the input and output file paths will be relative to the directory from
which `xrun` is called.

Before executing the firmware, ensure that the xcore tools are on your path and
that your xcore.ai device has been discovered. The easiest way to do this is to
run `xrun -l`. For example:

```
$ xrun -l

Available XMOS Devices
----------------------

  ID    Name                    Adapter ID      Devices
  --    ----                    ----------      -------
  0     XMOS XTAG-4             891T5J8B        P[0]

```

If the command is successful and a device is listed, you should be all set.

To run the application firmware, do following from your workspace root:

```
xrun --xscope bin/<STAGE>.xe
```

Where `<STAGE>` is the tutorial stage whose firmware is to be run. For example,
to run Part 2C's firmware, the following command is used:

```
xrun --xscope bin/part2C.xe
```

The executed stage should output two files, an output wav file, and a JSON file
containing the performance info. Both are placed in the `out\` directory within
your workspace root.

[^1]: West can be installed using: `python -m pip install west`