
set( STAGE_NUMBER   6)

# Application Name
set( APP_NAME   "stage${STAGE_NUMBER}" )

add_executable( ${APP_NAME} )

target_sources( ${APP_NAME}
    PRIVATE
      ../common/main.xc
      ${APP_NAME}.c
      ../common/filters/filter_coef_q2_30.c
)

target_link_libraries( ${APP_NAME} 
    app_common
    lib_xcore_math
)

target_compile_options( ${APP_NAME} PRIVATE ${APP_SHARED_COMPILE_OPTIONS} )

target_compile_definitions( ${APP_NAME}
    PRIVATE
      STAGE_NUMBER=${STAGE_NUMBER}    
      INPUT_WAV="${INPUT_WAV_PATH}"
      OUTPUT_WAV="${WORKSPACE_PATH}/out/output-stage${STAGE_NUMBER}.wav"
      OUTPUT_JSON="${WORKSPACE_PATH}/out/stage${STAGE_NUMBER}.json"
)

target_link_options( ${APP_NAME} PRIVATE ${APP_SHARED_LINK_OPTIONS} )

install(TARGETS ${APP_NAME} DESTINATION ${WORKSPACE_PATH}/bin )


