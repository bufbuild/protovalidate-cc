#pragma once

#include <string>
#include <string_view>

#include "eval/public/cel_value.h"

namespace buf::validate::internal {

    class CelExt {
    public:
        bool isHostname(std::string_view) const;
        bool isEmail(std::string_view) const;
    };

} // namespace buf::validate::internal