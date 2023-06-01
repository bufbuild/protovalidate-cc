#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "re2/re2.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>

// <windows.h> uses macros to #define a ton of symbols,
// many of which interfere with our code here and down
// the line in various extensions.
#undef DELETE
#undef ERROR
#undef GetMessage
#undef interface
#undef TRUE
#undef min

#endif

#include "src/google/protobuf/message.h"

namespace buf::validate::internal {
using std::string;

static inline bool IsPrefix(const string& maybe_prefix, const string& search_in) {
  return search_in.compare(0, maybe_prefix.size(), maybe_prefix) == 0;
}

static inline bool IsSuffix(const string& maybe_suffix, const string& search_in) {
  return maybe_suffix.size() <= search_in.size() &&
      search_in.compare(
          search_in.size() - maybe_suffix.size(), maybe_suffix.size(), maybe_suffix) == 0;
}

static inline bool Contains(const string& search_in, const string& to_find) {
  return search_in.find(to_find) != string::npos;
}

static inline bool NotContains(const string& search_in, const string& to_find) {
  return !Contains(search_in, to_find);
}

static inline bool IsIpv4(const string& to_validate) {
  struct sockaddr_in sa;
  return !(inet_pton(AF_INET, to_validate.c_str(), &sa.sin_addr) < 1);
}

static inline bool IsIpv6(const string& to_validate) {
  struct sockaddr_in6 sa_six;
  return !(inet_pton(AF_INET6, to_validate.c_str(), &sa_six.sin6_addr) < 1);
}

static inline bool IsIp(const string& to_validate) {
  return IsIpv4(to_validate) || IsIpv6(to_validate);
}

static inline bool IsHostname(const string& to_validate) {
  if (to_validate.length() > 253) {
    return false;
  }
  const re2::RE2 component_regex("^[A-Za-z0-9]+(?:-[A-Za-z0-9]+)*$");
  std::vector<std::string> split = absl::StrSplit(to_validate, '.');
  std::vector<std::string> search = {split.begin(), split.end() - 1};
  if (split.size() < 2) {
    return re2::RE2::FullMatch(split[0], component_regex);
  }
  std::string last = split[split.size() - 1];
  for (const std::string& part : search) {
    if (part.empty() || part.size() > 63) {
      return false;
    }
    if (!re2::RE2::FullMatch(part, component_regex)) {
      return false;
    }
  }
  return true;
}

} // namespace buf::validate::internal
