.PHONY: build dev run install clean test lint fmt vet lint-fe generate deps build-fe build-be help

.DEFAULT_GOAL := build

APP_NAME := goleaf
FE_DIR   := web
BIN_DIR  := bin

# ── Primary ─────────────────────────────────────────────

build: generate deps        ## Production build
	wails3 build

dev:                        ## Dev mode with hot reload
	wails3 dev -config ./build/config.yml

run:                        ## Launch the built binary
	@$(BIN_DIR)/$(APP_NAME)$(if $(filter Windows_NT,$(shell uname -s 2>/dev/null || echo Windows)),.exe,)

install: build run          ## Build and launch

# ── Housekeeping ────────────────────────────────────────

clean:                      ## Remove build artifacts
	rm -rf $(BIN_DIR) $(FE_DIR)/dist
	go clean

# ── Quality ─────────────────────────────────────────────

test:                       ## Run Go tests
	go test -v -count=1 ./...

lint: fmt vet lint-fe       ## Run all linters

fmt:                        ## Format Go source
	go fmt ./...

vet:                        ## Run Go vet
	go vet ./...

lint-fe:                    ## Run ESLint on frontend
	cd $(FE_DIR) && pnpm lint:fix

# ── Tooling ─────────────────────────────────────────────

generate:                   ## Generate Wails TS bindings
	go mod tidy
	wails3 generate bindings -clean=true -ts -d $(FE_DIR)/bindings

deps:                       ## Install all dependencies
	go mod download
	cd $(FE_DIR) && pnpm install --frozen-lockfile

# ── Partial ─────────────────────────────────────────────

build-fe: deps generate     ## Build frontend only
	cd $(FE_DIR) && pnpm build

build-be:                   ## Check backend compilation
	go build ./...

# ── Help ────────────────────────────────────────────────

help:                       ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-14s\033[0m %s\n", $$1, $$2}'
