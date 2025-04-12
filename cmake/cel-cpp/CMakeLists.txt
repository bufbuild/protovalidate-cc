find_package(Threads REQUIRED)

option(CEL_CPP_ENABLE_TESTS "Enable cel-cpp unit tests" OFF)

# Generate ANTLR4 parser for CEL
set(CEL_CPP_GRAMMAR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/parser/internal/Cel.g4)
set(CEL_CPP_PARSER_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/cel_cpp_parser)
set(CEL_CPP_PARSER_OUTPUT_DIR ${CEL_CPP_PARSER_GEN_DIR}/parser/internal)
Antlr4Generate(CEL_CPP_ANTLR4_GENERATED_SOURCES ${CEL_CPP_GRAMMAR_FILE} "cel_parser_internal" ${CEL_CPP_PARSER_OUTPUT_DIR})
add_library(cel_cpp_parser OBJECT ${CEL_CPP_ANTLR4_GENERATED_SOURCES})
target_include_directories(cel_cpp_parser PUBLIC
    $<BUILD_INTERFACE:${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src>
    $<BUILD_INTERFACE:${CEL_CPP_PARSER_OUTPUT_DIR}>
    $<BUILD_INTERFACE:${CEL_CPP_PARSER_GEN_DIR}>
)
target_compile_definitions(cel_cpp_parser PRIVATE ANTLR4CPP_USING_ABSEIL)
if(NOT MSVC)
    # We don't need this on MSVC because this is the default.
    # (We don't _want_ it either, since it isn't a valid command to the cl.exe
    # driver and causes a lot of warnings to be printed.)
    target_compile_options(cel_cpp_parser PUBLIC -fsized-deallocation)
endif()
target_link_libraries(cel_cpp_parser PUBLIC absl::base antlr4_static)
add_library(cel_cpp::parser ALIAS cel_cpp_parser)

# Build cel_cc_embed tool for embedding descriptor sets
add_executable(cel_cc_embed bazel/cel_cc_embed.cc)
target_link_libraries(cel_cc_embed PRIVATE
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
)
add_executable(cel_cpp::embed ALIAS cel_cc_embed)

# Generate code for googleapis protos
set(CEL_CPP_GOOGLEAPIS_PROTO_DIR ${googleapis_SOURCE_DIR})
set(CEL_CPP_GOOGLEAPIS_PROTO
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
list(TRANSFORM CEL_CPP_GOOGLEAPIS_PROTO PREPEND ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}/)
set(CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/proto_cc_googleapis)
file(MAKE_DIRECTORY ${CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR})
add_library(cel_googleapis_proto OBJECT ${CEL_CPP_GOOGLEAPIS_PROTO})
target_include_directories(cel_googleapis_proto PUBLIC
    $<BUILD_INTERFACE:${CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cel_googleapis_proto PUBLIC protobuf::libprotobuf)
protobuf_generate(
    TARGET cel_googleapis_proto
    LANGUAGE cpp
    PROTOC_OUT_DIR ${CEL_CPP_GOOGLEAPIS_PROTO_GEN_DIR}
    IMPORT_DIRS ${CEL_CPP_GOOGLEAPIS_PROTO_DIR}
                ${PROTOBUF_IMPORT_PATH}
)
add_library(cel_cpp::googleapis_proto ALIAS cel_googleapis_proto)

# Generate C++ code for cel spec protos
set(CEL_SPEC_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/proto_cc_cel_spec)
file(MAKE_DIRECTORY ${CEL_SPEC_PROTO_GEN_DIR}/proto)
set(CEL_SPEC_PROTO
    test/v1/proto2/test_all_types.proto
    test/v1/proto3/test_all_types.proto
    cel/expr/checked.proto
    cel/expr/eval.proto
    cel/expr/explain.proto
    cel/expr/syntax.proto
    cel/expr/value.proto
)
set(CEL_SPEC_PROTO_DIR ${cel_spec_SOURCE_DIR}/proto)
list(TRANSFORM CEL_SPEC_PROTO PREPEND ${CEL_SPEC_PROTO_DIR}/)
add_library(cel_cpp_spec_proto OBJECT ${CEL_SPEC_PROTO})
target_include_directories(cel_cpp_spec_proto
    PUBLIC ${CEL_SPEC_PROTO_GEN_DIR}/proto
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

# Generate empty descriptor set
set(CEL_CPP_EMPTY_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/empty_descriptor_set.pb)
set(CEL_CPP_EMPTY_DESCRIPTOR_SET_PROTO google/protobuf/empty.proto)
list(TRANSFORM CEL_CPP_EMPTY_DESCRIPTOR_SET_PROTO PREPEND ${PROTOBUF_IMPORT_PATH}/)
add_custom_command(
    OUTPUT ${CEL_CPP_EMPTY_DESCRIPTOR_SET_PATH}
    COMMAND ${PROTOC_EXECUTABLE}
            --include_imports
            --descriptor_set_out=${CEL_CPP_EMPTY_DESCRIPTOR_SET_PATH}
            -I${PROTOBUF_IMPORT_PATH}
            ${CEL_CPP_EMPTY_DESCRIPTOR_SET_PROTO}
    DEPENDS ${CEL_CPP_EMPTY_DESCRIPTOR_SET_PROTO}
    COMMENT "Generating empty descriptor set"
    VERBATIM
)
set(CEL_CPP_EMPTY_DESCRIPTOR_SET_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/empty_descriptor_set)
file(MAKE_DIRECTORY ${CEL_CPP_EMPTY_DESCRIPTOR_SET_GEN_DIR}/internal)
set(CEL_CPP_EMPTY_DESCRIPTOR_SET_EMBED_PATH ${CEL_CPP_EMPTY_DESCRIPTOR_SET_GEN_DIR}/internal/empty_descriptor_set_embed.inc)
add_custom_command(
    OUTPUT ${CEL_CPP_EMPTY_DESCRIPTOR_SET_EMBED_PATH}
    COMMAND $<TARGET_FILE:cel_cpp::embed>
            --in=${CEL_CPP_EMPTY_DESCRIPTOR_SET_PATH}
            --out=${CEL_CPP_EMPTY_DESCRIPTOR_SET_EMBED_PATH}
    DEPENDS ${CEL_CPP_EMPTY_DESCRIPTOR_SET_PATH}
            cel_cpp::embed
    COMMENT "Embedding empty descriptor set"
    VERBATIM
)
add_custom_target(cel_cpp_empty_descriptor_set_gen DEPENDS ${CEL_CPP_EMPTY_DESCRIPTOR_SET_EMBED_PATH})
add_library(cel_cpp_empty_descriptor_set INTERFACE)
add_dependencies(cel_cpp_empty_descriptor_set cel_cpp_empty_descriptor_set_gen)
target_include_directories(cel_cpp_empty_descriptor_set INTERFACE
    $<BUILD_INTERFACE:${CEL_CPP_EMPTY_DESCRIPTOR_SET_GEN_DIR}>
)
add_library(cel_cpp::empty_descriptor_set ALIAS cel_cpp_empty_descriptor_set)

# Generate embedded descriptor set
set(CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/minimal_descriptor_set.pb)
set(CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTO
    google/protobuf/any.proto
    google/protobuf/duration.proto
    google/protobuf/struct.proto
    google/protobuf/timestamp.proto
    google/protobuf/wrappers.proto
)
list(TRANSFORM CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTO PREPEND ${PROTOBUF_IMPORT_PATH}/)
add_custom_command(
    OUTPUT ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH}
    COMMAND ${PROTOC_EXECUTABLE}
            --include_imports
            --descriptor_set_out=${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PATH}
            -I${PROTOBUF_IMPORT_PATH}
            ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTO}
    DEPENDS ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTO}
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
target_include_directories(cel_cpp_minimal_descriptor_set INTERFACE
    $<BUILD_INTERFACE:${CEL_CPP_MINIMAL_DESCRIPTOR_SET_GEN_DIR}>
)
add_library(cel_cpp::minimal_descriptor_set ALIAS cel_cpp_minimal_descriptor_set)

# Collect core source files
file(GLOB CEL_CPP_CORE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    base/*.cc
    base/ast_internal/*.cc
    base/internal/*.cc
    checker/*.cc
    checker/internal/*.cc
    common/*.cc
    common/ast/*.cc
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
list(FILTER CEL_CPP_CORE_SOURCES EXCLUDE REGEX ".*_benchmarks\\.cc$")

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

    # These seem to be legitimately broken in cel-cpp v0.10.0.
    # They do not work on any platform.
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
    # Unsure what is wrong with this one. Broken on all platforms.
    eval/compiler/flat_expr_builder_test.cc
)

add_library(cel_cpp STATIC ${CEL_CPP_CORE_SOURCES} ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_EMBED_PATH})
target_link_libraries(cel_cpp PUBLIC
    cel_cpp::parser
    cel_cpp::empty_descriptor_set
    cel_cpp::minimal_descriptor_set
    cel_cpp::googleapis_proto
    cel_cpp::spec_proto
    absl::base
    protobuf::libprotobuf
    re2::re2
)
target_include_directories(cel_cpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_definitions(cel_cpp PRIVATE ANTLR4CPP_USING_ABSEIL)
add_library(cel_cpp::cel_cpp ALIAS cel_cpp)

if(CEL_CPP_ENABLE_TESTS)
    enable_testing()

    # Generate C++ code for cel-cpp test protos
    set(CEL_CPP_TEST_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/proto_cc_cel_cpp_test)
    file(MAKE_DIRECTORY ${CEL_CPP_TEST_PROTO_GEN_DIR})
    set(CEL_CPP_TEST_PROTO
        eval/testutil/test_message.proto
        eval/testutil/simple_test_message.proto
        eval/testutil/test_extensions.proto
        eval/tests/request_context.proto
    )
    add_library(cel_cpp_test_proto OBJECT ${CEL_CPP_TEST_PROTO})
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

    # Generate embedded descriptor set for testing
    set(CEL_CPP_TESTING_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/testing_descriptor_set.pb)
    set(CEL_CPP_TESTING_PROTO
        ${CEL_CPP_MINIMAL_DESCRIPTOR_SET_PROTO}
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
                ${CEL_CPP_TESTING_PROTO}
        DEPENDS ${CEL_CPP_TESTING_PROTO}
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

    # Generate descriptor set for flat_expr_builder_test.cc.
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/eval/testutil/)
    set(CEL_CPP_SIMPLE_TEST_MESSAGE_DESCRIPTOR_SET_PATH ${CMAKE_CURRENT_BINARY_DIR}/eval/testutil/simple_test_message_proto-descriptor-set.proto.bin)
    set(CEL_CPP_SIMPLE_TEST_MESSAGE_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/eval/testutil/simple_test_message.proto)
    add_custom_command(
        OUTPUT ${CEL_CPP_SIMPLE_TEST_MESSAGE_DESCRIPTOR_SET_PATH}
        COMMAND ${PROTOC_EXECUTABLE}
                --include_source_info
                --descriptor_set_out=${CEL_CPP_SIMPLE_TEST_MESSAGE_DESCRIPTOR_SET_PATH}
                -I${CMAKE_CURRENT_SOURCE_DIR}/eval/testutil
                ${CEL_CPP_SIMPLE_TEST_MESSAGE_PROTO}
        DEPENDS ${CEL_CPP_SIMPLE_TEST_MESSAGE_PROTO}
        COMMENT "Generating simple test message descriptor set"
        VERBATIM
    )
    add_custom_target(cel_cpp_simple_test_message_descriptor_set_gen DEPENDS ${CEL_CPP_SIMPLE_TEST_MESSAGE_DESCRIPTOR_SET_PATH})
    add_library(cel_cpp_simple_test_message_descriptor_set INTERFACE)
    add_dependencies(cel_cpp_simple_test_message_descriptor_set cel_cpp_simple_test_message_descriptor_set_gen)
    add_library(cel_cpp::simple_test_message_descriptor_set ALIAS cel_cpp_simple_test_message_descriptor_set)

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
        cel_cpp::simple_test_message_descriptor_set
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
