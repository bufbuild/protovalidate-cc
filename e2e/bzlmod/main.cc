// Copyright 2023-2025 Buf Technologies, Inc.
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

#include <iostream>

#include "buf/validate/validator.h"
#include "example.pb.h"

int main(int argc, char** argv) {
  google::protobuf::Arena arena;
  TestMessage test_message;

  // Construct a validator instance
  auto factory_or = buf::validate::ValidatorFactory::New();
  if (!factory_or.ok()) {
    std::cerr << "Failed to build factory: " << factory_or.status() << std::endl;
    return 1;
  }
  auto validator = factory_or.value()->NewValidator(&arena, false);

  // Perform validation
  auto result_or = validator.Validate(test_message);
  if (!result_or.ok()) {
    std::cerr << "Failed to validate message: " << result_or.status() << std::endl;
    return 1;
  }

  // Print validation results
  std::cerr << "Violation Count: " << result_or->violations_size() << std::endl;
  for (int i = 0; i < result_or->violations_size(); i++) {
    auto violation = result_or->violations(i).proto();
    std::cerr << "Violation " << i + 1 << ": " << violation.message() << std::endl;
  }
  return 0;
}
