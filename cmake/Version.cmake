find_package(Git)

if(GIT_EXECUTABLE)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE PROTOVALIDATE_CC_VERSION
        RESULT_VARIABLE ERROR_CODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(PROTOVALIDATE_CC_VERSION STREQUAL "")
    set(PROTOVALIDATE_CC_VERSION "v0.0-unknown")
    message(WARNING "protovalidate-cc: Failed to fetch version from Git. Using placeholder version ${PROTOVALIDATE_CC_VERSION}.")
endif()
