cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchContent)
include(SharedDeps)

project(protovalidate-cc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake will warn that this variable is unused if it is set.
# This no-op prevents that warning.
if(CMAKE_TOOLCHAIN_FILE)
endif()

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/deps/shared_deps.json SHARED_DEPS)

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # In subproject: set appropriate defaults for embedding
    option(PROTOVALIDATE_CC_ENABLE_VENDORING "Fall back to vendored libraries when possible" ON)
    option(PROTOVALIDATE_CC_ENABLE_INSTALL "Enable protovalidate-cc installation targets" OFF)
    option(PROTOVALIDATE_CC_ENABLE_TESTS "Enable protovalidate-cc unit tests" OFF)
else()
    # Standalone: set appropriate defaults for system-wide installation
    option(PROTOVALIDATE_CC_ENABLE_VENDORING "Fall back to vendored libraries when possible" OFF)
    option(PROTOVALIDATE_CC_ENABLE_INSTALL "Enable protovalidate-cc installation targets" ON)
    option(PROTOVALIDATE_CC_ENABLE_TESTS "Enable protovalidate-cc unit tests" ON)
endif()

if(PROTOVALIDATE_CC_ENABLE_VENDORING)
    message(STATUS "protovalidate-cc: Disabling installation because vendoring is enabled.")
    set(PROTOVALIDATE_CC_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
endif()

# Googletest, only needed when tests are enabled.
if(PROTOVALIDATE_CC_ENABLE_TESTS AND BUILD_TESTING)
    message(STATUS "protovalidate-cc: tests are enabled")
    enable_testing()
    set(ABSL_BUILD_TEST_HELPERS ON)

    if(TARGET GTest::gtest)
        message(STATUS "protovalidate-cc: Using pre-existing googletest targets")
    else()
        find_package(GTest CONFIG)
        if(GTest_FOUND)
            message(STATUS "protovalidate-cc: Using external googletest ${GTest_VERSION}")
        else()
            if(PROTOVALIDATE_CC_ENABLE_VENDORING)
                message(STATUS "protovalidate-cc: Fetching googletest")
                option(INSTALL_GTEST OFF)
                option(INSTALL_GMOCK OFF)
                set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
                FetchContent_Declare(
                    googletest
                    URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
                    URL_HASH SHA256=8c0ceafa3ea24bf78e3519b7846d99e76c45899aa4dac4d64e7dd62e495de9fd
                )
                FetchContent_MakeAvailable(googletest)
                set(ABSL_USE_EXTERNAL_GOOGLETEST ON CACHE BOOL "" FORCE)
                set(ABSL_FIND_GOOGLETEST OFF CACHE BOOL "" FORCE)
                if(NOT TARGET GTest::gtest)
                    add_library(GTest::gtest ALIAS gtest)
                    add_library(GTest::gmock ALIAS gmock)
                    add_library(GTest::gtest_main ALIAS gtest_main)
                    add_library(GTest::gmock_main ALIAS gmock_main)
                endif()
            else()
                message(FATAL_ERROR "protovalidate-cc: Could not find googletest package and vendoring is disabled.")
            endif()
        endif()
    endif()
endif()

# Abseil
if(TARGET absl::base)
    message(STATUS "protovalidate-cc: Using pre-existing absl targets")
else()
    SharedDeps_GetMetaValue(PROTOVALIDATE_CC_MIN_ABSL_VERSION "absl" "cmake_min_version" "${SHARED_DEPS}")
    find_package(absl ${PROTOVALIDATE_CC_MIN_ABSL_VERSION})
    if(absl_FOUND)
        message(STATUS "protovalidate-cc: Using external absl ${absl_VERSION}")
        set(protobuf_ABSL_PROVIDER "package")
    else()
        if(PROTOVALIDATE_CC_ENABLE_VENDORING)
            SharedDeps_GetSourceValue(PROTOVALIDATE_CC_ABSL_URLS "absl" "urls" "${SHARED_DEPS}")
            SharedDeps_GetSourceValue(PROTOVALIDATE_CC_ABSL_SHA256 "absl" "sha256" "${SHARED_DEPS}")
            message(STATUS "protovalidate-cc: Fetching absl")
            FetchContent_Declare(
                absl
                URL ${PROTOVALIDATE_CC_ABSL_URLS}
                URL_HASH SHA256=${PROTOVALIDATE_CC_ABSL_SHA256}
            )
            set(ABSL_PROPAGATE_CXX_STD ON)
            set(ABSL_ENABLE_INSTALL OFF)
            FetchContent_MakeAvailable(absl)
        else()
            message(FATAL_ERROR "protovalidate-cc: Could not find absl package and vendoring is disabled.")
        endif()
    endif()
endif()

# Protobuf
if(TARGET protobuf::libprotobuf)
    message(STATUS "protovalidate-cc: Using pre-existing protobuf targets")
    set(PROTOC_EXECUTABLE protoc)
    else()
    SharedDeps_GetMetaValue(PROTOVALIDATE_CC_PROTOBUF_VERSION_RANGE "protobuf" "cmake_version_range" "${SHARED_DEPS}")
    find_package(Protobuf ${PROTOVALIDATE_CC_PROTOBUF_VERSION_RANGE} CONFIG)
    if(Protobuf_FOUND)
        if(NOT TARGET protobuf::libprotoc)
            message(FATAL_ERROR "protovalidate-cc: Please install libprotoc development files.")
        endif()
        message(STATUS "protovalidate-cc: Using external protobuf ${Protobuf_VERSION}")
        set(PROTOC_EXECUTABLE ${Protobuf_PROTOC_EXECUTABLE})
        set(PROTOBUF_IMPORT_PATH ${Protobuf_INCLUDE_DIRS})
    else()
        if(PROTOVALIDATE_CC_ENABLE_VENDORING)
            SharedDeps_GetSourceValue(PROTOVALIDATE_CC_PROTOBUF_URLS "protobuf" "urls" "${SHARED_DEPS}")
            SharedDeps_GetSourceValue(PROTOVALIDATE_CC_PROTOBUF_SHA256 "protobuf" "sha256" "${SHARED_DEPS}")
            message(STATUS "protovalidate-cc: Fetching protobuf")
            FetchContent_Declare(
                protobuf
                URL ${PROTOVALIDATE_CC_PROTOBUF_URLS}
                URL_HASH SHA256=${PROTOVALIDATE_CC_PROTOBUF_SHA256}
            )
            set(protobuf_BUILD_TESTS OFF)
            set(protobuf_BUILD_EXPORT OFF)
            set(protobuf_INSTALL OFF)
            set(protobuf_MSVC_STATIC_RUNTIME OFF)
            FetchContent_MakeAvailable(protobuf)
            include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)
            set(PROTOC_EXECUTABLE $<TARGET_FILE:protoc>)
            set(PROTOBUF_IMPORT_PATH ${protobuf_SOURCE_DIR}/src)
        else()
            message(FATAL_ERROR "protovalidate-cc: Could not find protobuf package and vendoring is disabled.")
        endif()
    endif()
endif()

# Cel-cpp; Note that cel-cpp has no CMake build and is not packaged anywhere,
# so it is always vendored. When building in a hermetic environment, use the
# FETCHCONTENT_SOURCE_DIR_CEL_CPP option to avoid the external fetch.
# https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_CEL_CPP_URLS "cel_cpp" "urls" "${SHARED_DEPS}")
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_CEL_CPP_SHA256 "cel_cpp" "sha256" "${SHARED_DEPS}")
include(CelCpp)

# Protovalidate; mainly needed for the proto files. There is no CMake build and
# it is not packaged anywhere, so it is always vendored. When building in a
# hermetic environment, use the FETCHCONTENT_SOURCE_DIR_PROTOVALIDATE option to
# avoid the external fetch.
# https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_PROTOVALIDATE_URLS "protovalidate" "urls" "${SHARED_DEPS}")
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_PROTOVALIDATE_SHA256 "protovalidate" "sha256" "${SHARED_DEPS}")
message(STATUS "protovalidate-cc: Fetching protovalidate")
FetchContent_Declare(
    protovalidate
    URL ${PROTOVALIDATE_CC_PROTOVALIDATE_URLS}
    URL_HASH SHA256=${PROTOVALIDATE_CC_PROTOVALIDATE_SHA256}
)
FetchContent_MakeAvailable(protovalidate)

add_library(protovalidate_gen)
file(GLOB_RECURSE PROTOVALIDATE_PROTO_SOURCE_FILES ${protovalidate_SOURCE_DIR}/proto/protovalidate/*.proto)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen)
protobuf_generate(
    TARGET protovalidate_gen
    PROTOS ${PROTOVALIDATE_PROTO_SOURCE_FILES}
    LANGUAGE cpp
    OUT_VAR PROTO_GEN_FILES
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen
    IMPORT_DIRS
        ${protovalidate_SOURCE_DIR}/proto/protovalidate
        ${PROTOBUF_IMPORT_PATH}
)
target_include_directories(protovalidate_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/gen)
target_link_libraries(protovalidate_gen PUBLIC protobuf::libprotobuf)

if(PROTOVALIDATE_CC_ENABLE_TESTS)
    # protoc-gen-validate; just needed for the proto files. There is no CMake
    # build, so it is always vendored for now. When building in a hermetic
    # environment, use the FETCHCONTENT_SOURCE_DIR_PROTOC_GEN_VALIDATE option to
    # avoid the external fetch.
    # https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
    message(STATUS "protovalidate-cc: Fetching protoc-gen-validate")
    FetchContent_Declare(
        protoc_gen_validate
        URL https://github.com/bufbuild/protoc-gen-validate/archive/refs/tags/v1.2.1.tar.gz
    )
    FetchContent_MakeAvailable(protoc_gen_validate)

    add_library(protovalidate_testing_gen)
    file(GLOB_RECURSE PROTOVALIDATE_TESTING_PROTO_SOURCE_FILES
        ${protovalidate_SOURCE_DIR}/proto/protovalidate-testing/buf/*.proto
    )
    protobuf_generate(
        TARGET protovalidate_testing_gen
        PROTOS ${PROTOVALIDATE_TESTING_PROTO_SOURCE_FILES}
        LANGUAGE cpp
        OUT_VAR PROTO_GEN_FILES
        PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen
        DEPENDENCIES protovalidate_gen
        IMPORT_DIRS
            ${protovalidate_SOURCE_DIR}/proto/protovalidate-testing
            ${protovalidate_SOURCE_DIR}/proto/protovalidate
            ${protoc_gen_validate_SOURCE_DIR}
            ${PROTOBUF_IMPORT_PATH}
    )
    target_include_directories(protovalidate_testing_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/gen)
    target_link_libraries(protovalidate_testing_gen PUBLIC protobuf::libprotobuf)
endif()
