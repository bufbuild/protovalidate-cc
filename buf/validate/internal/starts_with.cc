#include "buf/validate/internal/starts_with.h"

#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"

namespace buf::validate::internal {
namespace cel = ::google::api::expr::runtime;

absl::Status StartsWith::startsWith(
    std::string& builder, std::string_view lhs, const cel::CelValue rhs) const {
  auto suffix = rhs.BytesOrDie().value();
  std::byte lhsValue[lhs.length()];
  std::memcpy(lhsValue, lhs.data(), lhs.length());
  if (lhs.length() >= suffix.size()) {
    auto suffixData = suffix.data();
    auto lhsSize = lhs.length();
    auto suffixSize = suffix.size();

    if (std::memcmp(lhsValue + (lhsSize - suffixSize), suffixData, suffixSize) == 0) {
      return absl::OkStatus();
    }
  }
  return absl::InvalidArgumentError("does not start with the right value");
}
} // namespace buf::validate::internal
