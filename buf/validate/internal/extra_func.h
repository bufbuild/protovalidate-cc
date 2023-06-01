#pragma once

#include <arpa/inet.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "absl/status/status.h"
#include "eval/public/cel_function_registry.h"
#include "re2/re2.h"
#include "src/google/protobuf/message.h"

namespace buf::validate::internal {

absl::Status RegisterExtraFuncs(
    google::api::expr::runtime::CelFunctionRegistry& registry, google::protobuf::Arena* regArena);

using std::string;

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