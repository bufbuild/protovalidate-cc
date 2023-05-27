#pragma once

#include <string>
#include <string_view>

#include "eval/public/cel_value.h"

namespace buf::validate::internal {

class StartsWith {
 public:
  absl::Status startsWith(
      std::string& builder,
      std::string_view format,
      const google::api::expr::runtime::CelValue args) const;

 private:
};

} // namespace buf::validate::internal