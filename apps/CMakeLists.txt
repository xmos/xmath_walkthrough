

# Get absolute path to the config.xscope file used by each application.
get_filename_component(XSCOPE_CONFIG "common/config.xscope" ABSOLUTE 
  BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}) 


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
add_subdirectory( stage12 )

install(TARGETS 
    stage0 stage1 stage2 stage3 stage4 stage5 stage6
    stage7 stage8 stage9 stage10 stage11 stage12
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin )