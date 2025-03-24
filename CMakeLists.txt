cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(protovalidate-cc)

option(PROTOVALIDATE_CC_ENABLE_VENDORING "Fall back to vendored libraries when possible" ON)

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # In subproject: set appropriate defaults for embedding
    option(PROTOVALIDATE_CC_ENABLE_INSTALL "Enable protovalidate-cc installation targets" OFF)
    option(PROTOVALIDATE_CC_ENABLE_TESTS "Enable protovalidate-cc unit tests" OFF)
    option(PROTOVALIDATE_CC_ENABLE_CONFORMANCE "Build conformance runner" OFF)
else()
    # Only set C++ standard when not being embedded
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Standalone: set appropriate defaults for system-wide installation
    option(PROTOVALIDATE_CC_ENABLE_INSTALL "Enable protovalidate-cc installation targets" ON)
    option(PROTOVALIDATE_CC_ENABLE_TESTS "Enable protovalidate-cc unit tests" ON)
    option(PROTOVALIDATE_CC_ENABLE_CONFORMANCE "Build conformance runner" ON)
endif()

if(CMAKE_CXX_STANDARD LESS 17)
  message(FATAL_ERROR "protovalidate-cc requires CMAKE_CXX_STANDARD >= 17 (got: ${CMAKE_CXX_STANDARD})")
endif()

if(PROTOVALIDATE_CC_ENABLE_INSTALL)
    include(GNUInstallDirs)
endif()

include(GitVersion)
include(Deps)

set(PROTOVALIDATE_LIBS
    cel_cpp::cel_cpp
    protovalidate::proto
)
set(PROTOVALIDATE_TESTING_LIBS
    GTest::gmock_main
    protovalidate::testing_proto
)
file(GLOB PROTOVALIDATE_CC_CORE_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    buf/validate/*.h
    buf/validate/internal/*.h
)
file(GLOB PROTOVALIDATE_CC_CORE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    buf/validate/*.cc
    buf/validate/internal/*.cc
)
set(PROTOVALIDATE_CC_TEST_SOURCES
    ${PROTOVALIDATE_CC_CORE_SOURCES}
    ${PROTOVALIDATE_CC_CONFORMANCE_SOURCES}
)
list(FILTER PROTOVALIDATE_CC_TEST_SOURCES INCLUDE REGEX ".*_test\\.cc$")
list(FILTER PROTOVALIDATE_CC_CORE_SOURCES EXCLUDE REGEX ".*_test\\.cc$")
add_library(protovalidate_cc ${PROTOVALIDATE_CC_CORE_SOURCES} ${PROTOVALIDATE_CC_CORE_HEADERS})
target_compile_features(protovalidate_cc PUBLIC cxx_std_17)
target_link_libraries(protovalidate_cc PUBLIC ${PROTOVALIDATE_LIBS})
target_include_directories(protovalidate_cc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
add_library(protovalidate_cc::protovalidate_cc ALIAS protovalidate_cc)

if(PROTOVALIDATE_CC_ENABLE_CONFORMANCE)
    set(PROTOVALIDATE_CC_CONFORMANCE_HEADERS buf/validate/conformance/runner.h)
    set(PROTOVALIDATE_CC_CONFORMANCE_SOURCES buf/validate/conformance/runner.cc)
    add_library(protovalidate_cc_conformance_runner OBJECT ${PROTOVALIDATE_CC_CONFORMANCE_SOURCES} ${PROTOVALIDATE_CC_CONFORMANCE_HEADERS})
    target_link_libraries(protovalidate_cc_conformance_runner PUBLIC
        protovalidate_cc::protovalidate_cc
        protovalidate::testing_proto
    )
    add_library(protovalidate_cc::conformance_runner ALIAS protovalidate_cc_conformance_runner)

    add_executable(protovalidate_cc_conformance_runner_main buf/validate/conformance/runner_main.cc)
    target_link_libraries(protovalidate_cc_conformance_runner_main PUBLIC
        protovalidate_cc::conformance_runner
        protovalidate::testing_proto
    )
    add_executable(protovalidate_cc::conformance_runner_main ALIAS protovalidate_cc_conformance_runner_main)

    # Add conformance runner unit tests
    list(APPEND PROTOVALIDATE_CC_TEST_SOURCES buf/validate/conformance/runner_test.cc)
    list(APPEND PROTOVALIDATE_TESTING_LIBS protovalidate_cc::conformance_runner)
endif()

if(PROTOVALIDATE_CC_ENABLE_TESTS AND BUILD_TESTING)
    enable_testing()

    foreach(PROTOVALIDATE_CC_TEST_SOURCE IN LISTS PROTOVALIDATE_CC_TEST_SOURCES)
        string(REPLACE "/" "_" target_name "${PROTOVALIDATE_CC_TEST_SOURCE}")
        string(REPLACE ".cc" "" target_name "${target_name}")
        add_executable(${target_name} ${PROTOVALIDATE_CC_TEST_SOURCE})
        target_link_libraries(${target_name} PUBLIC ${PROTOVALIDATE_TESTING_LIBS})
        add_test(NAME ${target_name} COMMAND ${target_name})
        unset(target_name)
    endforeach()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(EXPECTED_FAILURES_FILE "expected_failures_linux.yaml")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(EXPECTED_FAILURES_FILE "expected_failures_mac.yaml")
    else()
        # Default to Linux for now
        set(EXPECTED_FAILURES_FILE "expected_failures_linux.yaml")
    endif()

    if(PROTOVALIDATE_CC_ENABLE_CONFORMANCE)
        add_test(
            NAME protovalidate_cc_conformance
            COMMAND go run github.com/bufbuild/protovalidate/tools/protovalidate-conformance@v${PROTOVALIDATE_CC_PROTOVALIDATE_VERSION}
                    --strict_message
                    --expected_failures=${CMAKE_CURRENT_SOURCE_DIR}/buf/validate/conformance/${EXPECTED_FAILURES_FILE}
                    $<TARGET_FILE:protovalidate_cc::conformance_runner_main>
        )
    endif()
endif()

if(PROTOVALIDATE_CC_ENABLE_INSTALL)
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buf
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
    )

    install(
        DIRECTORY ${PROTOVALIDATE_PROTO_GEN_DIR}/buf
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
    )

    install(
        TARGETS protovalidate_cc ${PROTOVALIDATE_CC_EXPORT_TARGETS}
        EXPORT protovalidate_cc-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )

    install(
        EXPORT protovalidate_cc-targets
        NAMESPACE protovalidate_cc::
        FILE protovalidate_cc-targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/protovalidate_cc
    )

    include(CMakePackageConfigHelpers)

    # CMake configuration scripts
    list(JOIN PROTOVALIDATE_CC_FIND_DEPENDENCIES "\n" PROTOVALIDATE_CC_FIND_DEPENDENCIES_STR)
    string(REGEX MATCH "([0-9]+.[0-9]+.[0-9]+)" PROTOVALIDATE_CC_CMAKE_VERSION "${PROTOVALIDATE_CC_GIT_VERSION}")
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/protovalidate_cc-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/protovalidate_cc-config.cmake"
        INSTALL_DESTINATION "lib/cmake/protovalidate_cc"
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/protovalidate_cc-config-version.cmake"
        VERSION "${PROTOVALIDATE_CC_CMAKE_VERSION}"
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/protovalidate_cc-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/protovalidate_cc-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/protovalidate_cc
    )

    # pkg-config configuration script
    list(JOIN PROTOVALIDATE_CC_PKG_CONFIG_REQS ", " PROTOVALIDATE_CC_PKG_CONFIG_REQS_STR)
    set(PROTOVALIDATE_CC_PKG_CONFIG_PREFIX ${CMAKE_INSTALL_PREFIX})
    set(PROTOVALIDATE_CC_PKG_CONFIG_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
    set(PROTOVALIDATE_CC_PKG_CONFIG_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    set(PROTOVALIDATE_CC_PKG_CONFIG_VERSION "${PROTOVALIDATE_CC_GIT_VERSION}")
    set(PROTOVALIDATE_CC_PKG_CONFIG_LIBS "")
    foreach(_DEP ${PROTOVALIDATE_CC_DEPS})
        set(PROTOVALIDATE_CC_PKG_CONFIG_LIBS "${PROTOVALIDATE_CC_PKG_CONFIG_LIBS} -l${_DEP}")
    endforeach()
    configure_file(protovalidate_cc.pc.in protovalidate_cc.pc @ONLY)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/protovalidate_cc.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()
