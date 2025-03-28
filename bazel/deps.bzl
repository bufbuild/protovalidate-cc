# Copyright 2023-2025 Buf Technologies, Inc.
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

_dependencies = {
    # cel-cpp needs a newer version of absl, otherwise it will fail to build in
    # a very strange manner.
    "com_google_absl": shared_dep(
        name="absl",
    ),
    # These extra dependencies are needed by protobuf.
    # This may be alleviated somewhat by protobuf v30.
    # https://github.com/protocolbuffers/protobuf/issues/17200
    "rules_cc": dict(
        sha256="abc605dd850f813bb37004b77db20106a19311a96b2da1c92b789da529d28fe1",
        strip_prefix="rules_cc-0.0.17",
        urls=[
            "https://github.com/bazelbuild/rules_cc/releases/download/0.0.17/rules_cc-0.0.17.tar.gz"
        ],
    ),
    "rules_java": dict(
        sha256="b0b8b7b2cfbf575112acf716ec788847929f322efa5c34195eb12a43d1df7e5c",
        urls=[
            "https://github.com/bazelbuild/rules_java/releases/download/8.7.2/rules_java-8.7.2.tar.gz"
        ],
    ),
    "rules_python": dict(
        sha256="9c6e26911a79fbf510a8f06d8eedb40f412023cf7fa6d1461def27116bff022c",
        strip_prefix="rules_python-1.1.0",
        urls=[
            "https://github.com/bazelbuild/rules_python/releases/download/1.1.0/rules_python-1.1.0.tar.gz",
        ],
    ),
    "com_google_protobuf": shared_dep(
        name="protobuf",
    ),
    "rules_proto": dict(
        sha256="dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
        strip_prefix="rules_proto-5.3.0-21.7",
        urls=[
            "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
        ],
    ),
    "rules_buf": dict(
        sha256="523a4e06f0746661e092d083757263a249fedca535bd6dd819a8c50de074731a",
        strip_prefix="rules_buf-0.1.1",
        urls=[
            "https://github.com/bufbuild/rules_buf/archive/refs/tags/v0.1.1.zip",
        ],
    ),
    "com_google_cel_cpp": shared_dep(
        name="cel_cpp",
        patches=[
            "@com_github_bufbuild_protovalidate_cc//deps:patches/cel_cpp/0001-Allow-message-field-access-using-index-operator.patch",
            "@com_github_bufbuild_protovalidate_cc//deps:patches/cel_cpp/0002-Add-missing-include-for-absl-StrCat.patch",
            "@com_github_bufbuild_protovalidate_cc//deps:patches/cel_cpp/0003-Remove-unnecessary-dependency-on-cel_proto_wrap_util.patch",
        ],
        patch_args=["-p1"],
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
