cmake_minimum_required(VERSION 3.26)

project(protovalidate-cc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if(APPLE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
elseif(UNIX)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

set(ABSL_PROPAGATE_CXX_STD ON)
set(protobuf_ABSL_PROVIDER "module")
set(protobuf_BUILD_PROTOC_BINARIES OFF)
set(protobuf_BUILD_TESTS OFF)

add_subdirectory(buf/validate)
add_subdirectory(third_party/protobuf)
add_subdirectory(third_party/re2)
