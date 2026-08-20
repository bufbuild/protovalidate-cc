// Copyright 2023-2026 Buf Technologies, Inc.
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

#ifdef _WIN32
#include <fcntl.h>
#endif

#include <iostream>

#include "buf/validate/conformance/harness/harness.pb.h"
#include "buf/validate/conformance/runner.h"

#ifdef _WIN32
#include "google/protobuf/io/io_win32.h"
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
  // The request and response are binary protobuf over stdin/stdout,
  // but Windows defaults the streams to text mode.
  google::protobuf::io::win32::setmode(STDIN_FILENO, _O_BINARY);
  google::protobuf::io::win32::setmode(STDOUT_FILENO, _O_BINARY);
#endif
  google::protobuf::DescriptorPool descriptorPool{
      google::protobuf::DescriptorPool::generated_pool()};
  buf::validate::conformance::harness::TestConformanceRequest request;
  if (!request.ParseFromIstream(&std::cin)) {
    std::cerr << "failed to parse conformance request from stdin" << std::endl;
    return 1;
  }
  for (const auto& file : request.fdset().file()) {
    descriptorPool.BuildFile(file);
  }
  buf::validate::conformance::TestRunner runner{&descriptorPool};
  auto response = runner.runTest(request);
  response.SerializeToOstream(&std::cout);
  return 0;
}
