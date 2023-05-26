#include "buf/validate/conformance/harness/harness.pb.h"
#include "buf/validate/conformance/runner.h"

int main(int argc, char** argv) {
  buf::validate::conformance::TestRunner runner;
  buf::validate::conformance::harness::TestConformanceRequest request;
  request.ParseFromIstream(&std::cin);
  google::protobuf::DescriptorPool descriptorPool;
  for (const auto& file : request.fdset().file()) {
    descriptorPool.BuildFile(file);
  }
  auto response = runner.runTest(request, &descriptorPool);
  response.SerializeToOstream(&std::cout);
  return 0;
}
