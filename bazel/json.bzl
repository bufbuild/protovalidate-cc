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

def _json_file_impl(repository_ctx):
    json_data = json.decode(repository_ctx.read(repository_ctx.attr.src))
    repository_ctx.file("BUILD", "exports_files([ \"json.bzl\"])")
    repository_ctx.file("json.bzl", "{}={}".format(
        repository_ctx.attr.variable,
        repr(json_data),
    ))

json_file = repository_rule(
    implementation = _json_file_impl,
    attrs = {
        "src": attr.label(allow_single_file = True),
        "variable": attr.string(mandatory = False),
    },
    local = True,
)
