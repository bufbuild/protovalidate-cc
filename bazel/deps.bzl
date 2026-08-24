# Copyright 2023-2026 Buf Technologies, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@protovalidate_cc_dependencies//:json.bzl", "shared_deps")

def format_values(value, meta):
    if type(value) == "string":
        return value.format(**meta)
    elif type(value) == "list":
        return [v.format(**meta) for v in value]
    else:
        return value

def shared_dep(name, **kwargs):
    dep = {k: format_values(v, shared_deps[name]["meta"]) for k, v in shared_deps[name]["source"].items()}
    return dict(dep, **kwargs)

_re2 = dict(
    sha256="87f6029d2f6de8aa023654240a03ada90e876ce9a4676e258dd01ea4c26ffd67",
    strip_prefix="re2-2025-11-05",
    urls=[
        "https://github.com/google/re2/archive/refs/tags/2025-11-05.tar.gz",
    ],
)

_dependencies = {
    # Some archives are registered under both their legacy WORKSPACE repo names
    # and their bzlmod module names, since BUILD files in the dependency graph
    # reference them by either name.
    "com_google_absl": shared_dep(
        name="absl",
    ),
    "abseil-cpp": shared_dep(
        name="absl",
    ),
    "com_googlesource_code_re2": _re2,
    "re2": _re2,
    "bazel_features": dict(
        sha256="c26b4e69cf02fea24511a108d158188b9d8174426311aac59ce803a78d107648",
        strip_prefix="bazel_features-1.43.0",
        urls=[
            "https://github.com/bazel-contrib/bazel_features/releases/download/v1.43.0/bazel_features-v1.43.0.tar.gz",
        ],
    ),
    "bazel_skylib": dict(
        sha256="3b5b49006181f5f8ff626ef8ddceaa95e9bb8ad294f7b5d7b11ea9f7ddaf8c59",
        urls=[
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.9.0/bazel-skylib-1.9.0.tar.gz",
        ],
    ),
    "com_google_googletest": dict(
        sha256="65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c",
        strip_prefix="googletest-1.17.0",
        urls=[
            "https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz",
        ],
    ),
    "rules_cc": dict(
        sha256="1de5b47721fce0af0dd453b3071228fdfc44bd18199826b3f0b03b423aae9f65",
        strip_prefix="rules_cc-0.2.18",
        urls=[
            "https://github.com/bazelbuild/rules_cc/releases/download/0.2.18/rules_cc-0.2.18.tar.gz",
        ],
    ),
    "rules_java": dict(
        sha256="6ef26d4f978e8b4cf5ce1d47532d70cb62cd18431227a1c8007c8f7843243c06",
        urls=[
            "https://github.com/bazelbuild/rules_java/releases/download/9.3.0/rules_java-9.3.0.tar.gz",
        ],
    ),
    "rules_python": dict(
        sha256="2f5c284fbb4e86045c2632d3573fc006facbca5d1fa02976e89dc0cd5488b590",
        strip_prefix="rules_python-1.6.3",
        urls=[
            "https://github.com/bazelbuild/rules_python/releases/download/1.6.3/rules_python-1.6.3.tar.gz",
        ],
    ),
    "com_google_protobuf": shared_dep(
        name="protobuf",
    ),
    "rules_proto": dict(
        sha256="14a225870ab4e91869652cfd69ef2028277fc1dc4910d65d353b62d6e0ae21f4",
        strip_prefix="rules_proto-7.1.0",
        urls=[
            "https://github.com/bazelbuild/rules_proto/releases/download/7.1.0/rules_proto-7.1.0.tar.gz",
        ],
    ),
    "rules_buf": dict(
        sha256="1ebeb843f09a62bd04de9b408c43a0759775c9cf9c063a7b386d70cee7f70c8b",
        strip_prefix="rules_buf-0.3.0",
        urls=[
            "https://github.com/bufbuild/rules_buf/archive/refs/tags/v0.3.0.zip",
        ],
    ),
    # Upstream antlr4 has no Bazel build.
    "antlr4-cpp-runtime": dict(
        build_file_content="""
package(default_visibility = ["//visibility:public"])
cc_library(
    name = "antlr4-cpp-runtime",
    srcs = glob(["runtime/Cpp/runtime/src/**/*.cpp"]),
    hdrs = glob(["runtime/Cpp/runtime/src/**/*.h"]),
    defines = ["ANTLR4CPP_USING_ABSEIL"],
    includes = ["runtime/Cpp/runtime/src"],
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/synchronization",
    ],
)
        """,
        sha256="42d1268524a9c972f5ca1ad1633372ea02a812ff66c1e992925edea5e5cf9c81",
        strip_prefix="antlr4-4.13.2",
        urls=["https://github.com/antlr/antlr4/archive/refs/tags/4.13.2.zip"],
    ),
    "com_google_cel_cpp": shared_dep(
        name="cel_cpp",
    ),
    "com_github_bufbuild_protovalidate": shared_dep(
        name="protovalidate",
    ),
}

def protovalidate_cc_dependencies():
    """An utility method to load all dependencies of `rules_proto`.

    Loads the remote repositories used by default in Bazel.
    """

    for name in _dependencies:
        maybe(http_archive, name, **_dependencies[name])
