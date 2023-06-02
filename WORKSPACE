workspace(name = "com_github_bufbuild_protovalidate_cc")

load("//bazel:deps.bzl", "protovalidate_cc_dependencies")

protovalidate_cc_dependencies()

load("@rules_buf//buf:repositories.bzl", "rules_buf_dependencies", "rules_buf_toolchains")

rules_buf_dependencies()

rules_buf_toolchains(version = "v1.19.0")

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

load("@com_google_cel_cpp//bazel:deps.bzl", "base_deps", "parser_deps")

base_deps()

parser_deps()

load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
)

load("@com_github_protocolbuffers_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()
