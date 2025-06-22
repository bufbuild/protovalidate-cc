[![The Buf logo](.github/buf-logo.svg)][buf] 

# protovalidate-cc

[![CI](https://github.com/bufbuild/protovalidate-cc/actions/workflows/ci.yaml/badge.svg)](https://github.com/bufbuild/protovalidate-cc/actions/workflows/ci.yaml)
[![Conformance](https://github.com/bufbuild/protovalidate-cc/actions/workflows/conformance.yaml/badge.svg)](https://github.com/bufbuild/protovalidate-cc/actions/workflows/conformance.yaml)
[![BSR](https://img.shields.io/badge/BSR-Module-0C65EC)][buf-mod]

[Protovalidate][protovalidate] provides standard annotations to validate common constraints on messages and fields, as well as the ability to use [CEL][cel] to write custom constraints. It's the next generation of [protoc-gen-validate][protoc-gen-validate], the only widely used validation library for Protobuf.

With Protovalidate, you can annotate your Protobuf messages with both standard and custom validation rules:

```protobuf
syntax = "proto3";

package banking.v1;

import "buf/validate/validate.proto";

message MoneyTransfer {
  string to_account_id = 1 [
    // Standard rule: `to_account_id` must be a UUID
    (buf.validate.field).string.uuid = true
  ];

  string from_account_id = 2 [
    // Standard rule: `from_account_id` must be a UUID
    (buf.validate.field).string.uuid = true
  ];

  // Custom rule: `to_account_id` and `from_account_id` can't be the same.
  option (buf.validate.message).cel = {
    id: "to_account_id.not.from_account_id"
    message: "to_account_id and from_account_id should not be the same value"
    expression: "this.to_account_id != this.from_account_id"
  };
}
```

Once you've added `protovalidate-cc` to your project, validation is idiomatic C++:

```c++
std::unique_ptr<buf::validate::ValidatorFactory> factory =
  buf::validate::ValidatorFactory::New().value();
google::protobuf::Arena arena;
buf::validate::Validator validator = factory->NewValidator(&arena);
buf::validate::Violations results = validator.Validate(moneyTransfer).value();
if (results.violations_size() > 0) {
    // Handle failure
}
```

## Installation

### Building from source

To install `protovalidate-cc`, clone the repository and build the project:

```shell
git clone https://github.com/bufbuild/protovalidate-cc.git
cd protovalidate-cc
make build
```

Remember to always check for the latest version of `protovalidate-cc` on the project's [GitHub releases page](https://github.com/bufbuild/protovalidate-cc/releases) to ensure you're using the most up-to-date version.

### Bazel external repository

#### Workspace

To use `protovalidate-cc` as an external Bazel repository, add the following to the `WORKSPACE` file:

```bzl
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_github_bufbuild_protovalidate_cc",
    sha256 = ...,
    strip_prefix = "protovalidate-cc-{version}",
    urls = [
        "https://github.com/bufbuild/protovalidate-cc/releases/download/${TAG}/protovalidate-cc-${TAG:1}.tar.gz",
    ],
)

load("@com_github_bufbuild_protovalidate_cc//bazel:deps.bzl", "protovalidate_cc_dependencies")

protovalidate_cc_dependencies()
```

Then add a dependency to a `cc_library` or `cc_binary` target:

```bzl
cc_library(
    ...
    deps = [
        "@com_github_bufbuild_protovalidate_cc//buf/validate:validator",
        ...
    ]
)
```

#### Bzlmod

To use `protovalidate-cc` as an external dependency for bzlmod, add the following to the `MODULE.bazel`:

```bzl
module(
    name = "my-module",
    version = "1.0",
)

bazel_dep(name = "cel-cpp", repo_name = "com_google_cel_cpp", version="0.11.0")
bazel_dep(name = "protovalidate-cc", version = "1.0.0-rc.2")
```

And the following to your `BUILD.bazel`:

```bzl

cc_binary(
   ...
   deps = [ ..., "@protovalidate-cc//buf/validate:validator", ...]
   ...
)
```

## Documentation

Comprehensive documentation for Protovalidate is available in [Buf's documentation library][protovalidate].

Highlights for C++ developers include:

* The [developer quickstart][quickstart]
* A [migration guide for protoc-gen-validate][migration-guide] users

## Additional Languages and Repositories

Protovalidate isn't just for C++! You might be interested in sibling repositories for other languages:

- [`protovalidate-go`][pv-go] (Go)
- [`protovalidate-java`][pv-java] (Java)
- [`protovalidate-python`][pv-python] (Python)
- [`protovalidate-es`][pv-es] (TypeScript and JavaScript)

Additionally, [protovalidate's core repository](https://github.com/bufbuild/protovalidate) provides:

- [Protovalidate's Protobuf API][validate-proto]
- [Conformance testing utilities][conformance] for acceptance testing of `protovalidate` implementations

## Contribution

We genuinely appreciate any help! If you'd like to contribute, check out these resources:

- [Contributing Guidelines][contributing]: Guidelines to make your contribution process straightforward and meaningful
- [Conformance testing utilities](https://github.com/bufbuild/protovalidate/tree/main/docs/conformance.md): Utilities providing acceptance testing of `protovalidate` implementations

## Related Sites

- [Buf][buf]: Enterprise-grade Kafka and gRPC for the modern age
- [Common Expression Language (CEL)][cel]: The open-source technology at the core of Protovalidate

## Legal

Offered under the [Apache 2 license][license].

[buf]: https://buf.build
[cel]: https://cel.dev

[pv-go]: https://github.com/bufbuild/protovalidate-go
[pv-java]: https://github.com/bufbuild/protovalidate-java
[pv-python]: https://github.com/bufbuild/protovalidate-python
[pv-cc]: https://github.com/bufbuild/protovalidate-cc
[pv-es]: https://github.com/bufbuild/protovalidate-es

[buf-mod]: https://buf.build/bufbuild/protovalidate
[license]: LICENSE
[contributing]: .github/CONTRIBUTING.md

[protoc-gen-validate]: https://github.com/bufbuild/protoc-gen-validate

[protovalidate]: https://buf.build/docs/protovalidate/
[quickstart]: https://buf.build/docs/protovalidate/quickstart/
[connect-go]: https://buf.build/docs/protovalidate/quickstart/connect-go/
[grpc-go]: https://buf.build/docs/protovalidate/quickstart/grpc-go/
[grpc-java]: https://buf.build/docs/protovalidate/quickstart/grpc-java/
[grpc-python]: https://buf.build/docs/protovalidate/quickstart/grpc-python/
[migration-guide]: https://buf.build/docs/migration-guides/migrate-from-protoc-gen-validate/
[pkg-go]: https://pkg.go.dev/github.com/bufbuild/protovalidate-go

[validate-proto]: https://buf.build/bufbuild/protovalidate/docs/main:buf.validate
[conformance]: https://github.com/bufbuild/protovalidate/blob/main/docs/conformance.md
[examples]: https://github.com/bufbuild/protovalidate/tree/main/examples
[migrate]: https://buf.build/docs/migration-guides/migrate-from-protoc-gen-validate/
