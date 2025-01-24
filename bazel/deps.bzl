# Copyright 2023 Buf Technologies, Inc.
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

_dependencies = {
    # cel-cpp needs a newer version of absl, otherwise it will fail to build in
    # a very strange manner.
    "com_google_absl": {
        "sha256": "f50e5ac311a81382da7fa75b97310e4b9006474f9560ac46f54a9967f07d4ae3",
        "strip_prefix": "abseil-cpp-20240722.0",
        "urls": [
            "https://github.com/abseil/abseil-cpp/archive/20240722.0.tar.gz",
        ],
    },
    # These extra dependencies are needed by protobuf.
    # This may be alleviated somewhat by protobuf v30.
    # https://github.com/protocolbuffers/protobuf/issues/17200
    "rules_cc": {
        "sha256": "abc605dd850f813bb37004b77db20106a19311a96b2da1c92b789da529d28fe1",
        "strip_prefix": "rules_cc-0.0.17",
        "urls": [
            "https://github.com/bazelbuild/rules_cc/releases/download/0.0.17/rules_cc-0.0.17.tar.gz"
        ],
    },
    "rules_java": {
        "sha256": "c0ee60f8757f140c157fc2c7af703d819514de6e025ebf70386d38bdd85fce83",
        "urls": [
            "https://github.com/bazelbuild/rules_java/releases/download/7.12.3/rules_java-7.12.3.tar.gz"
        ],
    },
    "rules_python": {
        "sha256": "9c6e26911a79fbf510a8f06d8eedb40f412023cf7fa6d1461def27116bff022c",
        "strip_prefix": "rules_python-1.1.0",
        "urls": [
            "https://github.com/bazelbuild/rules_python/releases/download/1.1.0/rules_python-1.1.0.tar.gz",
        ],
    },
    "com_google_protobuf": {
        "sha256": "63150aba23f7a90fd7d87bdf514e459dd5fe7023fdde01b56ac53335df64d4bd",
        "strip_prefix": "protobuf-29.2",
        "urls": [
            "https://mirror.bazel.build/github.com/protocolbuffers/protobuf/archive/v29.2.tar.gz",
            "https://github.com/protocolbuffers/protobuf/archive/v29.2.tar.gz",
        ],
    },
    "rules_proto": {
        "sha256": "dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
        "strip_prefix": "rules_proto-5.3.0-21.7",
        "urls": [
            "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
        ],
    },
    "rules_buf": {
        "sha256": "523a4e06f0746661e092d083757263a249fedca535bd6dd819a8c50de074731a",
        "strip_prefix": "rules_buf-0.1.1",
        "urls": [
            "https://github.com/bufbuild/rules_buf/archive/refs/tags/v0.1.1.zip",
        ],
    },
    "com_google_cel_cpp": {
        "sha256": "dd06b708a9f4c3728e76037ec9fb14fc9f6d9c9980e5d5f3a1d047f3855a8b98",
        "strip_prefix": "cel-cpp-0.10.0",
        "urls": [
            "https://github.com/google/cel-cpp/archive/v0.10.0.tar.gz",
        ],
        "patches": [
            "@com_github_bufbuild_protovalidate_cc//bazel:patches/cel_cpp/0001-Allow-message-field-access-using-index-operator.patch"
        ],
        "patch_args": ["-p1"],
    },
    # NOTE: Keep Version in sync with `/Makefile`.
    "com_github_bufbuild_protovalidate": {
        "sha256": "fca6143d820e9575f3aec328918fa25acc8eeb6e706050127d3a36cfdede4610",
        "strip_prefix": "protovalidate-0.9.0",
        "urls": [
            "https://github.com/bufbuild/protovalidate/archive/v0.9.0.tar.gz",
        ],
    },
}

def protovalidate_cc_dependencies():
    """An utility method to load all dependencies of `rules_proto`.

    Loads the remote repositories used by default in Bazel.
    """

    for name in _dependencies:
        maybe(http_archive, name, **_dependencies[name])
