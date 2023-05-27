#pragma once

#include <string>
#include <string_view>

#include "eval/public/cel_value.h"

namespace buf::validate::internal {

constexpr const char kUpperHexDigits[17] = "0123456789ABCDEF";
constexpr const char kLowerHexDigits[17] = "0123456789abcdef";

// TODO: Support locale-specific formatting.
class StringFormat {
 public:
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