
set( LIB_NAME  app_common )

add_library( ${LIB_NAME} STATIC )

target_sources( ${LIB_NAME}
    PUBLIC
      file_utils/fileio.c
      file_utils/wav_utils.c
      timing/timing.c
      wav_io/wav_io.c
)

target_include_directories( ${LIB_NAME} 
    PUBLIC 
      file_utils
      timing
      wav_io
      misc
)

target_compile_options( ${LIB_NAME} 
    PUBLIC
      -g -Os 
      -fxscope
)

# target_compile_definitions( ${LIB_NAME}
#     PRIVATE
# )

target_link_libraries( ${LIB_NAME}
    lib_xcore_math
)