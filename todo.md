# goleaf — 待办事项

> **产品定位**：专注、本地优先、有安全感的 LaTeX 写作环境。
> 不是 IDE，不是协作平台，不是 Overleaf 替代品。
> 卖点是"写 LaTeX"这件事摩擦最小：开箱即用、编译预览闭环流畅、自动快照给安全感。
> **把"少干扰"当成功能，而不是把"多功能"当成目标。**

---

## 📦 技术栈与架构（当前实现，待迁移）

### 技术选型
- **桌面框架**：Wails v3（`v3.0.0-alpha.98-tui`，Go + WebView）
- **后端**：Go 1.26 + GORM + SQLite（纯 Go 驱动 `glebarez/sqlite`，无 CGO）
- **前端**：Vue 3 + TypeScript + Vite 8 + Vue Router
- **UI**：Element Plus + `@element-plus/icons-vue`
- **编辑器**：CodeMirror 6
- **PDF 渲染**：pdf.js（`pdfjs-dist` v6，canvas 渲染）
- **状态管理**：Pinia
- **样式**：SCSS（`_variables.scss` + `_mixins.scss`）
- **配置**：Viper（`config.yaml`，环境变量前缀 `GOLEAF_`）

### 后端分层（`internal/`）
| 目录 | 职责 |
|------|------|
| `factory/factory.go` | **唯一依赖初始化点** `factory.New()`，聚合所有 service |
| `bindings/` | Wails 方法绑定的**薄壳**，只做参数转发和 DTO 适配，不写业务逻辑 |
| `service/` | 业务逻辑层（项目/文件/编译/快照/synctex） |
| `repository/` | GORM 数据访问层 |
| `domain/` | 领域模型（一个文件一个模型） |
| `config/` | Viper 配置加载 + XDG 数据目录解析 |
| `database/` | SQLite 连接初始化 |
| `log/` | 日志（lumberjack 滚动） |

### 前端结构（`web/src/`）
| 目录/文件 | 职责 |
|-----------|------|
| `api/index.ts` | **唯一 API 出口**，封装 `projectAPI` / `fileAPI` / `latexAPI`，组件不直接调 Wails 绑定 |
| `views/` | Home（项目列表）/ Editor（编辑器）/ Settings（设置） |
| `components/` | FileTree / TreeNode / PdfPreview / ProjectList / Loading / HistoryPanel / SnapshotPanel |
| `layouts/BaseLayout.vue` | 无边框窗口外壳 + 自定义标题栏（`--wails-draggable`） |
| `platform/window.ts` | 窗口操作（最小化/最大化/关闭，调 `@wailsio/runtime`） |
| `stores/theme.ts` | appTheme + editorTheme（localStorage 持久化） |
| `styles/` | SCSS 变量 + mixin（`flex-center` / `flex-between` / `panel-header` / `custom-scrollbar` 等） |

### 数据目录布局（XDG 规范）
```
{dataRoot}/
├── goleaf.db                          SQLite 元数据 + 历史索引
├── projects/{id}/                     项目工作文件（真实数据源）
├── .backup/blobs/{hash[:2]}/{hash}    内容寻址快照存储
└── downloads/texlive/                 缓存的 TeX Live 安装器
```
- **Linux**：`$XDG_DATA_HOME/goleaf`，未设置则 `~/.local/share/goleaf`
- **macOS**：`~/Library/Application Support/goleaf`
- **Windows**：`%LOCALAPPDATA%/goleaf`，回退 `%APPDATA%/goleaf`
- `dataRoot = filepath.Dir(config.Database.Path)`

### 窗口配置（`main.go`）
- `Frameless: true`，1280×800，最小 900×600
- 自定义标题栏：`--wails-draggable: drag` 标记可拖拽区，`no-drag` 排除按钮
- 嵌入 `web/dist`（`//go:embed all:web/dist`）

---

## 🗄️ 数据模型

### `projects` 表
| 字段 | 类型 | 说明 |
|------|------|------|
| id | uint PK | |
| name | string(255) | 项目名 |
| created_at / updated_at | time | |

### `files` 表（邻接表，文件/文件夹统一）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | uint PK | |
| project_id | uint index | 所属项目 |
| parent_id | *uint index | 父目录（nil = 根） |
| name | string(255) | 名字（单层，非路径） |
| is_dir | bool | 是否文件夹 |
| created_at / updated_at | time | |
| Content / Children | - | `gorm:"-"`，不落库，运行时填充 |

### `file_revisions` 表（文件历史版本）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | uint PK | |
| project_id | uint index | 冗余，便于项目级查询 |
| file_id | uint index | 文件 ID（删除后变悬空引用，预期） |
| file_path | string(512) | 当时相对路径 |
| content_hash | string(64) index | sha256 hex |
| blob_path | string(512) | 相对 blob root 的路径 |
| size | int64 | 字节数 |
| reason | string(32) | save / delete |
| snapshot_id | *uint index | 关联的项目快照（nil = 普通保存） |
| created_at | time index | |

### `project_snapshots` 表（破坏性操作前的项目快照）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | uint PK | |
| project_id | uint index | |
| reason | string(32) | delete |
| file_count | int | 快照文件数 |
| total_size | int64 | 总字节数 |
| snapshot_of | string(512) | 快照目标路径（如被删文件） |
| created_at | time index | |

---

## 🔌 后端 Wails 绑定方法清单（24 个）

### ProjectService（5 个）
- `ListProjects()` → `[]ProjectDTO`
- `GetProject(id)` → `ProjectDTO`
- `CreateProject(name)` → `ProjectDTO`
- `UpdateProject(id, name)` → `ProjectDTO`
- `DeleteProject(id)`

### FileService（15 个）
- **文件树/内容**：`GetFileTree` / `GetFileContent` / `CreateFile` / `UpdateFileContent` / `RenameFile` / `DeleteFile`
- **编译**：`CompileProject` → `CompileResult{PDF []byte, Log string}`（PDF 以字节返回，前端 base64 解码）
- **历史/快照**：`GetFileHistory` / `GetProjectHistory` / `GetProjectSnapshots` / `GetRevisionContent` / `DiffRevisions`
- **SyncTeX**：`SynctexForward(projectID, input, line, column)` / `SynctexInverse(projectID, page, x, y)`
- **系统**：`OpenProjectFolder`

### EnvironmentService（4 个）
- `CheckLatexEnvironment()` → 检测 pdflatex/xelatex/lualatex/tlmgr 等工具
- `DownloadTexLiveInstaller(variant)` → 下载 TeX Live 安装器
- `StartTexLiveInstaller(variant)` → 启动安装器
- `ReloadLatexEnvironment()` → 刷新 PATH 重检测

---

## ✅ 已完成功能与实现细节

### 1. 项目管理
- 列表（紧凑列表样式）、创建、重命名（双击/对话框）、删除（带确认）
- **补偿式创建**：磁盘创建失败时回滚 DB 记录（`project_service.go`）
- 新建项目自动生成 `main.tex` 模板（含 ctex 中文支持）

### 2. 文件系统
- 邻接表模型（`parent_id`），文件/文件夹统一
- **路径计算**：`computePath` 从 fileID 向上拼接相对路径，内部带 `ValidateRelativePath` 校验
- **递归删除**：`collectDescendants` 收集子孙 ID，删除前生成快照
- **原子写**：`writeFileAtomic`（临时文件 + fsync + rename + chmod）
- **项目级锁**：`ProjectLockManager`，每个 projectID 一把 `sync.Mutex`，所有操作串行化

### 3. 路径安全（`path_guard.go`）
- `ValidateFileName`：拒绝空 / `.` / `..` / 路径分隔符（`/` `\`）/ Windows 盘符前缀（`C:`）/ UNC（`\\`）/ 非法字符（`<>:"|?*`）/ 控制字符 / 末尾点空格 / 保留设备名（CON/COM1 等）/ 超长
- `ValidateRelativePath`：拒绝绝对路径（含 Windows 上 `filepath.IsAbs` 漏判的 `/a`）/ `..` 段 / 前导分隔符
- `computeFilePath(fileMap, fileID)`：带循环引用保护的路径拼接（共享 helper）
- **同级重名检查**：`FindByNameInParent(projectID, parentID, name, ignoreID)`

### 4. 编辑器（CodeMirror 6）
- 2 秒 debounce 自动保存（`scheduleAutoSave`）
- 切换文件时保存当前文件
- 编译前自动保存
- 编辑器主题（VS Code 暗色风格）：通过 `Prec.high(EditorView.theme(...))` + CSS 变量
- 主题切换时销毁重建编辑器实例
- 动态 import（`codemirror` / `@codemirror/view` / `@codemirror/state` / `@codemirror/theme-one-dark`）

### 5. LaTeX 编译
- 支持 pdflatex / xelatex / lualatex（通过 `latex.compiler` 配置）
- `-interaction=nonstopmode -synctex=1 -output-directory=工作目录`
- 超时控制（`context.WithTimeout`，默认 60 秒）
- 主文件选择：优先 `main.tex`，否则第一个 `.tex`
- PDF 以**字节流**返回（`CompileResult.PDF []byte`），前端 base64 解码 → Blob URL

### 6. SyncTeX 正反向同步（`synctex.go`）
- 编译加 `-synctex=1` 生成 `.synctex.gz`
- **后端封装 synctex CLI**：
  - `SynctexView(workDir, input, line, column, pdfPath)` → `{Page, X, Y, H, V, W, Height}`
  - `SynctexEdit(workDir, page, x, y, pdfPath)` → `{Input, Line, Column, Offset}`
  - 解析 `SyncTeX result begin...end` 输出块（纯函数 + 测试）
- **前端 pdf.js 渲染**（替换原 iframe）：
  - 每页独立 canvas，devicePixelRatio 高清
  - **坐标转换**：synctex 用 big point(72dpi)，pdf.js 用 CSS px(96dpi)，比率 `72/96`，正反两个方向都处理，并考虑缩放系数
  - **反向同步**：canvas click → 算 bp 坐标 → API → 跳编辑器光标（自动打开对应文件）
  - **正向同步**：光标行 → API → 滚动到 PDF 位置 + accent 色高亮闪烁动画
- 顺带完成：缩放（0.5x-3x）、页码指示器

### 7. LaTeX 环境管理（`latex_environment_service.go`）
- 检测系统已安装的 LaTeX 工具（`exec.LookPath` + `--version`）
- 下载 TeX Live 安装器（`install-tl-windows.exe` / `install-tl-unx.tar.gz`，断点续传式临时文件 + rename）
- 启动安装器（`explorer`/`open`/`xdg-open`）
- Windows 环境变量重载（`environment_reload_windows.go`，刷新进程 PATH）
- variant：base / small / medium / full（对应 TeX Live scheme）

### 8. 文件版本管理（快照系统）
- **blob 内容寻址**（`blob_store.go`）：sha256 去重，`Put` 相同内容跳过写入，目录分桶 `{hash[:2]}/{hash}`
- **每次保存记录历史**：`UpdateContent` 算 hash，与上一版相同则跳过，否则写 blob + 插 `file_revisions(reason=save)`
- **删除前快照**：删除文件/项目前，把所有文件内容入 blob + 创建 `project_snapshots` + 关联 `file_revisions(reason=delete)`
- 历史失败不阻断主流程（文件已落盘为先）

### 9. Diff（`diff.go`，无外部依赖）
- 基于 LCS 动态规划的行级 unified diff
- ±3 行上下文，GNU diff 兼容格式
- 处理纯插入/删除/修改场景，hunk 合并（相邻变更间隔 ≤ 6 行）

### 10. 历史与快照面板
- **HistoryPanel**：项目级版本列表（倒序），点击选 A（查看内容）→ 选 B（A/B diff）→ 重置；save/delete 不同色圆点
- **SnapshotPanel**：删除前快照列表（时间、文件数、大小、目标路径）
- 顶栏三段式布局切换：文件 / 历史 / 快照（`activePanel` + `panelRefreshKey`）
- FileTree 用 `v-show` 保状态，History/Snapshot 用 `v-if` 按需加载

### 11. 主题系统
- `appTheme`（应用 chrome）+ `editorTheme`（编辑器）独立切换，localStorage 持久化
- CSS 变量驱动（`web/src/styles/index.scss`），暗色/亮色双套

### 12. 界面
- 无边框窗口 + 自定义标题栏（拖拽/最小化/最大化/关闭）
- 编辑器可拖拽分隔条（sidebar / pdf 面板宽度）
- 项目重命名（双击标题或按钮）
- 在文件夹中打开项目（`explorer`/`open`/`xdg-open`）

---

## 🔧 外部依赖与工具链

### 后端关键依赖
| 包 | 用途 |
|----|------|
| `gorm.io/gorm` | ORM |
| `github.com/glebarez/sqlite` | 纯 Go SQLite（无 CGO） |
| `github.com/spf13/viper` | 配置 |
| `gopkg.in/natefinch/lumberjack.v2` | 日志滚动 |
| `github.com/wailsapp/wails/v3` | 桌面框架 |

### 前端关键依赖
| 包 | 用途 |
|----|------|
| `vue` / `vue-router` / `pinia` | 框架 |
| `element-plus` + `@element-plus/icons-vue` | UI |
| `codemirror` + `@codemirror/*` | 编辑器 |
| `pdfjs-dist` | PDF 渲染（SyncTeX 必需） |
| `@wailsio/runtime` | Wails 运行时（窗口操作） |
| `eslint` / `prettier` / `eslint-plugin-perfectionist` | 代码规范 |

### 系统 CLI 依赖（运行时）
- `synctex`（TeX Live 自带，SyncTeX 查询）
- `pdflatex` / `xelatex` / `lualatex`（编译）
- `explorer` / `open` / `xdg-open`（打开文件夹）

### 构建工具
- `wails3` CLI（`go install github.com/wailsapp/wails/v3/cmd/wails3@latest`）
- 绑定生成：`wails3 generate bindings -clean=true -ts -d web/bindings`

---

## ⚙️ 配置（`config.yaml`）

```yaml
database:
  path: ""        # 空 = 使用 XDG 数据目录
latex:
  compiler: pdflatex   # pdflatex / xelatex / lualatex / 完整路径
  timeout: 60          # 编译超时秒数
```
- 环境变量覆盖：`GOLEAF_DATABASE_PATH` / `GOLEAF_LATEX_COMPILER` / `GOLEAF_LATEX_TIMEOUT`

---

## 🧪 测试与 CI

### 后端测试（`internal/service/*_test.go`）
- `path_guard_test.go`：文件名/相对路径校验（含路径穿越攻击用例）
- `blob_store_test.go`：去重 / 路径相对性 / 绝对路径拒绝
- `diff_test.go`：插入/删除/修改/上下文/hunk 头
- `synctex_test.go`：真实 CLI 输出解析
- `project_lock_test.go` / `atomic_file_test.go`

### CI（`.github/workflows/ci.yml`）
- PR 触发，前后端分流 + 路径过滤（`dorny/paths-filter`）
- 前端：lint + vue-tsc + vitest + build
- 后端：gofmt + vet + test -race
- concurrency 自动取消旧运行

### Release（`.github/workflows/release.yml`）
- tag 触发，三平台 matrix（linux/windows/darwin）原生构建
- Linux 需 `libgtk-3-dev libwebkit2gtk-4.1-dev`

---

## 🟢 核心（下一阶段优先做）

### 写作主循环打磨
- [ ] 编译错误定位：从 log 跳转到对应源码文件行
- [ ] 编译前选择主 `.tex` 文件（项目内多个 .tex 时）
- [ ] 长耗时操作优化：编译改为复制项目到临时目录后执行，避免长时间持有项目锁
- [ ] 编译中间产物清理策略（aux/log/out 隐藏或清理）

### 开箱即用
- [ ] **项目模板**（论文 / 简历 / Beamer 幻灯片）
- [ ] 设置持久化（UI 修改 compiler/timeout，运行时刷新）

### 版本安全感
- [ ] RestoreRevision（从历史版本还原到工作区）
- [ ] 后台 GC 清理 orphan blob 和过期快照
- [ ] 版本事务边界（restore/reset 前必须先生成 snapshot）

---

## 🟡 锦上添花

- [ ] 最近打开文件 / 上次编辑位置恢复
- [ ] 未保存状态提示与手动保存按钮（Ctrl+S）
- [ ] 项目导入 / 导出为 zip
- [ ] 暗色/亮色主题切换（应用级与编辑器级独立，已有基础）
- [ ] 应用图标
- [ ] 统一前端错误处理：Wails 异常 → Element Plus message
- [ ] 文件操作补偿：DB 成功但磁盘失败时回滚元数据
- [ ] 前端测试（Vitest + Playwright E2E）

---

## 🔴 明确不做（决策记录）

| 功能 | 拒绝理由 |
|------|---------|
| 多 Tab 编辑 | 专注单文件写作更纯粹；多 Tab 是 IDE 思维 |
| 自动补全（自造） | 不如 LSP；接 LSP 又是巨大工程，是 VS Code 插件主场 |
| LaTeX 语法高亮增强 | CodeMirror 默认够用，投入产出比极低 |
| 文件树拖拽移动 | 重命名已够用，拖拽增加复杂度不增写作价值 |
| 完整 LSP（texlab） | 做完和 LaTeX Workshop 重叠 90%，无插件生态/远程开发优势 |
| 写作检查（LTeX LS） | 独立大工程，与"专注写作"定位关系间接 |
| 图片拖入 `\includegraphics` | 小聪明，维护成本高于价值 |
| 实时协作 / 多用户 / JWT | 桌面单用户，已从架构移除 |

---

## 🧭 远期探索

### 类 Git 异步协作（非实时）
- 局域网自动发现，合并/冲突解决模型（非 OT/CRDT）
- 与本地优先、快照系统契合
- ⚠️ Overleaf 核心赛道，需明确差异化（纯本地、无服务端、P2P）

### AI 辅助（MCP / Agent）
- MCP 暴露项目上下文给 AI（comments / diag / git / 快照，PDF 除外）
- Agent CLI / 侧边栏
- ⚠️ 独立大方向，应作为独立模块，不污染核心写作体验
