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

#include <algorithm>
#include <string_view>

namespace buf::validate::internal::lib {

namespace {

constexpr unsigned calculateDecimalDigits(unsigned value) {
  unsigned i = 0;
  for (; value != 0; i++) {
    value /= 10;
  }
  return std::max(1U, i);
}

constexpr unsigned calculateHexadecimalDigits(unsigned value) {
  unsigned i = 0;
  for (; value != 0; i++) {
    value >>= 4;
  }
  return std::max(1U, i);
}

bool decimalDigitValue(char c, int& outputValue) {
  if ('0' <= c && c <= '9') {
    outputValue = c - '0';
    return true;
  }
  return false;
}

bool hexadecimalDigitValue(char c, int& outputValue) {
  if ('0' <= c && c <= '9') {
    outputValue = c - '0';
    return true;
  } else if ('a' <= c && c <= 'f') {
    outputValue = c - 'a' + 10;
    return true;
  } else if ('A' <= c && c <= 'F') {
    outputValue = c - 'A' + 10;
    return true;
  }
  return false;
}

struct ParserCommon {
  std::string_view str;

  ParserCommon(std::string_view str) : str{str} {}

  template <char c>
  bool consumeCharLiteral() {
    if (str.empty() || str[0] != c) {
      return false;
    }
    str = str.substr(1);
    return true;
  }

  bool consumeDot() { return consumeCharLiteral<'.'>(); }
  bool consumeSlash() { return consumeCharLiteral<'/'>(); }
  bool consumeColon() { return consumeCharLiteral<':'>(); }
  bool consumePercent() { return consumeCharLiteral<'%'>(); }

  bool consumeDoubleColon() {
    if (str.length() < 2 || str[0] != ':' || str[1] != ':') {
      return false;
    }
    str = str.substr(2);
    return true;
  }

  /**
   * Consumes an unsigned decimal number, storing the value in outputValue.
   * Leading zeros are disallowed.
   */
  template <typename T, T maxValue>
  bool consumeDecimalNumber(T& outputValue) {
    constexpr unsigned maxDigits = calculateDecimalDigits(static_cast<unsigned>(maxValue));
    int digitValue;
    if (str.empty() || !decimalDigitValue(str[0], digitValue)) {
      // Invalid: too short
      return false;
    }
    int i = 1;
    unsigned value = digitValue;
    if (str.length() > i && decimalDigitValue(str[i], digitValue)) {
      if (value == 0) {
        // Invalid: leading zero
        return false;
      }
      do {
        i++;
        value *= 10U;
        value += digitValue;
      } while (i <= maxDigits && str.length() > i && decimalDigitValue(str[i], digitValue));
    }
    if (i > maxDigits || value > maxValue) {
      // Invalid: out of range
      return false;
    }
    str = str.substr(i);
    outputValue = static_cast<T>(value);
    return true;
  }

  /**
   * Consumes an unsigned hexadecimal number, storing the value in outputValue.
   */
  template <typename T, T maxValue>
  bool consumeHexadecimalNumber(T& outputValue) {
    constexpr unsigned maxDigits = calculateHexadecimalDigits(static_cast<unsigned>(maxValue));
    int digitValue;
    int i = 0;
    if (str.empty() || !hexadecimalDigitValue(str[i++], digitValue)) {
      // Invalid: too short
      return false;
    }
    unsigned value = digitValue;
    while (i <= maxDigits && str.length() > i && hexadecimalDigitValue(str[i], digitValue)) {
      i++;
      value <<= 4U;
      value += digitValue;
    }
    if (i > maxDigits || value > maxValue) {
      // Invalid: out of range
      return false;
    }
    str = str.substr(i);
    outputValue = static_cast<T>(value);
    return true;
  }

  /**
   * Consumes a decimal octet (0-255) with no leading zeroes.
   */
  bool consumeDecimalOctet(uint8_t& outputValue) {
    return consumeDecimalNumber<uint8_t, 255U>(outputValue);
  }

  /**
   * Consumes a hexadecimal hexadecatet (0x0 - 0xffff).
   */
  bool consumeHexadecimalHexadecatet(uint16_t& outputValue) {
    return consumeHexadecimalNumber<uint16_t, 0xffffU>(outputValue);
  }
};

} // namespace

} // namespace buf::validate::internal::lib
