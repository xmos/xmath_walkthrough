
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

add_subdirectory( stage0 )
add_subdirectory( stage1 )
add_subdirectory( stage2 )
add_subdirectory( stage3 )
add_subdirectory( stage4 )
add_subdirectory( stage5 )
add_subdirectory( stage6 )
add_subdirectory( stage7 )
add_subdirectory( stage8 )
add_subdirectory( stage9 )
add_subdirectory( stage10 )
add_subdirectory( stage11 )