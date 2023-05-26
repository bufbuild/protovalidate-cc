#include "buf/validate/internal/starts_with.h"

#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"

namespace buf::validate::internal {
namespace cel = ::google::api::expr::runtime;

absl::Status StartsWith::startsWith(
    std::string& builder, std::string_view fmtStr, const cel::CelList& args) const {
  return absl::OkStatus();
  // auto result = google::protobuf::Arena::Create<std::string>(arena);
  // if (!rhs.IsBytes()) {
  //   auto* error = google::protobuf::Arena::Create<cel::CelError>(
  //       arena, absl::StatusCode::kInvalidArgument, "ends_with: bytes were not provided");
  //   return cel::CelValue::CreateError(error);
  // }
  // auto suffix = rhs.BytesOrDie().value();
  // LOG(WARNING) << suffix;
  // //    if (suffix == nullptr) {
  // //        auto* error = google::protobuf::Arena::Create<cel::CelError>(
  // //                arena, absl::StatusCode::kInvalidArgument, "ends_with: no input provided");
  // //        return cel::CelValue::CreateError(error);
  // //    }
  // auto lhsValue = lhs.value();
  // if (lhsValue != nullptr && lhsValue.size() >= suffix.size()) {
  //   auto lhsData = lhsValue.data();
  //   auto suffixData = suffix.data();
  //   auto lhsSize = lhsValue.size();
  //   auto suffixSize = suffix.size();
  //   if (std::memcmp(lhsData + (lhsSize - suffixSize), suffixData, suffixSize) == 0) {
  //     return cel::CelValue::CreateString(result);
  //   }
  // }
  // auto* error = google::protobuf::Arena::Create<cel::CelError>(
  //     arena, absl::StatusCode::kInvalidArgument, "ends_with: does not ends with");
  // return cel::CelValue::CreateError(error);
}
} // namespace buf::validate::internal
