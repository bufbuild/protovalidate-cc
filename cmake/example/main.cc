// Copyright 2023 Buf Technologies, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "buf/validate/validator.h"
#include "example.pb.h"

int main(int argc, char** argv) {
  example::v1::User user;
  user.set_id(5);

  google::protobuf::Arena arena;
  auto factory = buf::validate::ValidatorFactory::New().value();
  auto validator = factory->NewValidator(&arena, false);
  
  auto results = validator.Validate(user).value();
  if (results.violations_size() > 0) {
    std::cout << "validation failed" << std::endl;
  } else {
    std::cout << "validation succeeded" << std::endl;
  }

  for (int i = 0; i < results.violations_size(); i++) {
    auto p = results.violations(i);

    // Print the validation message.
    std::cout << p.proto().message() << std::endl;

    // Print the entire ConstraintViolation to see its structure.
    std::cout << p.proto().DebugString() << std::endl;
  }
}
