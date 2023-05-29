#include "buf/validate/internal/celext.h"

#include <string>
#include <algorithm>

namespace buf::validate::internal {
    namespace cel = ::google::api::expr::runtime;

    bool CelExt::isHostname(std::string_view lhs) const {
        if (lhs.length() > 253) {
            return false;
        }

        std::string s(lhs);
        std::string_view delimiter = ".";
        if (s.length() >= delimiter.length() &&
            s.compare(s.length() - delimiter.length(), delimiter.length(), delimiter) == 0) {
            s.substr(0, s.length() - delimiter.length());
        }
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });

        // split isHostname on '.' and validate each part
        size_t pos = 0;
        std::string_view part;
        while ((pos = s.find(delimiter)) != std::string_view::npos) {
            part = s.substr(0, pos);
            // if part is empty, longer than 63 chars, or starts/ends with '-', it is invalid
            if (part.empty() || part.length() > 63 || part.front() == '-' || part.back() == '-') {
                return false;
            }
            // for each character in part
            for (char ch: part) {
                // if the character is not a-z, 0-9, or '-', it is invalid
                if ((ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '-') {
                    return false;
                }
            }
            s.erase(0, pos + delimiter.length());
        }

        return true;
    }

    bool CelExt::isEmail(std::string_view addr) const {
        std::string_view::size_type pos = addr.find('<');
        if (pos != std::string_view::npos) {
            return false;
        }

        std::string_view localPart, domainPart;
        std::string emailStr(addr);
        std::size_t atPos = emailStr.find('@');
        if (atPos != std::string::npos) {
            localPart = emailStr.substr(0, atPos);
            domainPart = emailStr.substr(atPos + 1);
        } else {
            return false;
        }

        if (localPart.length() < 1 || localPart.length() > 64 || domainPart.length() > 253) {
            return false;
        }

        // Validate the hostname
        return isHostname(domainPart);
    }


} // namespace buf::validate::internal
