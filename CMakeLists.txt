cmake_minimum_required(VERSION 3.13)
project(protovalidate-cc)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  absl
  URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20230802.0.zip
)
set(ABSL_PROPAGATE_CXX_STD ON)
FetchContent_MakeAvailable(absl)

FetchContent_Declare(
  protobuf
  URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v24.2.zip
)
set(protobuf_BUILD_TESTS OFF)
set(protobuf_INSTALL OFF)
FetchContent_MakeAvailable(protobuf)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  cel-cpp
  URL https://github.com/google/cel-cpp/archive/da0aba702f44a41ec6d2eb4bbf6a9f01efc2746d.zip
  PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/CMake/cel-cpp.patch
)
FetchContent_MakeAvailable(cel-cpp)

# Add source directory as include directory.
include_directories(${PROJECT_SOURCE_DIR})
# Add the generated protobuf include directory.
include_directories(${PROJECT_SOURCE_DIR}/gen)

# Find all the generated protobuf files.
file(GLOB_RECURSE PV_GEN_SRC ${PROJECT_SOURCE_DIR}/gen/*.pb.cc)

# Add the generated protobuf files to the library.
add_library(protovalidate_gen_pb ${PV_GEN_SRC})
target_link_libraries(
  protovalidate_gen_pb
  protobuf::libprotobuf
)

# Find all the conformance files.
file(GLOB_RECURSE PV_CONFORMANCE_FILES ${PROJECT_SOURCE_DIR}/buf/validate/conformance/*.cc)
# Find all the test files.
file(GLOB_RECURSE PV_TEST_FILES ${PROJECT_SOURCE_DIR}/buf/validate/*_test.cc)

# Find all the source files.
file(GLOB_RECURSE PV_SOURCE_FILES ${PROJECT_SOURCE_DIR}/buf/validate/*.cc)
# Remove the test files from the source files.
list(REMOVE_ITEM PV_SOURCE_FILES ${PV_TEST_FILES})
# Remove the conformance test files from the source files.
list(REMOVE_ITEM PV_SOURCE_FILES ${PV_CONFORMANCE_FILES})

# Create the library.
add_library(protovalidate ${PV_SOURCE_FILES})

# Link the library with the cel-cpp.
target_link_libraries(
  protovalidate
  cel
  protovalidate_gen_pb
)

target_include_directories(
  protovalidate
  PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/gen
)
