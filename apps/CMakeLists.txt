

# Get absolute path to the config.xscope file used by each application.
get_filename_component(XSCOPE_CONFIG "common/config.xscope" ABSOLUTE 
  BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}) 


add_subdirectory( common )

add_subdirectory( stage1 )
add_subdirectory( stage2 )
add_subdirectory( stage3 )
add_subdirectory( stage4 )
add_subdirectory( stage5 )
add_subdirectory( stage6 )
add_subdirectory( stage7 )