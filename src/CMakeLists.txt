
## If XCORE_TARGET hasn't been specified, default to XCORE-AI-EXPLORER
if( NOT DEFINED XCORE_TARGET )
  set( XCORE_TARGET XCORE-AI-EXPLORER )
endif()

set(APP_SHARED_COMPILE_OPTIONS 
    -Os -g -fxscope -mcmodel=large -target=${XCORE_TARGET} )

set(APP_SHARED_LINK_OPTIONS
    -target=${XCORE_TARGET} -report -fxscope )

set( INPUT_WAV_PATH "${WORKSPACE_PATH}/xmath_walkthrough/wav/input.wav" )


add_subdirectory( common )

add_subdirectory( part1A )
add_subdirectory( part1B )
add_subdirectory( part1C )
add_subdirectory( part2A )
add_subdirectory( part2B )
add_subdirectory( part2C )
add_subdirectory( part3A )
add_subdirectory( part3B )
add_subdirectory( part3C )
add_subdirectory( part4A )
add_subdirectory( part4B )
add_subdirectory( part4C )

add_subdirectory( appendixA )