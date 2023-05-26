#include "buf/validate/conformance/runner.h"

#include "buf/validate/validator.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::conformance {

harness::TestConformanceResponse TestRunner::runTest(
    const harness::TestConformanceRequest& request,
    google::protobuf::DescriptorPool* descriptorPool) {
  harness::TestConformanceResponse response;

  // Create a descriptor pool, and populate it.
  for (const auto& file : request.fdset().file()) {
    descriptorPool->BuildFile(file);
  }

  for (const auto& tc : request.cases()) {
    auto& result = response.mutable_results()->operator[](tc.first);
    const google::protobuf::Any& dyn = tc.second;
    // Get the type name from the type url
    auto pos = dyn.type_url().find_last_of('/');
    if (pos == std::string::npos) {
      *result.mutable_unexpected_error() = "could not parse type url " + dyn.type_url();
      continue;
    }
    const auto* desc = descriptorPool->FindMessageTypeByName(dyn.type_url().substr(pos + 1));
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
  auto violations_or = validator->Validate(message);
  if (!violations_or.ok()) {
    *result.mutable_runtime_error() = violations_or.status().message();
  } else if (violations_or.value().violations_size() > 0) {
    *result.mutable_validation_error() = std::move(violations_or).value();
  } else {
    result.set_success(true);
  }
  return result;
}

} // namespace buf::validate::conformance
