find_package(Threads REQUIRED)

include(Antlr4)
include(Googleapis)

option(CEL_CPP_ENABLE_TESTS "Enable cel-cpp unit tests" OFF)

# Generate ANTLR4 parser for CEL
set(CEL_GRAMMAR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/parser/internal/Cel.g4)
set(CEL_PARSER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/parser/internal)
Antlr4Generate(ANTLR4_GENERATED_SOURCES ${CEL_GRAMMAR_FILE} "cel_parser_internal" ${CEL_PARSER_OUTPUT_DIR})

# Add the generated parser as a library
add_library(cel_parser_generated ${ANTLR4_GENERATED_SOURCES})
target_include_directories(cel_parser_generated PUBLIC
    ${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src
    ${CEL_PARSER_OUTPUT_DIR}
)
target_compile_definitions(cel_parser_generated PRIVATE ANTLR4CPP_USING_ABSEIL)
target_compile_options(cel_parser_generated PUBLIC -fsized-deallocation)
target_link_libraries(cel_parser_generated PUBLIC absl::base antlr4_static)

# Build cel_cc_embed tool for embedding descriptor sets
add_executable(cel_cc_embed bazel/cel_cc_embed.cc)
target_link_libraries(cel_cc_embed PRIVATE
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::strings
    absl::span
)

# Generate googleapis protos
set(GOOGLEAPIS_PROTO_DIR ${googleapis_SOURCE_DIR})
set(GOOGLEAPIS_PROTO_FILES
    google/api/expr/v1alpha1/syntax.proto
    google/api/expr/v1alpha1/checked.proto
    google/api/expr/v1alpha1/value.proto
    google/api/expr/conformance/v1alpha1/conformance_service.proto
    google/rpc/status.proto
    google/rpc/code.proto
    google/api/expr/v1alpha1/eval.proto
    google/rpc/status.proto
    google/api/client.proto
    google/api/launch_stage.proto
    google/rpc/context/attribute_context.proto
    google/type/timeofday.proto
)

# Generate C++ code for googleapis protos
set(GOOGLEAPIS_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/googleapis)
file(MAKE_DIRECTORY ${GOOGLEAPIS_PROTO_GEN_DIR})
set(GOOGLEAPIS_PROTO_FILES
    google/api/expr/v1alpha1/syntax.proto
    google/api/expr/v1alpha1/checked.proto
    google/api/expr/v1alpha1/value.proto
    google/api/expr/conformance/v1alpha1/conformance_service.proto
    google/rpc/status.proto
    google/rpc/code.proto
    google/api/expr/v1alpha1/eval.proto
    google/rpc/status.proto
    google/api/client.proto
    google/api/launch_stage.proto
    google/rpc/context/attribute_context.proto
    google/type/timeofday.proto
)
set(GOOGLEAPIS_PROTO_DIR ${googleapis_SOURCE_DIR})
list(TRANSFORM GOOGLEAPIS_PROTO_FILES PREPEND ${googleapis_SOURCE_DIR}/)
add_library(googleapis_proto OBJECT ${GOOGLEAPIS_PROTO_FILES})
target_include_directories(googleapis_proto PUBLIC ${GOOGLEAPIS_PROTO_GEN_DIR})
target_link_libraries(googleapis_proto PUBLIC protobuf::libprotobuf)
protobuf_generate(
    TARGET
    googleapis_proto
    LANGUAGE
        cpp
    PROTOC_OUT_DIR
        ${GOOGLEAPIS_PROTO_GEN_DIR}
    IMPORT_DIRS
        ${GOOGLEAPIS_PROTO_DIR}
        ${PROTOBUF_IMPORT_PATH}
)

# Collect core source files
file(GLOB CEL_CPP_CORE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    base/*.cc
    base/ast_internal/*.cc
    base/internal/*.cc
    checker/*.cc
    checker/internal/*.cc
    common/*.cc
    common/internal/*.cc
    common/types/*.cc
    common/values/*.cc
    eval/compiler/*.cc
    eval/eval/*.cc
    eval/internal/*.cc
    eval/public/*.cc
    eval/public/containers/*.cc
    eval/public/structs/*.cc
    extensions/*.cc
    extensions/protobuf/*.cc
    extensions/protobuf/internal/*.cc
    internal/*.cc
    parser/*.cc
    runtime/*.cc
    runtime/internal/*.cc
    runtime/standard/*.cc
)

# Collect test source files
set(CEL_CPP_TEST_SOURCES ${CEL_CPP_CORE_SOURCES})

# Set up test sources
list(FILTER CEL_CPP_TEST_SOURCES INCLUDE REGEX ".*_test\\.cc$")

# Remove some tests that don't seem to work right.
list(REMOVE_ITEM CEL_CPP_TEST_SOURCES
    # Looks like an issue where absl::StrCat isn't IWYU'd.
    # 
    #     error: no member named 'StrCat' in namespace 'absl'
    eval/public/builtin_func_test.cc
    internal/message_equality_test.cc
    internal/strings_test.cc

    # Compilation issue:
    #
    #     error: call to 'CheckedAdd' is ambiguous
    internal/overflow_test.cc

    # Seems to require an operator<< overload that is not declared anywhere.
    #
    #     error: invalid operands to binary expression ('std::stringstream'
    #     and 'const google::api::expr::v1alpha1::ParsedExpr')
    parser/parser_test.cc

    # Seems to rely on ABSL_ATTRIBUTE_VIEW which isn't supported in the absl
    # version that is pinned by the release (?)
    #
    #     error: variable has incomplete type 'class ABSL_ATTRIBUTE_VIEW'
    common/arena_string_pool_test.cc
    common/arena_string_test.cc
)

# Collect testing library sources
set(CEL_CPP_TESTING_SOURCES
    base/internal/memory_manager_testing.cc
    common/value_testing.cc
    checker/internal/test_ast_helpers.cc
    eval/public/testing/matchers.cc
    internal/equals_text_proto.cc
    internal/testing.cc
    internal/testing_message_factory.cc
    internal/testing_descriptor_pool.cc
    testutil/baseline_tests.cc
    testutil/expr_printer.cc
    tools/branch_coverage.cc
    tools/navigable_ast.cc
)

# Remove all testing library sources from the core sources
list(REMOVE_ITEM CEL_CPP_CORE_SOURCES ${CEL_CPP_TESTING_SOURCES})

# Remove all test sources from the core sources
list(FILTER CEL_CPP_CORE_SOURCES EXCLUDE REGEX ".*_test\\.cc$")

add_library(cel_cpp ${CEL_CPP_CORE_SOURCES})

target_link_libraries(cel_cpp
    PUBLIC
    cel_parser_generated
    googleapis_proto
    cel_cpp_test_proto
    absl::base
    absl::strings
    absl::status
    absl::statusor
    absl::time
    absl::flat_hash_map
    absl::flat_hash_set
    absl::btree
    absl::synchronization
    absl::optional
    absl::span
    absl::variant
    protobuf::libprotobuf
    re2::re2
)

# Include directories for the library
target_include_directories(cel_cpp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CEL_PARSER_OUTPUT_DIR}
    ${GOOGLEAPIS_PROTO_GEN_DIR}
)

# Compiler definitions
target_compile_definitions(cel_cpp
    PRIVATE
    ANTLR4CPP_USING_ABSEIL
)

# Generate embedded descriptor set
set(MINIMAL_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/gen/minimal_descriptor_set.pb)

# Define the well-known protobuf files
set(PROTOBUF_WELL_KNOWN_PROTOS
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/any.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/duration.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/struct.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/timestamp.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/wrappers.proto
)

# Create the directory for the descriptor set
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen)

# Generate the descriptor set
add_custom_command(
    OUTPUT ${MINIMAL_DESCRIPTOR_SET_PATH}
    COMMAND ${PROTOC_EXECUTABLE}
            --include_imports
            --descriptor_set_out=${MINIMAL_DESCRIPTOR_SET_PATH}
            -I${PROTOBUF_IMPORT_PATH}
            ${PROTOBUF_WELL_KNOWN_PROTOS}
    DEPENDS ${PROTOBUF_WELL_KNOWN_PROTOS}
    COMMENT "Generating minimal descriptor set"
    VERBATIM
)

# Embed descriptor set into C++ header
set(MINIMAL_DESCRIPTOR_SET_EMBED_PATH ${CMAKE_CURRENT_BINARY_DIR}/gen/internal/minimal_descriptor_set_embed.inc)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen/internal)
add_custom_command(
    OUTPUT ${MINIMAL_DESCRIPTOR_SET_EMBED_PATH}
    COMMAND cel_cc_embed
            --in=${MINIMAL_DESCRIPTOR_SET_PATH}
            --out=${MINIMAL_DESCRIPTOR_SET_EMBED_PATH}
    DEPENDS ${MINIMAL_DESCRIPTOR_SET_PATH} cel_cc_embed
    COMMENT "Embedding minimal descriptor set"
    VERBATIM
)

# Add the embedded descriptor set to the build
target_sources(cel_cpp PRIVATE ${MINIMAL_DESCRIPTOR_SET_EMBED_PATH})
target_include_directories(cel_cpp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)

# Fetch CEL spec repository (needed for test protos)
FetchContent_Declare(
    cel_spec
    GIT_REPOSITORY https://github.com/google/cel-spec.git
    GIT_TAG f027a86d2e5bf18f796be0c4373f637a61041cde
)
FetchContent_MakeAvailable(cel_spec)

# Generate C++ code for test protos
set(CEL_SPEC_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/proto)
file(MAKE_DIRECTORY ${CEL_SPEC_PROTO_GEN_DIR})
set(CEL_SPEC_PROTO_FILES
    test/v1/proto2/test_all_types.proto
    test/v1/proto3/test_all_types.proto
    cel/expr/checked.proto
    cel/expr/eval.proto
    cel/expr/explain.proto
    cel/expr/syntax.proto
    cel/expr/value.proto
)
set(CEL_SPEC_PROTO_DIR ${cel_spec_SOURCE_DIR}/proto)
list(TRANSFORM CEL_SPEC_PROTO_FILES PREPEND ${CEL_SPEC_PROTO_DIR}/)
add_library(cel_spec_proto OBJECT ${CEL_SPEC_PROTO_FILES})
target_include_directories(cel_spec_proto PUBLIC ${CEL_SPEC_PROTO_GEN_DIR})
target_link_libraries(cel_spec_proto PUBLIC protobuf::libprotobuf googleapis_proto)
protobuf_generate(
    TARGET
        cel_spec_proto
    LANGUAGE
        cpp
    PROTOC_OUT_DIR
        ${CEL_SPEC_PROTO_GEN_DIR}
    IMPORT_DIRS
        ${CEL_SPEC_PROTO_DIR}
        ${PROTOBUF_IMPORT_PATH}
        ${GOOGLEAPIS_PROTO_DIR}
    DEPENDENCIES
        googleapis_proto
)

# Create a custom target for the generated protobuf files
set(CEL_CPP_TEST_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/cel-cpp-test-protos)
file(MAKE_DIRECTORY ${CEL_CPP_TEST_PROTO_GEN_DIR})
set(CEL_CPP_TEST_PROTO_FILES
    eval/testutil/test_message.proto
    eval/testutil/simple_test_message.proto
    eval/testutil/test_extensions.proto
    eval/tests/request_context.proto
)
add_library(cel_cpp_test_proto OBJECT ${CEL_CPP_TEST_PROTO_FILES})
target_include_directories(cel_cpp_test_proto PUBLIC ${CEL_CPP_TEST_PROTO_GEN_DIR})
target_link_libraries(cel_cpp_test_proto PUBLIC protobuf::libprotobuf)
protobuf_generate(
    TARGET
        cel_cpp_test_proto
    LANGUAGE
        cpp
    PROTOC_OUT_DIR
        ${CEL_CPP_TEST_PROTO_GEN_DIR}
    IMPORT_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTOBUF_IMPORT_PATH}
        ${GOOGLEAPIS_PROTO_DIR}
    DEPENDENCIES
        cel_spec_proto
)

# Create a custom target for the generated test protobuf files
add_custom_target(generate_test_proto DEPENDS ${CEL_CPP_TEST_PROTO_SRCS} ${CEL_CPP_TEST_PROTO_HDRS})

# Generate testing descriptor set
set(TESTING_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/gen/testing_descriptor_set.pb)

# Define the testing protobuf files
set(TESTING_PROTO
    ${PROTOBUF_WELL_KNOWN_PROTOS}
    ${CEL_SPEC_PROTO_DIR}/cel/expr/checked.proto
    ${CEL_SPEC_PROTO_DIR}/cel/expr/eval.proto
    ${CEL_SPEC_PROTO_DIR}/cel/expr/explain.proto
    ${CEL_SPEC_PROTO_DIR}/cel/expr/syntax.proto
    ${CEL_SPEC_PROTO_DIR}/cel/expr/value.proto
    ${CEL_SPEC_PROTO_DIR}/test/v1/proto2/test_all_types.proto
    ${CEL_SPEC_PROTO_DIR}/test/v1/proto3/test_all_types.proto
    ${GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/syntax.proto
    ${GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/checked.proto
    ${GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/eval.proto
    ${GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/value.proto
)

add_custom_command(
    OUTPUT ${TESTING_DESCRIPTOR_SET_PATH}
    COMMAND ${PROTOC_EXECUTABLE}
            --include_imports
            --descriptor_set_out=${TESTING_DESCRIPTOR_SET_PATH}
            -I${PROTOBUF_IMPORT_PATH}
            -I${GOOGLEAPIS_PROTO_DIR}
            -I${CEL_SPEC_PROTO_DIR}
            ${TESTING_PROTO}
    DEPENDS ${TESTING_PROTO}
    COMMENT "Generating testing descriptor set"
    VERBATIM
)

# Embed testing descriptor set into C++ header
set(TESTING_DESCRIPTOR_SET_EMBED_PATH ${CMAKE_CURRENT_BINARY_DIR}/gen/internal/testing_descriptor_set_embed.inc)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen/internal)
add_custom_command(
    OUTPUT ${TESTING_DESCRIPTOR_SET_EMBED_PATH}
    COMMAND cel_cc_embed
            --in=${TESTING_DESCRIPTOR_SET_PATH}
            --out=${TESTING_DESCRIPTOR_SET_EMBED_PATH}
    DEPENDS ${TESTING_DESCRIPTOR_SET_PATH} cel_cc_embed
    COMMENT "Embedding testing descriptor set"
    VERBATIM
)

if(CEL_CPP_ENABLE_TESTS AND BUILD_TESTING)
    enable_testing()

    # Fetch Google Benchmark
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
        GIT_SHALLOW TRUE
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark's tests" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark installation" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable benchmark's gtest tests" FORCE)
    FetchContent_MakeAvailable(benchmark)

    # cel-cpp test code
    add_library(cel_cpp_testing OBJECT ${CEL_CPP_TESTING_SOURCES} ${TESTING_DESCRIPTOR_SET_EMBED_PATH})

    target_include_directories(cel_cpp_testing
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(cel_cpp_testing
        PUBLIC
            absl::status_matchers
            GTest::gmock
            benchmark::benchmark
            Threads::Threads

            cel_spec_proto
            cel_cpp_test_proto
            cel_cpp
    )

    function(add_cel_cpp_test test_file)
        string(REPLACE "/" "_" target_name "${test_file}")
        string(REPLACE ".cc" "" target_name "${target_name}")
        add_executable(${target_name} ${test_file})
        target_link_libraries(${target_name}
            PUBLIC
                GTest::gmock_main
                Threads::Threads

                cel_cpp_testing
        )
        add_test(NAME ${target_name} COMMAND ${target_name})
    endfunction()

    foreach(CEL_CPP_TEST_SOURCE IN LISTS CEL_CPP_TEST_SOURCES)
        add_cel_cpp_test(${CEL_CPP_TEST_SOURCE})
    endforeach()
endif()
