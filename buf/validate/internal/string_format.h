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

#pragma once

#include <string>
#include <string_view>

#include "eval/public/cel_value.h"

namespace buf::validate::internal {

constexpr const char kUpperHexDigits[17] = "0123456789ABCDEF";
constexpr const char kLowerHexDigits[17] = "0123456789abcdef";

class StringFormat {
 public:
  google::api::expr::runtime::CelValue format(
      google::protobuf::Arena* arena,
      google::api::expr::runtime::CelValue::StringHolder format,
      google::api::expr::runtime::CelValue arg) const;

  absl::Status format(
      std::string& builder,
      std::string_view format,
      const google::api::expr::runtime::CelList& args) const;

  absl::Status formatExponent(
      std::string& builder, const google::api::expr::runtime::CelValue& value, int precision) const;

  absl::Status formatFloating(
      std::string& builder, const google::api::expr::runtime::CelValue& value, int precision) const;

  void formatUnsigned(
      std::string& builder,
      uint64_t value,
      int base,
      const char digits[17] = kLowerHexDigits) const;

  void formatInteger(
      std::string& builder, int64_t value, int base, const char digits[17] = kLowerHexDigits) const;
  void formatHexString(std::string& builder, std::string_view value) const;
  void formatHeXString(std::string& builder, std::string_view value) const;

  absl::Status formatBinary(
      std::string& builder, const google::api::expr::runtime::CelValue& value) const;
  absl::Status formatOctal(
      std::string& builder, const google::api::expr::runtime::CelValue& value) const;
  absl::Status formatDecimal(
      std::string& builder, const google::api::expr::runtime::CelValue& value) const;
  absl::Status formatHex(
      std::string& builder, const google::api::expr::runtime::CelValue& value) const;
  absl::Status formatHeX(
      std::string& builder, const google::api::expr::runtime::CelValue& value) const;
  absl::Status formatString(
      std::string& builder, const google::api::expr::runtime::CelValue& value) const;

 private:
  absl::Status formatStringSafe(
      std::string& builder, const google::api::expr::runtime::CelValue& value) const;
};

} // namespace buf::validate::internal
