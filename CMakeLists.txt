cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(protovalidate-cc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # In subproject: set appropriate defaults for embedding
    option(PROTOVALIDATE_CC_ENABLE_VENDORING "Fall back to vendored libraries when possible" ON)
    option(PROTOVALIDATE_CC_ENABLE_INSTALL "Enable protovalidate-cc installation targets" OFF)
    option(PROTOVALIDATE_CC_ENABLE_TESTS "Enable protovalidate-cc unit tests" OFF)
    option(PROTOVALIDATE_CC_ENABLE_CONFORMANCE "Build conformance runner" OFF)
else()
    # Standalone: set appropriate defaults for system-wide installation
    option(PROTOVALIDATE_CC_ENABLE_VENDORING "Fall back to vendored libraries when possible" ON)
    option(PROTOVALIDATE_CC_ENABLE_INSTALL "Enable protovalidate-cc installation targets" ON)
    option(PROTOVALIDATE_CC_ENABLE_TESTS "Enable protovalidate-cc unit tests" ON)
    option(PROTOVALIDATE_CC_ENABLE_CONFORMANCE "Build conformance runner" ON)
endif()

include(Deps)

set(PROTOVALIDATE_LIBS
    cel_cpp::cel_cpp
    protovalidate::proto
)
set(PROTOVALIDATE_TESTING_LIBS
    GTest::gmock_main
    protovalidate_cc::protovalidate_cc
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
target_link_libraries(protovalidate_cc PUBLIC ${PROTOVALIDATE_LIBS})
target_include_directories(protovalidate_cc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
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

    if(PROTOVALIDATE_CC_ENABLE_CONFORMANCE)
        add_test(
            NAME protovalidate_cc_conformance
            COMMAND go run github.com/bufbuild/protovalidate/tools/protovalidate-conformance@v${PROTOVALIDATE_CC_PROTOVALIDATE_VERSION}
                    --strict
                    $<TARGET_FILE:protovalidate_cc::conformance_runner_main>
        )
    endif()
endif()


