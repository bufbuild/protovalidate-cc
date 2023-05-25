#include "buf/validate/conformance/harness/harness.pb.h"
#include "buf/validate/conformance/runner.h"

int main(int argc, char** argv) {
  buf::validate::conformance::TestRunner runner;
  buf::validate::conformance::harness::TestConformanceRequest request;
  request.ParseFromIstream(&std::cin);
  auto response = runner.runTest(request);
  response.SerializeToOstream(&std::cout);
  return 0;
}
