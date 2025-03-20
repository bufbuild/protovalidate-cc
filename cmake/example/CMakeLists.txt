cmake_minimum_required(VERSION 3.24)

project(protovalidate-cc-cmake-example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# In a real project, you'll probably want to use FetchContent here. See the
# README at cmake/README.md for more information on how to embed
# protovalidate-cc.
add_subdirectory(../.. ${CMAKE_CURRENT_BINARY_DIR}/protovalidate-cc)

# This line lets us access the copy of protovalidate fetched by protovalidate-cc
# so we can import from it.
FetchContent_GetProperties(protovalidate)

# Generate our protobuf code. For convenience, protovalidate exports the
# variable PROTOBUF_IMPORT_PATH to the parent scope; it contains the import path
# needed to access the well-known types, regardless of how protobuf is being
# linked to.
add_library(example_proto OBJECT example.proto)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen)
protobuf_generate(
    TARGET example_proto
    LANGUAGE cpp
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen
    IMPORT_DIRS ${protovalidate_SOURCE_DIR}/proto/protovalidate
                ${PROTOBUF_IMPORT_PATH}
                ${CMAKE_CURRENT_SOURCE_DIR}
)

# Our generated code will depend on protovalidate.
target_link_libraries(example_proto PUBLIC protovalidate_cc::protovalidate_cc)

# Ensure code linking to this target can import the generated headers.
target_include_directories(example_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/gen)

# Finally, compile and link our example executable.
add_executable(example main.cc)
target_link_libraries(example protovalidate_cc::protovalidate_cc example_proto)
