
## Start Here

In a new workspace directory, do the following:

```
west init -m https://github.com/astewart-xmos/xmath_walkthrough
west update
mkdir xmath_walkthrough/.build
cd xmath_walkthrough/.build
cmake -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake -G"Unix Makefiles" ..
make install
```