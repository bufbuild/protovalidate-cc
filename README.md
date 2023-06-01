# [![The Buf logo](.github/buf-logo.svg)][buf] protovalidate-cc

[![CI](https://github.com/bufbuild/protovalidate-cc/actions/workflows/ci.yaml/badge.svg)](https://github.com/bufbuild/protovalidate-cc/actions/workflows/ci.yaml)
[![Conformance](https://github.com/bufbuild/protovalidate-cc/actions/workflows/conformance.yaml/badge.svg)](https://github.com/bufbuild/protovalidate-cc/actions/workflows/conformance.yaml)
[![BSR](https://img.shields.io/badge/BSR-Module-0C65EC)][buf-mod]

`protovalidate-cc` is the C++ language implementation
of [`protovalidate`](https://github.com/bufbuild/protovalidate) designed
to validate Protobuf messages at runtime based on user-defined validation constraints.
Powered by Google's Common Expression Language ([CEL](https://github.com/google/cel-spec)), it provides a
flexible and efficient foundation for defining and evaluating custom validation
rules.
The primary goal of `protovalidate` is to help developers ensure data
consistency and integrity across the network without requiring generated code.

## The `protovalidate` project

Head over to the core [`protovalidate`](https://github.com/bufbuild/protovalidate/) repository for:

- [The API definition](https://github.com/bufbuild/protovalidate/tree/main/proto/protovalidate/buf/validate/validate.proto): used to describe validation constraints
- [Documentation](https://github.com/bufbuild/protovalidate/tree/main/docs): how to apply `protovalidate` effectively
- [Migration tooling](https://github.com/bufbuild/protovalidate/tree/main/docs/migrate.md): incrementally migrate from `protoc-gen-validate`
- [Conformance testing utilities](https://github.com/bufbuild/protovalidate/tree/main/docs/conformance.md): for acceptance testing of `protovalidate` implementations

Other `protovalidate` runtime implementations coming soon include:

- Java: `protovalidate-java`
- Python: `protovalidate-py`
- TypeScript: `protovalidate-ts`

## Installation

To install `protovalidate-cc`, clone the repository and build the project:

```shell
git clone https://github.com/bufbuild/protovalidate-cc.git
cd protovalidate-cc
make build
```

Remember to always check for the latest version of `protovalidate-cc` on the
project's [GitHub releases page](https://github.com/bufbuild/protovalidate-cc/releases)
to ensure you're using the most up-to-date version.

## Usage

### Implementing validation constraints

Validation constraints are defined directly within `.proto` files.
Documentation for adding constraints can be found in the `protovalidate` project
[README](https://github.com/bufbuild/protovalidate) and its [comprehensive docs](https://github.com/bufbuild/protovalidate/tree/main/docs).

```protobuf
syntax = "proto3";

package my.package;

import "google/protobuf/timestamp.proto";
import "buf/validate/validate.proto";

message Transaction {
  uint64 id = 1 [(buf.validate.field).uint64.gt = 999];
  google.protobuf.Timestamp purchase_date = 2;
  google.protobuf.Timestamp delivery_date = 3;
  
  string price = 4 [(buf.validate.field).cel = {
   

 id: "transaction.price",
    message: "price must be positive and include a valid currency symbol ($ or £)",
    expression: "(this.startsWith('$') || this.startsWith('£')) && double(this.substring(1)) > 0"
  }];

  option (buf.validate.message).cel = {
    id: "transaction.delivery_date",
    message: "delivery date must be after purchase date",
    expression: "this.delivery_date > this.purchase_date"
  };
}
```

### Example

In your C++ code, include the header file and use the `Validate` function to validate your messages.

```cpp
#include <iostream>
#include "path/to/generated/protos/transaction.pb.h"
#include "bufbuild/protovalidate/cc/validator.h"

int main() {
  my::package::Transaction transaction;
  transaction.set_id(1234);
  transaction.set_price("$5.67");

  google::protobuf::Timestamp* purchase_date = transaction.mutable_purchase_date();
  google::protobuf::Timestamp* delivery_date = transaction.mutable_delivery_date();
  // set time for purchase_date and delivery_date

  bufbuild::protovalidate::cc::Validator validator;
  std::string error_message;
  if (!validator.Validate(transaction, &error_message)) {
    std::cout << "validation failed: " << error_message << std::endl;
  } else {
    std::cout << "validation succeeded" << std::endl;
  }
  return 0;
}
```

## Performance

Benchmarking tools are provided to test a variety of use-cases. Due to the inherent differences between languages and their runtimes, performance may vary from the Go implementation. However, the objective is to keep validation times as low as possible, ideally under a microsecond for a majority of cases after the initial warm-up period.

### Ecosystem

- [`protovalidate`](https://github.com/bufbuild/protovalidate) core repository
- [Buf][buf]
- [CEL Spec][cel-spec]

## Legal

Offered under the [Apache 2 license][license].

[license]: LICENSE
[buf]: https://buf.build
[buf-mod]: https://buf.build/bufbuild/protovalidate
[cel-spec]: https://github.com/google/cel-spec
