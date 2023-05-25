#pragma once

#include "buf/validate/conformance/harness/harness.pb.h"
#include "buf/validate/validator.h"
#include "google/protobuf/dynamic_message.h"

namespace buf::validate::conformance {

class TestRunner {
 public:
  explicit TestRunner() : validatorFactory_(ValidatorFactory::New().value()) {}

  harness::TestConformanceResponse runTest(const harness::TestConformanceRequest& request);
  harness::TestResult runTestCase(
      const google::protobuf::Descriptor* desc, const google::protobuf::Any& dyn);
  harness::TestResult runTestCase(const google::protobuf::Message& message);

 private:
  google::protobuf::DynamicMessageFactory messageFactory_;
  std::unique_ptr<ValidatorFactory> validatorFactory_;
  google::protobuf::Arena arena_;
};

} // namespace buf::validate::conformance