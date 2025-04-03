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

#include "buf/validate/internal/lib/ipv4.h"

#include <array>
#include <cstdint>

#include "buf/validate/internal/lib/parser_common.h"

namespace buf::validate::internal::lib {

namespace {

struct IPv4Parser : ParserCommon, public IPv4Prefix {
  bool consumePrefixLength() { return consumeDecimalNumber<uint8_t, bits_count>(prefixLength); }

  bool consumeAddressPart() {
    std::array<uint8_t, 4> octets;
    if (!consumeDecimalOctet(octets[0]) || !consumeDot() || //
        !consumeDecimalOctet(octets[1]) || !consumeDot() || //
        !consumeDecimalOctet(octets[2]) || !consumeDot() || //
        !consumeDecimalOctet(octets[3])) {
      return false;
    }
    bits |= static_cast<uint32_t>(octets[0]) << 24;
    bits |= static_cast<uint32_t>(octets[1]) << 16;
    bits |= static_cast<uint32_t>(octets[2]) << 8;
    bits |= static_cast<uint32_t>(octets[3]);
    return true;
  }

  bool parseAddress() { return consumeAddressPart() && str.empty(); }

  bool parsePrefix() {
    return consumeAddressPart() && consumeSlash() && consumePrefixLength() && str.empty();
  }
};

} // namespace

std::optional<IPv4Address> parseIPv4Address(std::string_view str) {
  IPv4Parser address{str};
  if (!address.parseAddress()) {
    return std::nullopt;
  }
  return address;
}

std::optional<IPv4Prefix> parseIPv4Prefix(std::string_view str) {
  IPv4Parser prefix{str};
  if (!prefix.parsePrefix()) {
    return std::nullopt;
  }
  return prefix;
}

} // namespace buf::validate::internal::lib
