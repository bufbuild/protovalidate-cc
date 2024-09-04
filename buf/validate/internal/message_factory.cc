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

#include "buf/validate/internal/message_factory.h"

namespace buf::validate::internal {

bool Reparse(
    MessageFactory& messageFactory,
    const google::protobuf::Message& from,
    google::protobuf::Message* to) {
  std::string serialized;
  from.SerializeToString(&serialized);
  google::protobuf::io::CodedInputStream input(
      reinterpret_cast<const uint8_t*>(serialized.c_str()), static_cast<int>(serialized.size()));
  input.SetExtensionRegistry(messageFactory.descriptorPool(), messageFactory.messageFactory());
  return to->ParseFromCodedStream(&input);
}

} // namespace buf::validate::internal
