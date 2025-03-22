# protovalidate-cc CMake Usage

protovalidate-cc comes with a CMake build script. You can use this to embed
protovalidate-cc into your existing CMake project or install protovalidate-cc
system-wide.

## CMake Flags

protovalidate-cc provides the following options:

| Option | Default Value | Description |
| ------ | ------------- | ----------- |
| `PROTOVALIDATE_CC_ENABLE_VENDORING` | `ON` | Controls whether protovalidate-cc will attempt to fetch absl, protobuf or re2 remotely when local copies are not available. |
| `PROTOVALIDATE_CC_ENABLE_INSTALL` | `ON` for standalone builds, `OFF` when embedding | Controls whether protovalidate-cc will set up install targets. When used as an embedded project, this will be off by default. |
| `PROTOVALIDATE_CC_ENABLE_TESTS` | `ON` for standalone builds, `OFF` when embedding | Controls whether protovalidate-cc will compile its own tests. When this is `ON`, the Google Test and Google Benchmark will also be loaded. |
| `PROTOVALIDATE_CC_ENABLE_CONFORMANCE` | `ON` for standalone builds, `OFF` when embedding | Controls whether protovalidate-cc will build the conformance test runner, to test with the protovalidate conformance test suite. |

An option can be used with CMake flags during the configure stage, e.g. to
disable vendoring:

```console
$ cmake -B .build -S . -DPROTOVALIDATE_CC_ENABLE_VENDORING=OFF
```

## Dependencies

> [!WARNING]
> **cel-cpp, and by extension, protovalidate-cc, require relatively recent
> versions of dependencies like absl and protobuf**. The versions installed on
> your system or embedded into your projects may not be new enough. While we do
> our best to try to throw reasonable error messages when a dependency is too
> old, you may run into strange errors if your dependencies are not compatible.
>
> Known compatible versions of dependencies can be found by looking at the
> dependency table below. Additionally, you can individually disable usage of
> system dependencies by setting CMake options like
> `CMAKE_DISABLE_FIND_PACKAGE_absl` and `CMAKE_DISABLE_FIND_PACKAGE_Protobuf`.

When built with CMake, protovalidate-cc has the following dependencies:

| Dependency                                                 | Known Compatible Version | Flag to Disable System Version               |
| ---------------------------------------------------------- | ------------------------ | -------------------------------------------- |
| [Abseil](https://abseil.io/)                               | 20240722                 | `-DCMAKE_DISABLE_FIND_PACKAGE_absl=TRUE`     |
| [Protocol Buffers](https://protobuf.dev/)                  | v29.2                    | `-DCMAKE_DISABLE_FIND_PACKAGE_Protobuf=TRUE` |
| [re2](https://github.com/google/re2)                       | 2024-02-01               | `-DCMAKE_DISABLE_FIND_PACKAGE_re2=TRUE`      |
| [ANTLR 4](https://www.antlr.org/)                          | (Always vendored)        |                                              |
| [googleapis](https://github.com/googleapis/googleapis.git) | (Always vendored)        |                                              |
| [cel-cpp](https://github.com/google/cel-cpp)               | (Always vendored)        |                                              |
| [protovalidate](https://github.com/bufbuild/protovalidate) | (Always vendored)        |                                              |

You can disable vendoring for any package that supports being loaded from the
system by using the CMake flag `-DPROTOVALIDATE_CC_ENABLE_VENDORING=OFF`. System
packages are always preferred when available, but this will ensure that
protovalidate-cc does not use vendoring (except for the libraries that are
always vendored.)

Depending on your project setup, these can be sourced in different ways:

- Abseil, Protocol Buffers and re2 can link to existing targets. If you already
  have Abseil, Protocol Buffers, or re2 added to your CMake project prior to
  the `FetchContent_MakeAvailable` for protovalidate-cc, the existing targets
  will be used. Please note that in this case, you need to ensure that the
  versions of Abseil, Protocol Buffers, and re2 are supported by cel-cpp and
  protovalidate-cc.

- ANTLR 4 can not be included from the system as cel-cpp depends on the runtime
  library being compiled with a specific flag (`ANTLR4CPP_USING_ABSEIL`). It
  will always be fetched using `FetchContent`. In addition, a version of the
  ANTLR 4 compiler will be downloaded from the Internet, and ran using `java`
  during the compilation process (naturally, this requires a JRE installation.)
  You can override this behavior by setting `FETCHCONTENT_SOURCE_DIR_ANTLR4` and
  `PROTOVALIDATE_CC_ANTLR4_JAR_LOCATION` to the location of a copy of the ANTLR
  4 source code and a compiled ANTLR 4 compiler JAR before running
  `FetchContent_MakeAvailable(protovalidate_cc)`.

- A copy of googleapis is always fetched for cel-cpp. You can override this
  behavior by setting `FETCHCONTENT_SOURCE_DIR_GOOGLEAPIS` to a copy of
  googleapis prior to running `FetchContent_MakeAvailable(protovalidate_cc)`.

- cel-cpp does not have CMakeLists upstream and is not currently packaged for
  system-wide installation, so it is always built as a static library locally.
  You can override the externally-fetched code by setting
  `FETCHCONTENT_SOURCE_DIR_CEL_CPP` prior to running
  `FetchContent_MakeAvailable(protovalidate_cc)`.

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

> [!IMPORTANT]
> **The order of declarations in your CMakeLists.txt file matters!** If you load
> protovalidate-cc before other libraries, the copy of re2, Protocol Buffers or
> Abseil that it loads may influence other CMake dependencies. Likewise, if you
> load protovalidate-cc *after* other libraries, it may pick up Protocol Buffers
> or Abseil from other dependencies. For more predictable behavior, you can
> grab those dependencies directly first, before including libraries that
> require them.

### Example

There is an example of how to use protovalidate-cc in the [example](./example)
directory. You can build and run it like this:

```console
$ cd <protovalidate-cc>/cmake/example
$ cmake -B .build -S .
$ cmake --build .build
$ ./.build/example
```
