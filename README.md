# goleaf

轻量级桌面 LaTeX 编辑器，基于 **Wails v2（Go + Vue 3）**。

## 运行

```bash
# 开发模式（热重载）
wails dev

# 构建可执行文件
wails build
```

## 架构

```
main.go                          Wails 入口
internal/
├── factory/factory.go           集中依赖注入（DB、服务初始化）
├── transport/wails/app.go       Wails 方法暴露（薄壳，调 factory）
├── service/                     业务逻辑
│   ├── project_service.go       项目 CRUD
│   └── file_service.go          文件树 + 读写 + 编译
├── repository/                  数据访问（GORM + SQLite）
├── domain/                      领域模型（Project, File）
├── config/                      配置加载（Viper）
├── database/                    SQLite 初始化
└── log/                         日志

frontend/src/
├── api/
│   ├── index.js                 API 入口
│   └── transport/wails.js       Wails Go 绑定
├── stores/auth.js               认证状态
├── router/index.js              路由（Setup / Home / Editor）
├── views/                       页面组件
├── components/                  通用组件（FileTree, Loading, ProjectList...）
├── layouts/BaseLayout.vue       全局顶栏
└── styles/                      SCSS 变量 + mixin + 全局样式
```

### 数据流

```
Vue 组件                    Go 后端
  ↓                          ↓
api/index.js               factory.New()         ← 集中初始化
  ↓                          ↓
transport/wails.js         transport/wails/app.go ← 薄壳代理
  ↓                          ↓
window.go.main.App.*      service.*              ← 业务逻辑
                             ↓
                           repository.*          ← 数据访问
```

### 设计原则

- **factory 是唯一的依赖注入点** — 所有服务在此初始化
- **transport 是薄壳** — Wails 方法只做参数转发，不写业务逻辑
- **桌面单用户** — 无需账户系统，所有项目共享
- **文件落盘** — 项目文件存在 `data/projects/{id}/`，SQLite 只存元数据

## 配置

`config.yaml`：

```yaml
database:
  path: ./data/goleaf.db

latex:
  compiler: pdflatex    # 也支持 xelatex / lualatex / 全路径
  timeout: 60            # 编译超时秒数
```

## 技术栈

| 层 | 技术 |
|----|------|
| 桌面框架 | Wails v2 |
| 后端 | Go + GORM + SQLite |
| 前端 | Vue 3 + Element Plus + CodeMirror 6 |
| 样式 | SCSS（变量 + mixin） |
| 状态 | Pinia |
| 路由 | Vue Router |

## License

MIT
