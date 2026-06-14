# Agent Guidelines for goleaf

AI agent / contributor conventions for this project. Read before making changes.

---

## Project Overview

Desktop LaTeX editor: **Wails v3 (Go + Vue 3 + TypeScript)**. Single user, no auth, files on disk.

---

## Critical Conventions

### 1. Package Managers

| Context  | Tool                         |
| -------- | ---------------------------- |
| Frontend | **pnpm** (NOT npm, NOT yarn) |
| Backend  | Go modules (`go mod`)        |

### 2. Development Commands

```bash
wails3 dev -config ./build/config.yml  # 热重载开发（Wails + Vite）
wails3 build       # 生产构建桌面应用
go build ./...     # 仅编译 Go（不打包前端）
go test ./...      # 后端测试
task build         # pnpm build + wails3 build
make lint          # go fmt + go vet + 前端 ESLint fix
```

Wails v3 构建由 `Taskfile.yml` 和 `build/config.yml` 管理。前端目录为 `web/`，Wails 前端安装/构建命令必须继续使用 pnpm。

### 3. Frontend

- **Root**: `web/`
- **Stack**: Vue 3 + TypeScript + Vite + Vue Router + Element Plus + CodeMirror 6。
- **UI**: Element Plus 组件库和 `@element-plus/icons-vue`。不要引入其他 UI 框架。
- **SCSS**: 组件样式使用 `<style lang="scss" scoped>`。`@/styles/_variables.scss` 提供色值/尺寸变量，`@/styles/_mixins.scss` 提供 `flex-center`、`flex-between`、`flex-column`、`text-ellipsis`、`full-page`、`panel-header`、`custom-scrollbar`、`desktop-only` 等复用 mixin。
- **路径别名**: `@` -> `web/src/`，`@bindings` -> `web/bindings/`。
- **API 调用**: 组件统一从 `web/src/api/index.ts` 导入 `projectAPI` / `fileAPI`。内部调用 Wails v3 生成绑定。不要在组件里直接调 Wails 生成函数，也不要新增 HTTP/axios 请求。
- **Wails 绑定**: `web/bindings/` 是生成代码，不要手改；后端导出方法变更后用 `wails3 generate bindings -clean=true -ts` 重新生成。
- **返回结构**: Wails 绑定直接返回领域对象或数组，例如 `Promise<domain.Project>`、`Promise<Array<domain.File>>`，不是 axios 风格 `{ data }`，除非 API 层显式包装。

### 4. Backend

- **入口**: `main.go`（Wails 要求根目录），嵌入 `web/dist`。
- **依赖注入**: `internal/factory/factory.go` 的 `factory.New()` 是**唯一初始化点**。不要在别处创建 service/repository。
- **bindings**: `internal/bindings/` 是 Wails v3 method bindings 薄壳，只做参数转发和 Wails 返回值适配，**不写业务逻辑**。
- **service**: `internal/service/` 是业务逻辑层，项目/文件/编译等核心代码写在这里。
- **repository**: `internal/repository/` 是数据访问层（GORM + SQLite）。
- **domain**: `internal/domain/` 是领域模型，一个文件一个模型。
- **config**: `internal/config/config.go` 使用 Viper，优先级为环境变量 > `config.yaml` > 默认值。环境变量前缀为 `GOLEAF_`。
- **database**: `internal/database/database.go` 使用纯 Go SQLite 驱动 `github.com/glebarez/sqlite`。
- **文件存储**: 项目文件落盘到 `{database dir}/projects/{id}/`。`database.path` 留空时默认使用系统用户配置目录。SQLite 只存文件和目录元数据（name、parent_id、is_dir）。读文件用 `os.ReadFile`，写文件用 `os.WriteFile`。
- **LaTeX 编译**: 通过配置的 `latex.compiler` 执行 `pdflatex` / `xelatex` / `lualatex` 等命令，超时由 `latex.timeout` 控制。
- **单用户**: 桌面应用，无用户系统、无登录、无 JWT。所有项目共享。

### 5. File Naming

| 层 | 命名规则 | 示例 |
|----|---------|------|
| `domain/` | `{entity}.go` | `project.go`, `file.go` |
| `repository/` | `{entity}_repo.go` | `project_repo.go`, `file_repo.go` |
| `service/` | `{entity}_service.go` | `project_service.go`, `file_service.go` |
| `bindings/` | 按用途 | `project_service.go`, `file_service.go` |
| frontend components | PascalCase | `ProjectList.vue`, `FileTree.vue` |

### 6. Current Product State

- 已迁移到 Wails v3 桌面端，前后端通过 Wails method bindings 通信。
- 已有项目列表、项目创建/删除/重命名、文件树、新建/重命名/删除文件、CodeMirror 编辑、2 秒自动保存、LaTeX 编译和 PDF iframe 预览。
- 已移除用户系统和 HTTP/axios 依赖；当前业务不走 HTTP API。
- `todo.md` 记录后续方向：文件快照/Git 集成、测试、CI、PDF 预览增强、多 Tab、LaTeX 补全、模板、图片拖入、主题切换。

### 7. Code Style

- **Go**: Standard `gofmt`。文件命名全小写下划线。
- **Vue/TS**: ESLint + Prettier。保持 Composition API 写法。
- **Comments**: 只在复杂逻辑前写必要说明，不写重复代码含义的注释。

### 8. What NOT to do

- 不要在 `bindings/` 里写业务逻辑，一律委托给 `service/`。
- 不要在组件里直接调 Wails 生成函数，统一走 `web/src/api/index.ts`。
- 不要把文件内容存 SQLite，磁盘才是真实数据源。
- 不要用 `npm` / `yarn`，只用 `pnpm`。
- 不要手动修改 `web/bindings/` 生成文件。
- 不要重新引入用户、认证、JWT、多用户权限等 Web 服务概念。
