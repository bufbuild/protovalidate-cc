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
    "bazel_skylib": {
        "sha256": "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
        "urls": [
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
        ],
    },
    "com_github_protocolbuffers_protobuf": {
        "sha256": "75be42bd736f4df6d702a0e4e4d30de9ee40eac024c4b845d17ae4cc831fe4ae",
        "strip_prefix": "protobuf-21.7",
        "urls": [
            "https://mirror.bazel.build/github.com/protocolbuffers/protobuf/archive/v21.7.tar.gz",
            "https://github.com/protocolbuffers/protobuf/archive/v21.7.tar.gz",
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
        "sha256": "d62b93fd07c6151749e83855157f3f2778d62c168318f9c40dfcfe1c336c496f",
        "strip_prefix": "cel-cpp-da0aba702f44a41ec6d2eb4bbf6a9f01efc2746d",
        "urls": [
            "https://github.com/google/cel-cpp/archive/da0aba702f44a41ec6d2eb4bbf6a9f01efc2746d.tar.gz",
        ],
        "patches": ["@com_github_bufbuild_protovalidate_cc//bazel:cel_cpp.patch"],
        "patch_args": ["-p1"],
    },
    "com_github_bufbuild_protovalidate": {
        "sha256": "0eac3ba444770fa4050602d29a018492a03a289b76e62f236f2507af802dc684",
        "strip_prefix": "protovalidate-0.1.12",
        "urls": [
            "https://github.com/bufbuild/protovalidate/archive/v0.1.12.tar.gz",
        ],
    },
}

def protovalidate_cc_dependencies():
    """An utility method to load all dependencies of `rules_proto`.

    Loads the remote repositories used by default in Bazel.
    """

    for name in _dependencies:
        maybe(http_archive, name, **_dependencies[name])
