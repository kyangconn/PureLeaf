.PHONY: help build build-fe build-be build-silent dev-fe dev-be test lint lint-fe lint-be docker clean

.DEFAULT_GOAL := help

FE_DIR := frontend

# Detect OS for binary extension
ifeq ($(OS),Windows_NT)
    BINARY := goleaf.exe
else
    BINARY := goleaf
endif

help: ## Show available commands
	@echo "Music Online Go - Available Commands"
	@echo ""
	@echo "=== Build ==="
	@echo "  make build          Build frontend + backend (production)"
	@echo "  make build-fe       Build frontend only"
	@echo "  make build-be       Build backend only (requires dist/)"
	@echo ""
	@echo "=== Develop ==="
	# @echo "  make dev            Start both frontend and backend dev server"
	@echo "  make dev-fe         Start frontend dev server (hot reload)"
	@echo "  make dev-be         Start backend dev server"
	@echo ""
	@echo "=== Quality ==="
	@echo "  make lint           Run all linters (Go + ESLint)"
	@echo "  make lint-fe        Run ESLint on frontend"
	@echo "  make lint-be        Run Go vet"
	@echo "  make test           Run all tests (Go + frontend lint)"
	@echo ""
	@echo "=== Docker ==="
	@echo "  make docker         Build Docker image"
	@echo "  make clean          Remove build artifacts"

build: build-fe build-be ## Build frontend then backend

build-fe: ## Build Vue frontend, output to cmd/server/dist/
	cd $(FE_DIR) && pnpm install --frozen-lockfile && pnpm build

build-be: ## Build Go server binary
	wails build

dev-fe: ## Start Vite dev server at localhost:5173
	cd $(FE_DIR) && pnpm dev

dev-be: ## Start Go server at localhost:8080
	wails dev

test: test-be lint-fe ## Run backend tests + frontend lint

test-be: ## Run Go tests
	go test -v ./...

# ── Lint ──────────────────────────────────────────────────

lint: ## Run all linters (Go fmt + vet + ESLint)
	go fmt ./...
	go vet ./...
	cd $(FE_DIR) && pnpm lint

lint-fe: ## Run ESLint on frontend
	cd $(FE_DIR) && pnpm eslint . --quiet --fix

lint-be: ## Run Go vet
	go vet ./...

# ── Docker ────────────────────────────────────────────────

docker: ## Build multi-stage Docker image
	docker build -t goleaf .

clean: ## Remove build artifacts
	cd $(FE_DIR) && pnpm clean
	go clean
