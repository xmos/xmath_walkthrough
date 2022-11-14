cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0057 NEW)

project(xmath_primer)

enable_language( C CXX ASM )

string(REPLACE "-MD" "-MMD" CMAKE_DEPFILE_FLAGS_C ${CMAKE_DEPFILE_FLAGS_C})

add_subdirectory(lib_xcore_math)
add_subdirectory(xscope_fileio)
add_subdirectory(apps)