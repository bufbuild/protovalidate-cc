# protovalidate-cc CMake Usage

protovalidate-cc comes with a CMake build script. You can use this to embed
protovalidate-cc into your existing CMake project or install protovalidate-cc
system-wide.

## Embedding

protovalidate-cc requires C++17 or higher. Near the top of your CMakeLists,
ensure you are setting the C++ standard:

```
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

You can use the following snippet to include protovalidate-cc.

```
FetchContent_Declare(
    protovalidate_cc
    GIT_REPOSITORY "https://github.com/bufbuild/protovalidate-cc.git"
    GIT_TAG "v0.5.0"
)
FetchContent_MakeAvailable(protovalidate_cc)
```

When built with CMake, protovalidate-cc has the following external dependencies:

- [Abseil](https://abseil.io/)
- [Protocol Buffers](https://protobuf.dev/)
- [re2](https://github.com/google/re2)
- [ANTLR 4](https://www.antlr.org/)
- [googleapis](https://github.com/googleapis/googleapis.git)
- [cel-cpp](https://github.com/google/cel-cpp)
- [protovalidate](https://github.com/bufbuild/protovalidate)

Depending on your project setup, these can be sourced in different ways:

- Abseil, Protocol Buffers and re2 can link to external copies. If you already
  have Abseil, Protocol Buffers, or re2 added to your CMake project *prior to
  the `FetchContent_MakeAvailable` for protovalidate-cc*, the existing targets
  will be used. Please note that in this case, you need to ensure that the
  versions of Abseil, Protocol Buffers, and re2 are supported by cel-cpp and
  protovalidate-cc. The versions specified in
  [https://github.com/google/cel-cpp/blob/v0.10.0/bazel/deps.bzl](cel-cpp) are
  known to compile and function correctly.

- ANTLR 4 can not be included externally as cel-cpp depends on the runtime
  library being compiled with a specific flag (`ANTLR4CPP_USING_ABSEIL`). It
  will always be fetched using `FetchContent`. In addition, a version of the
  ANTLR 4 compiler will be downloaded from the Internet, and ran using `java`
  during the compilation process (naturally, this requires a JRE installation.)
  You can override this behavior by setting `FETCHCONTENT_SOURCE_DIR_ANTLR4` and
  `ANTLR4_JAR_LOCATION` to the location of a copy of the ANTLR 4 source code and
  a compiled ANTLR 4 compiler JAR before running
  `FetchContent_MakeAvailable(protovalidate_cc)`.

- A copy of googleapis is always fetched for cel-cpp. You can override this
  behavior by setting `FETCHCONTENT_SOURCE_DIR_GOOGLEAPIS` to a copy of
  googleapis prior to running `FetchContent_MakeAvailable(protovalidate_cc)`.

- cel-cpp does not have CMakeLists upstream and is not currently packaged for
  system-wide installation, so it is always built as a static library locally.
  You can override the externally-fetched code by setting
  `FETCHCONTENT_SOURCE_DIR_CEL_CPP` prior to running
  `FetchContent_MakeAvailable(protovalidate_cc)`.

> [!IMPORTANT]
> **The order of declarations in your CMakeLists.txt file matters!** If you load
> protovalidate-cc before other libraries, the copy of re2, Protocol Buffers or
> Abseil that it loads may influence other CMake dependencies. Likewise, if you
> load protovalidate-cc *after* other libraries, it may pick up Protocol Buffers
> or Abseil from external dependencies. For more predictable behavior, you can
> grab those external dependencies directly first, before including libraries
> that require them.

### Example

There is an example of how to use protovalidate-cc in the [example](./example)
directory. You can build and run it like this:

```console
$ cmake -B .build -S .
$ cmake --build .build
$ ./.build/example
```
