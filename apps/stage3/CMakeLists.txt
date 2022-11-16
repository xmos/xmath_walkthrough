
set( STAGE_NUMBER   3)

## If XCORE_TARGET hasn't been specified, default to XCORE-AI-EXPLORER
if( NOT DEFINED XCORE_TARGET )
  set( XCORE_TARGET XCORE-AI-EXPLORER )
endif()

set(PARTS A B C)

foreach(PART ${PARTS})
  
  # Application Name
  set( APP_NAME   "stage${STAGE_NUMBER}${PART}" )

  add_executable( ${APP_NAME} )

  target_sources( ${APP_NAME}
      PRIVATE
        ../common/main.xc
        part_${PART}.c
        ../common/filters/box_filter_q2_30.c
  )

  target_link_libraries( ${APP_NAME}
      app_common
      xscope_fileio
  )

  target_compile_options( ${APP_NAME}
      PRIVATE
        -Os -g -fxscope 
        -target=${XCORE_TARGET}
        ${XSCOPE_CONFIG}
  )

  target_compile_definitions( ${APP_NAME}
      PRIVATE
        TEST_WAV_XSCOPE=1
        STAGE_NUMBER=${STAGE_NUMBER}
        PART_${PART}
        STAGE_NAME="${APP_NAME}"
  )

  target_link_options( ${APP_NAME}
      PRIVATE
        ${XSCOPE_CONFIG}
        -target=${XCORE_TARGET}
        -report 
        -fxscope
  )

  install(TARGETS ${APP_NAME} 
          DESTINATION ${WORKSPACE_PATH}/bin )

endforeach()




