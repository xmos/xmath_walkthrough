
set( LIB_NAME  app_common )

add_library( ${LIB_NAME} STATIC )

target_sources( ${LIB_NAME}
    PUBLIC
      file_utils/fileio.c
      file_utils/wav_utils.c
      timing/timing.c
      framing/framing.c
      wav_proc/wav_proc.c
)

target_include_directories( ${LIB_NAME} 
    PUBLIC 
      file_utils
      timing
      wav_proc
      framing
      misc
)

target_compile_options( ${LIB_NAME} 
    PUBLIC
      -g -Os 
      -fxscope
)

target_compile_definitions( ${LIB_NAME}
    PRIVATE
      TEST_WAV_XSCOPE=1
)

target_link_libraries( ${LIB_NAME}
    xscope_fileio
    lib_xcore_math
)