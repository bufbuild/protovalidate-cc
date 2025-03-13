find_package(Threads REQUIRED)

include(Antlr4)
include(Googleapis)

option(CEL_CPP_ENABLE_TESTS "Enable cel-cpp unit tests" OFF)

# Generate ANTLR4 parser for CEL
set(CEL_CPP_GRAMMAR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/parser/internal/Cel.g4)
set(CEL_CPP_PARSER_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/cel_cpp_parser)
set(CEL_CPP_PARSER_OUTPUT_DIR ${CEL_CPP_PARSER_GEN_DIR}/parser/internal)
Antlr4Generate(CEL_CPP_ANTLR4_GENERATED_SOURCES ${CEL_CPP_GRAMMAR_FILE} "cel_parser_internal" ${CEL_CPP_PARSER_OUTPUT_DIR})
add_library(cel_cpp_parser ${CEL_CPP_ANTLR4_GENERATED_SOURCES})
target_include_directories(cel_cpp_parser PUBLIC
    ${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src
    ${CEL_CPP_PARSER_OUTPUT_DIR}
)
target_compile_definitions(cel_cpp_parser PRIVATE ANTLR4CPP_USING_ABSEIL)
target_compile_options(cel_cpp_parser PUBLIC -fsized-deallocation)
target_link_libraries(cel_cpp_parser PUBLIC absl::base antlr4_static)
target_include_directories(cel_cpp_parser PUBLIC ${CEL_CPP_PARSER_GEN_DIR})
add_library(cel_cpp::parser ALIAS cel_cpp_parser)

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
add_executable(cel_cpp::embed ALIAS cel_cc_embed)

# Generate code for googleapis protos
set(CEL_CPP_GOOGLEAPIS_PROTO_DIR ${googleapis_SOURCE_DIR})
set(CEL_CPP_GOOGLEAPIS_PROTO_FILES
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
set(CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/googleapis)
file(MAKE_DIRECTORY ${CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR})
set(CEL_CPP_GOOGLEAPIS_PROTO_DIR ${googleapis_SOURCE_DIR})
list(TRANSFORM CEL_CPP_GOOGLEAPIS_PROTO_FILES PREPEND ${googleapis_SOURCE_DIR}/)
add_library(cel_googleapis_proto OBJECT ${CEL_CPP_GOOGLEAPIS_PROTO_FILES})
target_include_directories(cel_googleapis_proto PUBLIC ${CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR})
target_link_libraries(cel_googleapis_proto PUBLIC protobuf::libprotobuf)
protobuf_generate(
    TARGET cel_googleapis_proto
    LANGUAGE cpp
    PROTOC_OUT_DIR ${CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR}
    IMPORT_DIRS ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}
                ${PROTOBUF_IMPORT_PATH}
)
add_library(cel_cpp::googleapis_proto ALIAS cel_googleapis_proto)

# Generate embedded descriptor set
set(CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/minimal_descriptor_set.pb)
set(CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTOS
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/any.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/duration.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/struct.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/timestamp.proto
    ${PROTOBUF_IMPORT_PATH}/google/protobuf/wrappers.proto
)
add_custom_command(
    OUTPUT ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH}
    COMMAND ${PROTOC_EXECUTABLE}
            --include_imports
            --descriptor_set_out=${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH}
            -I${PROTOBUF_IMPORT_PATH}
            ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTOS}
    DEPENDS ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTOS}
    COMMENT "Generating minimal descriptor set"
    VERBATIM
)
set(CEL_CPP_MINIMAL_DESCRIPTOR_SET_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/minimal_descriptor_set)
file(MAKE_DIRECTORY ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_GEN_DIR}/internal)
set(CEL_CPP_MINIMAL_DESCRIPTOR_SET_EMBED_PATH ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_GEN_DIR}/internal/minimal_descriptor_set_embed.inc)
add_custom_command(
    OUTPUT ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_EMBED_PATH}
    COMMAND $<TARGET_FILE:cel_cpp::embed>
            --in=${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH}
            --out=${CEL_CPP_MINIMAL_DESCRIPTOR_SET_EMBED_PATH}
    DEPENDS ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH}
            cel_cpp::embed
    COMMENT "Embedding minimal descriptor set"
    VERBATIM
)
add_custom_target(cel_cpp_minimal_descriptor_set_gen DEPENDS ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_EMBED_PATH})
add_library(cel_cpp_minimal_descriptor_set INTERFACE)
add_dependencies(cel_cpp_minimal_descriptor_set cel_cpp_minimal_descriptor_set_gen)
target_include_directories(cel_cpp_minimal_descriptor_set INTERFACE ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_GEN_DIR})
add_library(cel_cpp::minimal_descriptor_set ALIAS cel_cpp_minimal_descriptor_set)

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

# Collect test source files
set(CEL_CPP_TEST_SOURCES ${CEL_CPP_CORE_SOURCES})
list(FILTER CEL_CPP_TEST_SOURCES INCLUDE REGEX ".*_test\\.cc$")
list(FILTER CEL_CPP_CORE_SOURCES EXCLUDE REGEX ".*_test\\.cc$")

# Remove some tests that don't seem to work right.
list(REMOVE_ITEM CEL_CPP_TEST_SOURCES
    # Build Failures

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

    # Test Failures
    # These seem to be legitimately broken in v0.10.0.
    common/values/message_value_test.cc
    common/values/mutable_list_value_test.cc
    common/values/mutable_map_value_test.cc
    common/values/parsed_json_list_value_test.cc
    common/values/parsed_json_map_value_test.cc
    common/values/parsed_json_value_test.cc
    common/values/parsed_map_field_value_test.cc
    common/values/parsed_message_value_test.cc
    common/values/parsed_repeated_field_value_test.cc
    # This passes on some platforms and not others.
    common/native_type_test.cc
    # Requires us to build simple_test_message_proto-descriptor-set.proto.bin
    eval/compiler/flat_expr_builder_test.cc
)

add_library(cel_cpp ${CEL_CPP_CORE_SOURCES} ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_EMBED_PATH})
target_link_libraries(cel_cpp PUBLIC
    cel_cpp::parser
    cel_cpp::minimal_descriptor_set
    cel_cpp::googleapis_proto
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
target_include_directories(cel_cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cel_cpp PRIVATE ANTLR4CPP_USING_ABSEIL)
add_library(cel_cpp::cel_cpp ALIAS cel_cpp)

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

    # Fetch CEL spec repository (needed for test protos)
    FetchContent_Declare(
        cel_spec
        GIT_REPOSITORY https://github.com/google/cel-spec.git
        GIT_TAG f027a86d2e5bf18f796be0c4373f637a61041cde
    )
    FetchContent_MakeAvailable(cel_spec)

    # Generate C++ code for test protos
    set(CEL_SPEC_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/cel_spec_proto)
    file(MAKE_DIRECTORY ${CEL_SPEC_PROTO_GEN_DIR}/proto)
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
    add_library(cel_cpp_spec_proto OBJECT ${CEL_SPEC_PROTO_FILES})
    target_include_directories(cel_cpp_spec_proto
        PUBLIC ${CEL_SPEC_PROTO_GEN_DIR}
        PRIVATE ${CEL_SPEC_PROTO_GEN_DIR}/proto
    )
    target_link_libraries(cel_cpp_spec_proto PUBLIC protobuf::libprotobuf cel_cpp::googleapis_proto)
    protobuf_generate(
        TARGET cel_cpp_spec_proto
        LANGUAGE cpp
        PROTOC_OUT_DIR ${CEL_SPEC_PROTO_GEN_DIR}/proto
        IMPORT_DIRS ${CEL_SPEC_PROTO_DIR}
                    ${PROTOBUF_IMPORT_PATH}
                    ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}
        DEPENDENCIES cel_cpp::googleapis_proto
    )
    add_library(cel_cpp::spec_proto ALIAS cel_cpp_spec_proto)

    # Create a custom target for the generated protobuf files
    set(CEL_CPP_TEST_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/cel_cpp_test_proto)
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
        TARGET cel_cpp_test_proto
        LANGUAGE cpp
        PROTOC_OUT_DIR ${CEL_CPP_TEST_PROTO_GEN_DIR}
        IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    ${PROTOBUF_IMPORT_PATH}
                    ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}
        DEPENDENCIES cel_cpp::spec_proto
    )
    add_library(cel_cpp::test_proto ALIAS cel_cpp_test_proto)

    # Generate testing descriptor set
    set(CEL_CPP_TESTING_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/testing_descriptor_set.pb)
    set(TESTING_PROTO
        ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTOS}
        ${CEL_SPEC_PROTO_DIR}/cel/expr/checked.proto
        ${CEL_SPEC_PROTO_DIR}/cel/expr/eval.proto
        ${CEL_SPEC_PROTO_DIR}/cel/expr/explain.proto
        ${CEL_SPEC_PROTO_DIR}/cel/expr/syntax.proto
        ${CEL_SPEC_PROTO_DIR}/cel/expr/value.proto
        ${CEL_SPEC_PROTO_DIR}/test/v1/proto2/test_all_types.proto
        ${CEL_SPEC_PROTO_DIR}/test/v1/proto3/test_all_types.proto
        ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/syntax.proto
        ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/checked.proto
        ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/eval.proto
        ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}/google/api/expr/v1alpha1/value.proto
    )
    add_custom_command(
        OUTPUT ${CEL_CPP_TESTING_DESCRIPTOR_SET_PATH}
        COMMAND ${PROTOC_EXECUTABLE}
                --include_imports
                --descriptor_set_out=${CEL_CPP_TESTING_DESCRIPTOR_SET_PATH}
                -I${PROTOBUF_IMPORT_PATH}
                -I${CEL_CPP_GOOGLEAPIS_PROTO_DIR}
                -I${CEL_SPEC_PROTO_DIR}
                ${TESTING_PROTO}
        DEPENDS ${TESTING_PROTO}
        COMMENT "Generating testing descriptor set"
        VERBATIM
    )
    set(CEL_CPP_TESTING_DESCRIPTOR_SET_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/testing_descriptor_set)
    file(MAKE_DIRECTORY ${CEL_CPP_TESTING_DESCRIPTOR_SET_GEN_DIR}/internal)
    set(CEL_CPP_TESTING_DESCRIPTOR_SET_EMBED_PATH ${CEL_CPP_TESTING_DESCRIPTOR_SET_GEN_DIR}/internal/testing_descriptor_set_embed.inc)
    add_custom_command(
        OUTPUT ${CEL_CPP_TESTING_DESCRIPTOR_SET_EMBED_PATH}
        COMMAND $<TARGET_FILE:cel_cpp::embed>
                --in=${CEL_CPP_TESTING_DESCRIPTOR_SET_PATH}
                --out=${CEL_CPP_TESTING_DESCRIPTOR_SET_EMBED_PATH}
        DEPENDS ${CEL_CPP_TESTING_DESCRIPTOR_SET_PATH}
                cel_cpp::embed
        COMMENT "Embedding testing descriptor set"
        VERBATIM
    )
    add_custom_target(cel_cpp_testing_descriptor_set_gen DEPENDS ${CEL_CPP_TESTING_DESCRIPTOR_SET_EMBED_PATH})
    add_library(cel_cpp_testing_descriptor_set INTERFACE)
    add_dependencies(cel_cpp_testing_descriptor_set cel_cpp_testing_descriptor_set_gen)
    target_include_directories(cel_cpp_testing_descriptor_set INTERFACE ${CEL_CPP_TESTING_DESCRIPTOR_SET_GEN_DIR})
    add_library(cel_cpp::testing_descriptor_set ALIAS cel_cpp_testing_descriptor_set)

    # cel-cpp test code
    add_library(cel_cpp_testing OBJECT ${CEL_CPP_TESTING_SOURCES} ${CEL_CPP_TESTING_DESCRIPTOR_SET_EMBED_PATH})
    target_include_directories(cel_cpp_testing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(cel_cpp_testing PUBLIC
        absl::status_matchers
        GTest::gmock
        benchmark::benchmark
        Threads::Threads
        cel_cpp::cel_cpp
        cel_cpp::testing_descriptor_set
    )
    add_library(cel_cpp::testing ALIAS cel_cpp_testing)

    function(add_cel_cpp_test test_file)
        string(REPLACE "/" "_" target_name "${test_file}")
        string(REPLACE ".cc" "" target_name "${target_name}")
        add_executable(${target_name} ${test_file})
        target_link_libraries(${target_name} PUBLIC
            GTest::gmock_main
            Threads::Threads
            cel_cpp::testing
            cel_cpp::spec_proto
            cel_cpp::test_proto
        )
        add_test(NAME ${target_name} COMMAND ${target_name})
    endfunction()

    foreach(CEL_CPP_TEST_SOURCE IN LISTS CEL_CPP_TEST_SOURCES)
        add_cel_cpp_test(${CEL_CPP_TEST_SOURCE})
    endforeach()
endif()
