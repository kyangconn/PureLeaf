# Agent Guidelines for goleaf

AI agent / contributor conventions for this project. Read before making changes.

---

## Project Overview

Desktop LaTeX editor: **Wails v2 (Go + Vue 3)**. Single user, no auth, files on disk.

---

## Critical Conventions

### 1. Package Managers

| Context  | Tool                         |
| -------- | ---------------------------- |
| Frontend | **pnpm** (NOT npm, NOT yarn) |
| Backend  | Go modules (`go mod`)        |

### 2. Development

```bash
wails dev          # 热重载开发（前端 + 后端）
wails build        # 生产构建
go build ./...     # 仅编译 Go（不打包前端）
```

### 3. Frontend

- **Root**: `frontend/`
- **UI**: Element Plus 组件库。不要引入其他 UI 框架。
- **SCSS**: 组件用 `<style lang="scss" scoped>`。`@/styles/_variables.scss` 提供所有色值/尺寸变量，`@/styles/_mixins.scss` 提供 `flex-center`、`text-ellipsis`、`panel-header`、`auth-page` 等可复用 mixin。
- **路径别名**: `@` → `frontend/src/`
- **API 调用**: 统一从 `api/index.js` 导入，内部走 Wails Go 绑定：`window.go.main.App.*`。不要直接写 HTTP 请求。

### 4. Backend

- **入口**: `main.go`（Wails 要求根目录）
- **依赖注入**: `internal/factory/factory.go` 的 `factory.New()` 是**唯一初始化点**。不要在别处创建 service/repository。
- **transport**: `internal/transport/wails/app.go` 是薄壳，只做参数转发，**不写业务逻辑**。
- **service**: `internal/service/` 是业务逻辑层，所有核心代码写在这里。
- **repository**: `internal/repository/` 是数据访问层（GORM + SQLite）。
- **domain**: `internal/domain/` 是领域模型，一个文件一个模型。
- **文件存储**: 项目文件落盘到 `data/projects/{id}/`，SQLite 只存文件和目录的元数据（name、parent_id、is_dir）。读文件用 `os.ReadFile`，写文件用 `os.WriteFile`。
- **单用户**: 首次启动自动创建 `admin` 用户（userID=1），所有操作硬编码 userID=1。无登录、无 JWT。

### 5. File Naming

| 层 | 命名规则 | 示例 |
|----|---------|------|
| `domain/` | `{entity}.go` | `user.go`, `file.go` |
| `repository/` | `{entity}_repo.go` | `user_repo.go` |
| `service/` | `{entity}_service.go` | `user_service.go` |
| `transport/` | 按用途 | `wails/app.go` |
| frontend components | PascalCase | `ProjectList.vue` |

### 6. Code Style

- **Go**: Standard `gofmt`. 文件命名全小写下划线。
- **Vue**: ESLint + Prettier。

### 7. What NOT to do

- ❌ 不要在 `transport/` 里写业务逻辑 — 一律委托给 `service/`
- ❌ 不要在组件里直接调 `window.go.main.App.*` — 统一走 `api/index.js`
- ❌ 不要引入 HTTP 路由、JWT、登录逻辑 — 这是桌面单用户应用
- ❌ 不要把文件内容存 SQLite — 磁盘才是真实数据源
- ❌ 不要用 `npm` / `yarn` — 只用 `pnpm`
