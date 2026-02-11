.PHONY: all build clean run run-frontend debug check-format format lint sort-dictionary cleanup-dictionary check-cspell-ignored

# -----------------------------
# Helper Functions
# -----------------------------
# Check if a tool is available in PATH
define check_tool
	@command -v $(1) >/dev/null 2>&1 || { \
		echo "Error: $(1) is not installed or not in PATH."; \
		echo "Please install $(1) to continue."; \
		exit 1; \
	}
endef

# -----------------------------
# Build Configuration
# -----------------------------
# Default preset, override with `make BUILD_TYPE=Release`
BUILD_TYPE ?= Debug
CMAKE_PRESET := conan-$(shell echo $(BUILD_TYPE) | tr A-Z a-z)

TARGET := build/$(BUILD_TYPE)
CONAN_STAMP := build/.conan.$(BUILD_TYPE).stamp
BUILD_STAMP := build/.build.$(BUILD_TYPE).$(ENABLE_COVERAGE).stamp

SOURCES := $(shell find typetrace -type f \( -name '*.cpp' -o -name '*.hpp' \))
SOURCES_CMAKE := $(shell find typetrace . -name 'CMakeLists.txt')

# -----------------------------
# Build Targets
# -----------------------------
all: $(BUILD_STAMP)

$(BUILD_STAMP): $(SOURCES) $(SOURCES_CMAKE) $(CONAN_STAMP)
	@echo "Building project ($(BUILD_TYPE))..."
	@if [ -f CMakeUserPresets.json ]; then \
		echo "Using CMake presets..."; \
		cmake --preset $(CMAKE_PRESET); \
		cmake --build --preset $(CMAKE_PRESET); \
	else \
		echo "CMakeUserPresets.json not found, using traditional CMake..."; \
		mkdir -p $(TARGET); \
		cmake -S . -B $(TARGET) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_TOOLCHAIN_FILE=$(TARGET)/conan_toolchain.cmake; \
		cmake --build $(TARGET); \
	fi
	@touch $@
	@echo "Build complete."

CONAN_CMD := conan
$(CONAN_STAMP): conanfile.txt clang.profile
	$(call check_tool,$(CONAN_CMD))
	@printf "Running Conan ($(BUILD_TYPE))...\n"
	@$(CONAN_CMD) install . \
		--profile:host=clang.profile \
		--profile:build=clang.profile \
		--build=missing \
		-s build_type=$(BUILD_TYPE)
	@touch $@
	@printf "✓ Conan setup complete\n"

conan: $(CONAN_STAMP)

clean:
	@rm -rf build CMakeFiles CMakeCache.txt CMakeUserPresets.json .cache

run: build
	@echo "Running the typetrace backend..."
	@./$(TARGET)/typetrace/backend/typetrace_backend

run-frontend: build
	@echo "Running the typetrace frontend..."
	@./$(TARGET)/typetrace/frontend/typetrace_frontend

# -----------------------------
# Utility Targets
# -----------------------------
CLANG_TIDY_CMD := clang-tidy
RUN_CLANG_TIDY_CMD := run-clang-tidy
CLANG_FORMAT_CMD := clang-format
GERSEMI_CMD := gersemi

LINT_COMMON_FLAGS = -p build/$(BUILD_TYPE)/ -quiet
LINT_TIDY_FLAGS = -warnings-as-errors='*'
LINT_CPUS ?= $(shell nproc)

GERSEMI_FLAGS = --list-expansion=favour-expansion --no-warn-about-unknown-commands

# Function to check for tool existence
# Usage: $(call check_tool, tool_name)
define check_tool
@if ! command -v $(1) > /dev/null 2>&1; then \
	echo "Error: Required tool '$(1)' not found."; \
	echo "Please ensure it is installed and available in your PATH."; \
	exit 1; \
fi
endef

ifdef SOURCES_TO_LINT
	FILES_TO_LINT := $(SOURCES_TO_LINT)
else ifeq ($(LINT_FILES),source)
	FILES_TO_LINT := $(shell find src tests -name '*.cpp' ! -path "*/build/*")
else ifeq ($(LINT_FILES),header)
	FILES_TO_LINT := $(shell find src tests -name '*.hpp' ! -path "*/build/*")
else
	FILES_TO_LINT := $(SOURCES)
endif

# Use `make lint LINT_FILES=header/source` to lint either one
lint:
	$(call check_tool,$(RUN_CLANG_TIDY_CMD))
	$(call check_tool,$(CLANG_TIDY_CMD))
	@echo "Linting with $(LINT_CPUS) cores"
	@if [ -z "$(FILES_TO_LINT)" ]; then \
		echo "No files to lint (LINT_FILES='$(LINT_FILES)')."; \
		exit 0; \
	fi

	@if [ "$(LINT_FILES)" = "source" ] || [ -z "$(LINT_FILES)" ]; then \
		SOURCE_FILES="$$(echo '$(FILES_TO_LINT)' | tr ' ' '\n' | grep '\.cpp$$')"; \
		if [ -n "$$SOURCE_FILES" ]; then \
			echo "Running clang-tidy on source files..."; \
			echo "$$SOURCE_FILES" | xargs $(RUN_CLANG_TIDY_CMD) $(LINT_COMMON_FLAGS) $(LINT_TIDY_FLAGS) -j $(LINT_CPUS) || exit 1; \
		fi; \
	fi

	@if [ "$(LINT_FILES)" = "header" ] || [ -z "$(LINT_FILES)" ]; then \
		HEADER_FILES="$$(echo '$(FILES_TO_LINT)' | tr ' ' '\n' | grep '\.hpp$$')"; \
		if [ -n "$$HEADER_FILES" ]; then \
			echo "Running clang-tidy on headers..."; \
			echo "$$HEADER_FILES" | xargs -r -P $(LINT_CPUS) -n 1 $(CLANG_TIDY_CMD) $(LINT_COMMON_FLAGS) $(LINT_TIDY_FLAGS) || exit 1; \
		fi; \
	fi

	@echo "✓ Linting complete"

lint-diff:
	$(call check_tool,$(RUN_CLANG_TIDY_CMD))
	$(call check_tool,$(CLANG_TIDY_CMD))
	@echo "Linting changed files compared to main branch..."
	@CHANGED_FILES=$$(git diff --name-only --diff-filter=ACM main...HEAD | grep -E '\.(cpp|hpp)$$' || true); \
	if [ -z "$$CHANGED_FILES" ]; then \
		echo "No C++ files changed."; \
		exit 0; \
	fi; \
	echo "Files to lint: $$CHANGED_FILES"; \
	SOURCES=$$(echo "$$CHANGED_FILES" | grep '\.cpp$$' || true); \
	HEADERS=$$(echo "$$CHANGED_FILES" | grep '\.hpp$$' || true); \
	if [ -n "$$SOURCES" ]; then \
		echo "Running clang-tidy on changed source files..."; \
		$(RUN_CLANG_TIDY_CMD) $(LINT_COMMON_FLAGS) $(LINT_TIDY_FLAGS) -j $(LINT_CPUS) $$SOURCES || exit 1; \
	fi; \
	if [ -n "$$HEADERS" ]; then \
		echo "Running clang-tidy on changed headers..."; \
		echo "$$HEADERS" | xargs -r -P $(LINT_CPUS) -n 1 $(CLANG_TIDY_CMD) $(LINT_COMMON_FLAGS) $(LINT_TIDY_FLAGS) || exit 1; \
	fi; \
	echo "✓ Linting complete"

check-format:
	$(call check_tool,$(CLANG_FORMAT_CMD))
	$(call check_tool,$(GERSEMI_CMD))
	@echo "Checking code formatting..."
	@if $(CLANG_FORMAT_CMD) --dry-run --Werror $(SOURCES) && $(GERSEMI_CMD) --check --diff --color $(GERSEMI_FLAGS) $(SOURCES_CMAKE); then \
		echo "✓ All files are properly formatted"; \
	else \
		exit 1; \
	fi

format:
	$(call check_tool,$(CLANG_FORMAT_CMD))
	$(call check_tool,$(GERSEMI_CMD))
	@echo "Formatting code..."
	@$(CLANG_FORMAT_CMD) -i $(SOURCES)
	@$(GERSEMI_CMD) -i $(GERSEMI_FLAGS) $(SOURCES_CMAKE)
	@echo "✓ Code formatting complete"


sort-dictionary:
	@echo "Sorting dictionary..."
	tr '[:upper:]' '[:lower:]' < .cspell_ignored | sort -f -u -o .cspell_ignored
	@echo "✓ Sorted and converted .cspell_ignored to lowercase with unique entries"

cleanup-dictionary:
	@echo "Cleaning up unused words from .cspell_ignored..."
	@.github/scripts/cleanup-cspell-ignored.sh

check-cspell-ignored:
	@echo "Checking for unused words in .cspell_ignored..."
	@.github/scripts/check-cspell-ignored.sh
	@echo "✓ Cspell ignored file check complete"
