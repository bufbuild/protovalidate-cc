# See https://tech.davis-hansson.com/p/make/
SHELL := bash
.DELETE_ON_ERROR:
.SHELLFLAGS := -eu -o pipefail -c
.DEFAULT_GOAL := all
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-print-directory
BIN := .tmp/bin
COPYRIGHT_YEARS := 2023-2026
LICENSE_IGNORE := -e internal/testdata/ -e .github/ -e .bcr/ -e buf/validate/conformance/*.yaml -e e2e/bzlmod/*.yaml
BUF_VERSION := 1.69.0
PROTOVALIDATE_VERSION ?= v$(shell <./deps/shared_deps.json jq -j .protovalidate.meta.version)

# Set to use a different compiler. For example, `GO=go1.18rc1 make test`.
GO ?= go

ARGS ?= --strict_message --expected_failures=buf/validate/conformance/expected_failures.yaml

BAZEL ?= bazel

.PHONY: help
help: ## Describe useful make targets
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "%-15s %s\n", $$1, $$2}'

.PHONY: all
all: test conformance ## Run all unit and conformance tests

.PHONY: clean
clean: ## Delete intermediate build artifacts
	@# -X only removes untracked files, -d recurses into directories, -f actually removes files/dirs
	git clean -Xdf

.PHONY: generate
generate: generate-bzlmod generate-license ## Regenerate code and license headers

.PHONY: test
test: generate ## Run all unit tests
	$(BAZEL) test --test_output=errors //...

.PHONY: build
build: ## Build the project
	$(BAZEL) build //...

.PHONY: conformance
conformance: $(BIN)/protovalidate-conformance
	GOBIN=$(abspath $(BIN)) $(GO) install \
    	github.com/bufbuild/protovalidate/tools/protovalidate-conformance@$(PROTOVALIDATE_VERSION)
	$(BAZEL) build -c opt //buf/validate/conformance:runner_main && \
	$(BIN)/protovalidate-conformance bazel-bin/buf/validate/conformance/runner_main $(ARGS)

.PHONY: generate-bzlmod
generate-bzlmod: ## Generate MODULE.bazel file from template
	<MODULE.bazel.tmpl >MODULE.bazel \
	jq -nrR --argjson json "$$(<./deps/shared_deps.json)" \
		'inputs | gsub("\\{\\{(?<v>[^}]+)\\}\\}"; .v | split(".") as $$path | reduce $$path[] as $$key ($$json; .[$$key]) | tostring)'

.PHONY: generate-license
generate-license: $(BIN)/license-header ## Generate license headers for files
	$(BIN)/license-header \
		--license-type apache \
		--copyright-holder "Buf Technologies, Inc." \
		--year-range "$(COPYRIGHT_YEARS)" \
		$(LICENSE_IGNORE)

.PHONY: checkgenerate
checkgenerate: generate
	@# Used in CI to verify that `make generate` doesn't produce a diff.
	test -z "$$(git status --porcelain | tee /dev/stderr)"
$(BIN):
	@mkdir -p $(BIN)

$(BIN)/buf: $(BIN) Makefile
	GOBIN=$(abspath $(@D)) $(GO) install github.com/bufbuild/buf/cmd/buf@v$(BUF_VERSION)

$(BIN)/license-header: $(BIN) Makefile
	GOBIN=$(abspath $(@D)) $(GO) install \
		  github.com/bufbuild/buf/private/pkg/licenseheader/cmd/license-header@v$(BUF_VERSION)

$(BIN)/protovalidate-conformance: $(BIN) Makefile
