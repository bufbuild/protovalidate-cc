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

#include "buf/validate/conformance/runner.h"

#include "buf/validate/validator.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::conformance {

harness::TestConformanceResponse TestRunner::runTest(
    const harness::TestConformanceRequest& request) {
  harness::TestConformanceResponse response;
  for (const auto& tc : request.cases()) {
    auto& result = response.mutable_results()->operator[](tc.first);
    const google::protobuf::Any& dyn = tc.second;
    // Get the type name from the type url
    auto pos = dyn.type_url().find_last_of('/');
    if (pos == std::string::npos) {
      *result.mutable_unexpected_error() = "could not parse type url " + dyn.type_url();
      continue;
    }
    const auto* desc = descriptorPool_->FindMessageTypeByName(dyn.type_url().substr(pos + 1));
    if (desc == nullptr) {
      *result.mutable_unexpected_error() = "could not find descriptor for type " + dyn.type_url();
    } else {
      result = runTestCase(desc, dyn);
    }
  }
  return response;
}

harness::TestResult TestRunner::runTestCase(
    const google::protobuf::Descriptor* desc, const google::protobuf::Any& dyn) {
  std::unique_ptr<google::protobuf::Message> message(messageFactory_.GetPrototype(desc)->New());
  if (!dyn.UnpackTo(message.get())) {
    harness::TestResult result;
    *result.mutable_unexpected_error() = "could not unpack message of type " + dyn.type_url();
    return result;
  }
  return runTestCase(*message);
}

harness::TestResult TestRunner::runTestCase(const google::protobuf::Message& message) {
  harness::TestResult result;
  auto validator = validatorFactory_->NewValidator(&arena_, false);
  auto violations_or = validator.Validate(message);
  if (!violations_or.ok()) {
    switch (violations_or.status().code()) {
      case absl::StatusCode::kInvalidArgument:
        *result.mutable_runtime_error() = violations_or.status().message();
        break;
      case absl::StatusCode::kFailedPrecondition:
        *result.mutable_compilation_error() = violations_or.status().message();
        break;
      default:
        *result.mutable_unexpected_error() = violations_or.status().message();
        break;
    }
  } else if (violations_or.value().violations_size() > 0) {
    *result.mutable_validation_error() = violations_or->proto();
  } else {
    result.set_success(true);
  }
  return result;
}

} // namespace buf::validate::conformance
