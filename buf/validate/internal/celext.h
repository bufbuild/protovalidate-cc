#pragma once

#include <string>
#include <string_view>

#include "eval/public/cel_value.h"

namespace buf::validate::internal {

    class CelExt {
    public:
        bool isHostname(std::string_view lhs) const;
    };

} // namespace buf::validate::internal