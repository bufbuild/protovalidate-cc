include(FetchContent)
include(SharedDeps)
include(MakePatchCommand)

# CMake will warn that this variable is unused if it is set.
# This no-op prevents that warning.
if(CMAKE_TOOLCHAIN_FILE)
endif()

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/deps/shared_deps.json SHARED_DEPS)

set(PROTOVALIDATE_CC_EXPORT_TARGETS "")
set(PROTOVALIDATE_CC_PKG_CONFIG_REQS "")
set(PROTOVALIDATE_CC_FIND_DEPENDENCIES "")

# Googletest, only needed when tests are enabled.
if((PROTOVALIDATE_CC_ENABLE_TESTS OR CEL_CPP_ENABLE_TESTS) AND BUILD_TESTING)
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

if(CEL_CPP_ENABLE_TESTS AND BUILD_TESTING)
    # Google Benchmark
    if(TARGET benchmark::benchmark)
        message(STATUS "protovalidate-cc: Using pre-existing google benchmark targets")
    else()
        find_package(benchmark CONFIG)
        if(benchmark_FOUND)
            message(STATUS "protovalidate-cc: Using external google benchmark ${benchmark_VERSION}")
        else()
            message(STATUS "protovalidate-cc: Fetching google benchmark")
            if(PROTOVALIDATE_CC_ENABLE_VENDORING)
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
            else()
                message(FATAL_ERROR "protovalidate-cc: Could not find google benchmark package and vendoring is disabled.")
            endif()
        endif()
    endif()

    # Fetch CEL spec repository (needed for test protos)
    # When building in a hermetic environment, use the
    # FETCHCONTENT_SOURCE_DIR_CEL_SPEC option to avoid the external fetch.
    # https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
    FetchContent_Declare(
        cel_spec
        GIT_REPOSITORY https://github.com/google/cel-spec.git
        GIT_TAG f027a86d2e5bf18f796be0c4373f637a61041cde
    )
    FetchContent_MakeAvailable(cel_spec)
endif()

# Abseil
if(TARGET absl::base)
    message(STATUS "protovalidate-cc: Using pre-existing absl targets")
else()
    find_package(absl CONFIG)
    if(absl_FOUND)
        if(absl_VERSION LESS 20240722)
            message(FATAL_ERROR "protovalidate-cc: cel-cpp is known to need absl 20240722 or higher (found: ${absl_VERSION}). Please update absl, or to use a vendored copy of absl instead, re-run the configuration with -DCMAKE_DISABLE_FIND_PACKAGE_absl=TRUE.")
        endif()
        message(STATUS "protovalidate-cc: Using external absl ${absl_VERSION}")
        set(protobuf_ABSL_PROVIDER "package")
        list(APPEND PROTOVALIDATE_CC_PKG_CONFIG_REQS "absl_base")
        list(APPEND PROTOVALIDATE_CC_FIND_DEPENDENCIES "find_dependency(absl CONFIG)")
    else()
        if(PROTOVALIDATE_CC_ENABLE_VENDORING)
            if(PROTOVALIDATE_CC_ENABLE_INSTALL)
                message(FATAL_ERROR "protovalidate-cc: Installation can not be enabled when using vendored absl. Install absl system-wide, or disable installation using -DPROTOVALIDATE_CC_ENABLE_INSTALL=OFF.")
            endif()
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
            list(APPEND PROTOVALIDATE_CC_EXPORT_TARGETS absl_base)
        else()
            message(FATAL_ERROR "protovalidate-cc: Could not find absl package and vendoring is disabled.")
        endif()
    endif()
endif()
get_property(PROTOVALIDATE_CC_ABSL_IMPORTED TARGET absl::status PROPERTY IMPORTED)
if(PROTOVALIDATE_CC_ABSL_IMPORTED)
    set(PROTOVALIDATE_CC_ABSL_TRY_COMPILE_FLAGS LINK_LIBRARIES absl::status)
else()
    set(PROTOVALIDATE_CC_ABSL_TRY_COMPILE_FLAGS CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${absl_SOURCE_DIR})
endif()
try_compile(ABSL_CAN_MOVE_ASSIGN_STATUS
    SOURCE_FROM_CONTENT test_absl_status_move_assign.cc
                        "#include <type_traits>\n\
                        #include <absl/status/status.h>\n\
                        static_assert(std::is_nothrow_move_assignable_v<absl::Status>);\n\
                        int main() {}"
    ${PROTOVALIDATE_CC_ABSL_TRY_COMPILE_FLAGS}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
    NO_CACHE
)
if(NOT ABSL_CAN_MOVE_ASSIGN_STATUS)
    message(FATAL_ERROR "protovalidate-cc: absl::Status is not nothrow-move-assignable; please make sure your copy of absl is up-to-date enough (cel-cpp is known to need at least 20240722). To use a vendored copy of absl instead, re-run the configuration with -DCMAKE_DISABLE_FIND_PACKAGE_absl=TRUE.")
endif()


# Protobuf
if(TARGET protobuf::libprotobuf)
    message(STATUS "protovalidate-cc: Using pre-existing protobuf targets")
else()
    find_package(Protobuf CONFIG)
    if(Protobuf_FOUND)
        if(NOT TARGET protobuf::libprotoc)
            message(FATAL_ERROR "protovalidate-cc: Please install libprotoc development files.")
        endif()
        message(STATUS "protovalidate-cc: Using external protobuf ${Protobuf_VERSION}")
    else()
        if(PROTOVALIDATE_CC_ENABLE_VENDORING)
            if(PROTOVALIDATE_CC_ENABLE_INSTALL)
                message(FATAL_ERROR "protovalidate-cc: Installation can not be enabled when using vendored protobuf. Install protobuf system-wide, or disable installation using -DPROTOVALIDATE_CC_ENABLE_INSTALL=OFF.")
            endif()
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
            set(PROTOBUF_IMPORT_PATH ${protobuf_SOURCE_DIR}/src)
        else()
            message(FATAL_ERROR "protovalidate-cc: Could not find protobuf package and vendoring is disabled.")
        endif()
    endif()
endif()
set(PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)
if(DEFINED Protobuf_INCLUDE_DIRS)
    set(PROTOBUF_IMPORT_PATH ${Protobuf_INCLUDE_DIRS})
elseif(DEFINED protobuf_SOURCE_DIR)
    set(PROTOBUF_IMPORT_PATH ${protobuf_SOURCE_DIR}/src)
else()
    message(FATAL_ERROR "protovalidate-cc: Error determining protobuf import path: Protobuf_INCLUDE_DIRS is not set. Check that your Protobuf installation is correct and up-to-date enough. To use a vendored copy of Protobuf instead, re-run the configuration with -DCMAKE_DISABLE_FIND_PACKAGE_Protobuf=TRUE.")
endif()

# Re2
if(TARGET re2::re2)
    message(STATUS "protovalidate-cc: Using pre-existing re2 targets")
else()
    find_package(re2 CONFIG)
    if(re2_FOUND)
        message(STATUS "protovalidate-cc: Using external re2 ${re2_VERSION}")
    else()
        if(PROTOVALIDATE_CC_ENABLE_VENDORING)
            if(PROTOVALIDATE_CC_ENABLE_INSTALL)
                message(FATAL_ERROR "protovalidate-cc: Installation can not be enabled when using vendored re2. Install re2 system-wide, or disable installation using -DPROTOVALIDATE_CC_ENABLE_INSTALL=OFF.")
            endif()
            message(STATUS "protovalidate-cc: Fetching re2")
            set(RE2_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/re2/0001-Add-RE2_INSTALL-option.patch)
            FetchContent_Declare(
                re2
                GIT_REPOSITORY "https://github.com/google/re2.git"
                GIT_TAG "2024-04-01"
                GIT_SHALLOW TRUE
                PATCH_COMMAND git apply --check -R ${RE2_PATCH} || git apply ${RE2_PATCH}
            )
            set(RE2_INSTALL OFF)
            FetchContent_MakeAvailable(re2)
        else()
            message(FATAL_ERROR "protovalidate-cc: Could not find re2 package and vendoring is disabled.")
        endif()
    endif()
endif()

# ANTLR4; ANTLR4 is always vendored for now. When building in a hermetic
# environment, use the FETCHCONTENT_SOURCE_DIR_ANTLR4 option to avoid the
# external fetch.
# https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
# ANTLR4 has CMake support, but we need ANTLR4CPP_USING_ABSEIL which it doesn't support yet.
include(Antlr4)
list(APPEND PROTOVALIDATE_CC_EXPORT_TARGETS antlr4_static)

# googleapis protos; These don't tend to be packaged everywhere, so it is always
# vendored. When building in a hermetic environment, use the
# FETCHCONTENT_SOURCE_DIR_GOOGLEAPIS option to avoid the external fetch.
# https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
message(STATUS "protovalidate-cc: Fetching googleapis")
FetchContent_Declare(
    googleapis
    GIT_REPOSITORY https://github.com/googleapis/googleapis.git
    GIT_TAG 6eb56cdf5f54f70d0dbfce051add28a35c1203ce
)
FetchContent_MakeAvailable(googleapis)

# Cel-cpp; Note that cel-cpp has no CMake build and is not packaged anywhere,
# so it is always vendored. When building in a hermetic environment, use the
# FETCHCONTENT_SOURCE_DIR_CEL_CPP option to avoid the external fetch.
# https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_CEL_CPP_URLS "cel_cpp" "urls" "${SHARED_DEPS}")
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_CEL_CPP_SHA256 "cel_cpp" "sha256" "${SHARED_DEPS}")
set(CEL_CPP_PATCHES
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/cel_cpp/0001-Allow-message-field-access-using-index-operator.patch
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/cel_cpp/0002-Add-missing-include-for-absl-StrCat.patch
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/cel_cpp/0003-Remove-unnecessary-dependency-on-cel_proto_wrap_util.patch
)
MakePatchCommand(CEL_CPP_PATCH_COMMAND "${CEL_CPP_PATCHES}")
message(STATUS "protovalidate-cc: Fetching cel-cpp")
FetchContent_Declare(cel_cpp
    URL ${PROTOVALIDATE_CC_CEL_CPP_URLS}
    URL_HASH SHA256=${PROTOVALIDATE_CC_CEL_CPP_SHA256}
    PATCH_COMMAND ${CEL_CPP_PATCH_COMMAND}

    # THis stops MakeAvailable from trying to add_subdirectory, so we can do it
    # ourselves after adding the embedded CMakeLists.txt.
    SOURCE_SUBDIR nonexistant
)
FetchContent_MakeAvailable(cel_cpp)
# Ensure CMake reconfigures when the embedded CMakeLists is updated
set_property(DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cel-cpp/CMakeLists.txt
)
file(COPY_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cel-cpp/CMakeLists.txt
    ${cel_cpp_SOURCE_DIR}/CMakeLists.txt
)
message(STATUS "Added ${cel_cpp_SOURCE_DIR}/CMakeLists.txt")
add_subdirectory(${cel_cpp_SOURCE_DIR} ${cel_cpp_BINARY_DIR})
list(APPEND PROTOVALIDATE_CC_EXPORT_TARGETS 
    cel_cpp
    cel_cpp_parser
    cel_cpp_minimal_descriptor_set
    cel_googleapis_proto
)

# Protovalidate; mainly needed for the proto files. There is no CMake build and
# it is not packaged anywhere, so it is always vendored. When building in a
# hermetic environment, use the FETCHCONTENT_SOURCE_DIR_PROTOVALIDATE option to
# avoid the external fetch.
# https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_PROTOVALIDATE_URLS "protovalidate" "urls" "${SHARED_DEPS}")
SharedDeps_GetSourceValue(PROTOVALIDATE_CC_PROTOVALIDATE_SHA256 "protovalidate" "sha256" "${SHARED_DEPS}")
SharedDeps_GetMetaValue(PROTOVALIDATE_CC_PROTOVALIDATE_VERSION "protovalidate" "version" "${SHARED_DEPS}")
message(STATUS "protovalidate-cc: Fetching protovalidate")
FetchContent_Declare(
    protovalidate
    URL ${PROTOVALIDATE_CC_PROTOVALIDATE_URLS}
    URL_HASH SHA256=${PROTOVALIDATE_CC_PROTOVALIDATE_SHA256}
)
FetchContent_MakeAvailable(protovalidate)

set(PROTOVALIDATE_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/proto_cc_protovalidate)
file(MAKE_DIRECTORY ${PROTOVALIDATE_PROTO_GEN_DIR})
file(GLOB_RECURSE PROTOVALIDATE_PROTO ${protovalidate_SOURCE_DIR}/proto/protovalidate/*.proto)
add_library(protovalidate_proto OBJECT ${PROTOVALIDATE_PROTO})
protobuf_generate(
    TARGET protovalidate_proto
    PROTOS ${PROTOVALIDATE_PROTO}
    LANGUAGE cpp
    PROTOC_OUT_DIR ${PROTOVALIDATE_PROTO_GEN_DIR}
    IMPORT_DIRS
        ${protovalidate_SOURCE_DIR}/proto/protovalidate
        ${PROTOBUF_IMPORT_PATH}
)
target_include_directories(protovalidate_proto PUBLIC
    $<BUILD_INTERFACE:${PROTOVALIDATE_PROTO_GEN_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(protovalidate_proto PUBLIC protobuf::libprotobuf)
add_library(protovalidate::proto ALIAS protovalidate_proto)
list(APPEND PROTOVALIDATE_CC_EXPORT_TARGETS protovalidate_proto)

if((PROTOVALIDATE_CC_ENABLE_TESTS AND BUILD_TESTING) OR PROTOVALIDATE_CC_ENABLE_CONFORMANCE)
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

    set(PROTOVALIDATE_TESTING_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen/proto_cc_protovalidate_testing)
    file(MAKE_DIRECTORY ${PROTOVALIDATE_TESTING_PROTO_GEN_DIR})
    file(GLOB_RECURSE PROTOVALIDATE_TESTING_PROTO
        ${protovalidate_SOURCE_DIR}/proto/protovalidate-testing/buf/*.proto
    )
    add_library(protovalidate_testing_proto OBJECT ${PROTOVALIDATE_TESTING_PROTO})
    protobuf_generate(
        TARGET protovalidate_testing_proto
        PROTOS ${PROTOVALIDATE_TESTING_PROTO}
        LANGUAGE cpp
        PROTOC_OUT_DIR ${PROTOVALIDATE_TESTING_PROTO_GEN_DIR}
        DEPENDENCIES protovalidate_proto
        IMPORT_DIRS ${protovalidate_SOURCE_DIR}/proto/protovalidate-testing
                    ${protovalidate_SOURCE_DIR}/proto/protovalidate
                    ${protoc_gen_validate_SOURCE_DIR}
                    ${PROTOBUF_IMPORT_PATH}
    )
    target_include_directories(protovalidate_testing_proto PUBLIC
        ${PROTOVALIDATE_PROTO_GEN_DIR}
        ${PROTOVALIDATE_TESTING_PROTO_GEN_DIR}
    )
    target_link_libraries(protovalidate_testing_proto PUBLIC protobuf::libprotobuf)
    add_library(protovalidate::testing_proto ALIAS protovalidate_testing_proto)
endif()
