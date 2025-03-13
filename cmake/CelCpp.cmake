include(FetchContent)
include(MakePatchCommand)
include(Re2)

# Fetch cel-cpp
set(CEL_CPP_PATCHES
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/cel_cpp/0001-Allow-message-field-access-using-index-operator.patch
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/cel_cpp/0002-Add-missing-include-for-absl-StrCat.patch
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/cel_cpp/0003-Remove-unnecessary-dependency-on-cel_proto_wrap_util.patch
)
MakePatchCommand(CEL_CPP_PATCH_COMMAND "${CEL_CPP_PATCHES}")
message(STATUS "protovalidate-cc: Fetching cel-cpp")

FetchContent_Declare(cel_cpp
    URL
        ${PROTOVALIDATE_CC_CEL_CPP_URLS}
    URL_HASH
        SHA256=${PROTOVALIDATE_CC_CEL_CPP_SHA256}
    PATCH_COMMAND
        ${CEL_CPP_PATCH_COMMAND}
    SOURCE_SUBDIR
        nonexistant
)

FetchContent_MakeAvailable(cel_cpp)

file(
    COPY_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cel-cpp/CMakeLists.txt
        ${cel_cpp_SOURCE_DIR}/CMakeLists.txt
)

message(STATUS "Added ${cel_cpp_SOURCE_DIR}/CMakeLists.txt")
add_subdirectory(${cel_cpp_SOURCE_DIR} ${cel_cpp_BINARY_DIR})
