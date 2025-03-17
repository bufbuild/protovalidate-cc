cmake_minimum_required(VERSION 3.24)

project(protovalidate-cc-cmake-example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../.. ${CMAKE_CURRENT_BINARY_DIR}/protovalidate-cc)
add_executable(example main.cc)
target_link_libraries(example protovalidate_cc::protovalidate_cc)
