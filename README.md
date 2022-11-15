

Start by choosing a directory to serve as the root of your workspace.

## Set Up Workspace

From your workspace root, the following two methods are available to clone the
required repositories into your workspace.

### Using West

If you have the tool West installed (can be installed with 
`python -m pip install west`), use the following:

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
git checkout v2.1.1
git clone https://github.com/astewart-xmos/xscope_fileio
cd ..
```

## Configure CMake Build

To simplify usage, this project requires the CMake build directory to be in the
workspace root.

Two CMake projects will be required. The first is for the device firmware, which
builds using the XMOS toolchain. The second is a host application required by
`xscope_fileio`, which is used to transmit files between the host machine and
xcore device.

### Configure CMake Xcore Build

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


### Configure CMake Host Build

Configuring the host build will depend on your operating system.

#### Windows

The host application can be built in Windows using Visual Studio (VS 2019 is
confirmed to work). Configuring this CMake build should be done from the
Visual Studio Developer Command Prompt. It will also need to be able to find
your XMOS tools install directory to use the xscope endpoint library. 

To do that, in the Developer Command Prompt run:

```
C:\Program Files (x86)\XMOS\XTC\<TOOLS_VERSION>\SetEnv.bat
```

Where `<TOOLS_VERSION>` is your installed XMOS tools version. For example, 
`15.2.0`.

Then, to configure the host build, from your workspace root:

```
cmake -B build.host -S xmath_walkthrough/xscope_fileio/host -A Win32
```

#### Linux

```
TODO
```

## Building

### Host Application

#### Windows

From your workspace root:

```
cmake --build build.host --config Release
cmake --install build.host --config Release
```

The install step will copy the generated executable into the `xscope_fileio`
directory in your workspace root. This is where the `xscope_fileio` Python 
module will look for the host binary when running the app.

#### Linux

```
TODO
```

### Xcore Firmware

The xcore firmware needs to be built and installed to the `bin` directory within
your workspace root. That is where the `run_app.py` Python script will look for
the firmware.

To build the xcore firmware, run the following from your workspace root:

```
cmake --build build
cmake --install build
```

The install command copies several other files and directories into the 
workspace root.


## Running The Application

Because a host application is required as well, rather than using `xrun` to run 
the device firmware directly, these will both be coordinated through the 
`run_app.py` Python script.

First, you will need the adapter ID of your attached xcore device. You can find 
that using `xrun -l`. For example:

```
$ xrun -l

Available XMOS Devices
----------------------

  ID    Name                    Adapter ID      Devices
  --    ----                    ----------      -------
  0     XMOS XTAG-4             891T5J8B        P[0]

```

In this case, the adapter ID is `891T5J8B`.

To run the application on the xcore device, run the following from your 
workspace root:

```
python run_app.py <ADAPTER_ID> <STAGES>
```

Where `<ADAPTER_ID>` is the xcore adapter ID you found with `xrun -l`, and 
`<STAGES>` is a list of the stages to invoke. 

For example, to run stages 7 and 8 on `891T5J8B`, use:

```
python run_app.py 891T5J8B stage7 stage8
```

Each executed stage will output two files, an output wav file, and a JSON file
containing performance info. Both are placed in the `out` directory within your
workspace root.
