workspace(name = "com_github_bufbuild_protovalidate_cc")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# TODO(afuller): Remove the buf_lint rules from protovalidate-api and only
# depend on rules from rules_proto.
# http_archive(
#     name = "rules_proto",
#     sha256 = "dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
#     strip_prefix = "rules_proto-5.3.0-21.7",
#     urls = [
#         "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
#     ],
# )
http_archive(
    name = "rules_buf",
    sha256 = "523a4e06f0746661e092d083757263a249fedca535bd6dd819a8c50de074731a",
    strip_prefix = "rules_buf-0.1.1",
    urls = [
        "https://github.com/bufbuild/rules_buf/archive/refs/tags/v0.1.1.zip",
    ],
)

load("@rules_buf//buf:repositories.bzl", "rules_buf_dependencies", "rules_buf_toolchains")

rules_buf_dependencies()

rules_buf_toolchains(version = "v1.19.0")

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

http_archive(
    name = "com_google_cel_cpp",
    sha256 = "d62b93fd07c6151749e83855157f3f2778d62c168318f9c40dfcfe1c336c496f",
    strip_prefix = "cel-cpp-da0aba702f44a41ec6d2eb4bbf6a9f01efc2746d",
    urls = [
        "https://github.com/google/cel-cpp/archive/da0aba702f44a41ec6d2eb4bbf6a9f01efc2746d.tar.gz",
    ],
)

load("@com_google_cel_cpp//bazel:deps.bzl", "base_deps", "parser_deps")

base_deps()

parser_deps()

load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
)

# http_archive(
#     name = "com_github_bufbuild_protovalidate",
#     sha256 = "f565e0a315f36986467852556d5d253e65d887d96fa921881b9a337207e755b2",
#     strip_prefix = "protovalidate-e24aa6210b787b7887b876f68f95605162c522b4",
#     urls = [
#         "https://github.com/bufbuild/protovalidate/archive/e24aa6210b787b7887b876f68f95605162c522b4.tar.gz",
#     ],
# )
local_repository(
    name = "com_github_bufbuild_protovalidate",
    path = "../protovalidate",
)
