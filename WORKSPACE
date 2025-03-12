workspace(name = "com_github_bufbuild_protovalidate_cc")

load("//bazel:json.bzl", "json_file")

json_file(
    name = "protovalidate_cc_dependencies",
    src = "//deps:shared_deps.json",
    variable = "shared_deps",
)

load("//bazel:deps.bzl", "protovalidate_cc_dependencies")

protovalidate_cc_dependencies()

load("@rules_java//java:rules_java_deps.bzl", "rules_java_dependencies")

rules_java_dependencies()

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("@rules_java//java:repositories.bzl", "rules_java_toolchains")

rules_java_toolchains()

load("@rules_buf//buf:repositories.bzl", "rules_buf_dependencies", "rules_buf_toolchains")

rules_buf_dependencies()

rules_buf_toolchains(version = "v1.19.0")

load("@com_google_cel_cpp//bazel:deps.bzl", "base_deps", "parser_deps")

base_deps()

parser_deps()

load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()
