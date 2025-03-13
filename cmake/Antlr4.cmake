include(FetchContent)

# Set up ANTLR4 runtime.
FetchContent_Declare(
    antlr4
    GIT_REPOSITORY https://github.com/antlr/antlr4.git
    GIT_TAG 4.13.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(antlr4)

# ANTLR4 has CMakeLists, but we need ANTLR4CPP_USING_ABSEIL which it doesn't support yet.
file(GLOB_RECURSE ANTLR4_SOURCES ${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src/*.cpp)
add_library(antlr4_static STATIC ${ANTLR4_SOURCES})
target_include_directories(antlr4_static
    PUBLIC
    ${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src
)
target_compile_definitions(antlr4_static
    PUBLIC
    ANTLR4CPP_USING_ABSEIL
)
target_link_libraries(antlr4_static
    PUBLIC
    absl::base
    absl::strings
)

# Download ANTLR4 compiler
set(ANTLR4_VERSION "4.13.1")
set(ANTLR4_JAR_NAME "antlr-${ANTLR4_VERSION}-complete.jar")
set(ANTLR4_JAR_URL "https://www.antlr.org/download/${ANTLR4_JAR_NAME}")
set(ANTLR4_JAR_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/${ANTLR4_JAR_NAME}")
if(NOT EXISTS ${ANTLR4_JAR_LOCATION})
    message(STATUS "protovalidate-cc: Downloading ANTLR4 JAR from ${ANTLR4_JAR_URL}")
    file(DOWNLOAD ${ANTLR4_JAR_URL} ${ANTLR4_JAR_LOCATION}
         SHOW_PROGRESS
         STATUS DOWNLOAD_STATUS)
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
        message(FATAL_ERROR "protovalidate-cc: Error downloading ANTLR4 JAR: ${ERROR_MESSAGE}")
    endif()
endif()

# Function to generate ANTLR4 parser
function(Antlr4Generate output_variable grammar_file namespace output_dir)
    get_filename_component(grammar_name ${grammar_file} NAME_WE)

    # Define output files
    set(ANTLR4_GENERATED_FILES
        ${output_dir}/${grammar_name}Lexer.cpp
        ${output_dir}/${grammar_name}Lexer.h
        ${output_dir}/${grammar_name}Parser.cpp
        ${output_dir}/${grammar_name}Parser.h
        ${output_dir}/${grammar_name}BaseVisitor.cpp
        ${output_dir}/${grammar_name}BaseVisitor.h
        ${output_dir}/${grammar_name}Visitor.cpp
        ${output_dir}/${grammar_name}Visitor.h
    )

    # Create output directory
    file(MAKE_DIRECTORY ${output_dir})

    # Add custom command to generate parser
    add_custom_command(
        OUTPUT ${ANTLR4_GENERATED_FILES}
        COMMAND
            java -jar ${ANTLR4_JAR_LOCATION}
            -Dlanguage=Cpp
            -no-listener
            -visitor
            -o ${output_dir}
            -package ${namespace}
            ${grammar_file}
        DEPENDS ${grammar_file}
        COMMENT "Generating ANTLR4 parser from ${grammar_file}"
        VERBATIM
    )

    # Make the generated files available to the caller
    set(${output_variable} ${ANTLR4_GENERATED_FILES} PARENT_SCOPE)
endfunction()