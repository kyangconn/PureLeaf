.PHONY: all configure build run test format format-check lint clean distclean install package help

.DEFAULT_GOAL := build

# ── Variables ────────────────────────────────────────────

PRESET   ?= x64-debug
BUILD_DIR = out/build/$(PRESET)
INSTALL_DIR = out/install/$(PRESET)
PACKAGE_DIR = out/package
CLANG_FORMAT ?= clang-format
FORMAT_SCRIPT = scripts/format.cmake

# Detect the built executable
EXE_EXT   := $(if $(filter Windows_NT,$(OS)),.exe,)
APP_EXE   := $(BUILD_DIR)/apps/desktop-qml/desktop-qml$(EXE_EXT)

# ── Primary ──────────────────────────────────────────────

all: build                  ## Build (default)

configure:                  ## Run CMake configure with preset
	cmake --preset $(PRESET)

build: configure            ## Build the project
	cmake --build --preset $(PRESET)

run: build                  ## Build and launch
	@$(APP_EXE)

# ── Quality ──────────────────────────────────────────────

test: configure             ## Build and run tests
	cmake -B $(BUILD_DIR) -DPURELEAF_BUILD_TESTS=ON
	cmake --build $(BUILD_DIR)
	cd $(BUILD_DIR) && ctest --output-on-failure

format:                     ## Auto-format C/C++ source files
	cmake -DCLANG_FORMAT_EXE="$(CLANG_FORMAT)" -DPURELEAF_FORMAT_MODE=fix -P $(FORMAT_SCRIPT)

format-check:               ## Check formatting without changing files
	cmake -DCLANG_FORMAT_EXE="$(CLANG_FORMAT)" -DPURELEAF_FORMAT_MODE=check -P $(FORMAT_SCRIPT)

lint:                       ## Run clang-format check
	$(MAKE) format-check

# ── Install & Package ────────────────────────────────────

install: build              ## Install to CMAKE_INSTALL_PREFIX
	cmake --install $(BUILD_DIR) --prefix $(INSTALL_DIR)

package: build              ## Package for distribution
	cd $(BUILD_DIR) && cpack -G ZIP

# ── Housekeeping ─────────────────────────────────────────

clean:                      ## Remove build artifacts
	rm -rf out/

distclean: clean            ## Also remove CMake caches
	find . -name 'CMakeCache.txt' -delete
	find . -name 'CMakeFiles' -type d -exec rm -rf {} + 2>/dev/null || true

# ── Help ─────────────────────────────────────────────────

help:                       ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-14s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "  Variables: PRESET=x64-debug | x64-release | linux-debug | ..."
	@echo "  Example:  make build PRESET=x64-release"
