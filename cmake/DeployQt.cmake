if(NOT DEPLOY_TOOL OR NOT APP_BINARY)
    message(FATAL_ERROR "DeployQt.cmake needs DEPLOY_TOOL and APP_BINARY")
endif()

execute_process(
    COMMAND "${DEPLOY_TOOL}" --no-translations --no-compiler-runtime --no-system-d3d-compiler --no-opengl-sw "${APP_BINARY}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE  out
)

if(NOT rc EQUAL 0)
    message(STATUS "windeployqt returned ${rc}; continuing. Output:\n${out}")
endif()
