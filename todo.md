# PureLeaf — 开发路线图

## ✅ 已完成

- [x] C++ 项目骨架：CMake + Ninja + MSVC，C++17(core) / C++20(app)
- [x] 分层架构：`core/`（领域模型）+ `capi/`（C ABI）+ `platform/`（平台）+ `apps/`（UI）
- [x] 领域模型定义：Project、FileNode、Revision、Result<T>、Error 枚举
- [x] BlobStorage 路径分片（SHA-256，{hash[:2]}/{hash}）
- [x] SyncTeX 正向坐标转换（synctex bp → pdf.js px）
- [x] C API 层（version、synctex、blob path，供 HarmonyOS NAPI 消费）
- [x] 桌面平台路径解析（Windows %LOCALAPPDATA%、Linux XDG）
- [x] Qt 6 桌面壳：QWindowKit 无边框窗口 + 自定义标题栏
- [x] 三页面导航骨架（Home / Editor / Settings，QStackedWidget + Navigator）
- [x] IDE 风格启动首页：新建/打开入口、最近项目列表与 Git 状态展示接口
- [x] 语义化图标层：Linux 系统/Breeze 图标优先，Lucide SVG 按需打包回退
- [x] GoogleTest 冒烟测试（架构联通验证）
- [x] clang-format（Google 风格，4 空格缩进）
- [x] CMakePresets（x64-debug/release、x86-debug/release）
- [x] QWindowKit submodule（third_party/qwindowkit）
- [x] Windows 开发构建自动部署 Qt/QWindowKit 运行时依赖

---

## 🧠 核心功能

### 1. 存储与项目基础

- [ ] SHA-256 实现
  - vendor 一个单头文件 SHA-256 到 `third_party/`，或内联到 `core/src/storage.cpp`
  - `BlobStorage::sha256Hex()` 通过测试
- [ ] BlobStorage 读写
  - `writeBlob(content) → hash`：写入 blob 文件，返回 SHA-256
  - `readBlob(hash) → content`：从分片目录读取
  - `exists(hash) → bool`
  - `deleteBlob(hash)`（供 GC 使用）
- [ ] 项目仓库（ProjectRepository）
  - SQLite 存储（通过 `core/` 内 vendor 的 SQLite 或 JSON 文件方案）
  - CRUD：Create(name) / FindByID / FindAll / Update / Delete
  - 磁盘目录：`{userDataDir}/projects/{id}/`
- [ ] 文件仓库（FileRepository）
  - 邻接表树结构（parent_id、name、is_dir）
  - CRUD + FindByProjectID + FindByParentID
  - 文件内容从磁盘读取，不存数据库
  - 路径计算（从根到节点的相对路径拼接）
- [ ] 项目服务（ProjectService）
  - Create：创建项目目录 + 写入 `main.tex` 模板 + 写入 DB 元数据
  - 补偿式创建：磁盘失败 → 回滚 DB
  - Delete：级联删除文件 + 清理磁盘
  - Update（重命名）
- [ ] 文件服务（FileService）
  - GetTree：扁平列表 → 树结构
  - GetContent / UpdateContent：磁盘读写 + 原子写
  - Create / Rename / Delete：DB 元数据 + 磁盘操作
  - 路径安全校验：拒绝 `..`、绝对路径、非法字符、同级重名
  - 递归删除子孙节点
- [ ] 项目锁（ProjectLockManager）
  - 从 Go 版 port 到 C++：`std::mutex` per project，序列化同一项目的并发操作

### 2. 文件版本管理

- [ ] 基础快照
  - 文件保存前写入 app 内部快照 blob：`.pureleaf/snapshots/blobs/{sha256}`
  - SQLite 记录 snapshot metadata：project_id、file_path、content_hash、blob_path、size、reason、created_at
  - 后台 GC：清理 orphan blob 和过期快照（按时间和数量限额）
- [ ] Diff 算法
  - port Go 版 Myers diff 到 `core/src/diff.cpp`
  - 输出 `DiffHunk` 列表，供 UI diff 对比展示
- [ ] Git 集成（远期）
  - 新建项目可选 `git init`
  - `GitService` 封装 libgit2 或系统 git CLI：init/status/log/diff/add/commit/restore
  - 保存流程中 Git commit 是后置同步动作，失败不回滚文件保存
  - 手动 commit 优先，自动 commit 后置
- [ ] 版本事务边界
  - checkout / restore / reset 等会改工作区的操作，必须先生成 app snapshot
  - Git、History、FileService 共享 ProjectLockManager

### 3. LaTeX 编译

- [ ] 编译服务（CompileService）
  - 从设置读取 compiler（pdflatex/xelatex/lualatex）和 timeout
  - `QProcess` 异步调用编译器，实时捕获 stdout/stderr
  - 查找主文件：优先 `main.tex`，否则第一个 `.tex`
  - 返回：PDF 路径 + 编译日志
- [ ] 编译日志解析
  - 解析 LaTeX 错误/警告格式（`! `、`l.xxx`、`Warning: `）
  - 结构化输出：文件、行号、级别、消息
- [ ] 辅助文件管理
  - 编译产物（.aux/.log/.out/.toc 等）写到项目目录下的 `build/` 子目录
  - `-output-directory` 参数隔离
  - clean build：清理辅助文件重新编译
- [ ] SyncTeX 反向搜索
  - 解析 `.synctex.gz` 文件，从 PDF 坐标反查源码位置
  - `synctexReverse(page, x, y) → {file, line, column}`
  - UI 集成：PDF 点击 → 编辑器跳转

### 4. LSP 集成（远期）

- [ ] texlab LSP 进程管理
  - 从 PATH 查找 texlab，或由设置页指定路径
  - 项目维度维护 LSP session
  - JSON-RPC over stdio：initialize、didOpen、didChange、didSave
- [ ] 第一阶段：diagnostics + completion
  - publishDiagnostics → 编辑器波浪线
  - completion → 自动补全弹窗（`\ref{}`、`\cite{}`、环境名）
- [ ] LTeX LS 写作检查（可选，独立配置）
  - 拼写、语法、自然语言检查
  - 与 texlab 分开配置

---

## 🎨 UI（Qt 桌面端）

### 1. 页面架构与启动首页

保留 Home 作为稳定的工作区启动器；Editor 专注写作，Settings 从全局标题栏进入：

- [x] Home 左侧操作区：新建空白项目、打开本地文件夹
- [x] Home 右侧最近项目列表组件
  - 项目名、根路径、字符数
  - Git 状态：未检测 / 无仓库 / 干净 / 有更改 / 冲突
  - 更多菜单：打开、从最近项目移除
  - 无数据时显示空状态，不注入演示项目
- [x] 设置入口移到 QWindowKit 全局标题栏
- [ ] RecentProjectRepository
  - 持久化项目 ID、名称、根路径、最后打开时间
  - 打开 Home 时加载并调用 `HomePage::setRecentProjects()`
  - 移除项目时同步更新持久化数据
- [ ] 项目统计服务
  - 后台计算 `.tex` / `.bib` 等文本文件字符数
  - 后台检测 Git 状态，避免阻塞 UI 线程
- [ ] 首次启动 Setup 向导
  - 检测 LaTeX 发行版和默认编译器
  - 选择主题与界面语言
- [ ] Editor 工具栏
  - 左侧：返回首页、当前项目名和文件面包屑
  - 右侧：主文件、编译、日志等写作上下文操作

### 2. 编辑器核心

- [ ] 文件树面板（QTreeView）
  - 自定义 model（从 FileService::GetTree 获取数据）
  - 右键菜单：新建文件/文件夹、重命名、删除、复制路径
  - 拖拽移动文件/文件夹
  - 文件图标（.tex / .bib / .pdf / 文件夹等）
  - 排序：文件夹优先，按名称排序
- [ ] 代码编辑器
  - QPlainTextEdit 或 QScintilla 作为基础控件
  - LaTeX 语法高亮（QSyntaxHighlighter 子类实现）
  - 行号显示
  - Tab 缩进（2 空格或 4 空格可配置）
  - 括号/大括号匹配
  - 当前行高亮
  - 自动保存（2 秒 debounce）
  - 未保存状态指示（标题栏加 `●` 标记）
  - Ctrl+S 手动保存
- [ ] PDF 预览面板
  - 嵌入 pdf.js 通过 QWebEngineView，或使用 Qt PDF 模块
  - 页码导航（上一页/下一页/跳转）
  - 缩放（Ctrl+滚轮、按钮）
  - SyncTeX 正向搜索：编辑器光标位置 → PDF 高亮对应位置
  - SyncTeX 反向搜索：PDF 点击 → 编辑器跳转源码行
  - 下载/外部打开按钮

### 3. 设置面板

作为编辑器内的常驻二级界面（右侧抽屉 `QDrawer` 或弹出面板）：

- [ ] 设置面板 UI 骨架
  - 分类导航（编译 / 编辑器 / 外观 / 快捷键 / 关于）
  - 表单布局（QFormLayout + QScrollArea）
  - 保存/取消/应用 按钮
- [ ] 编译设置
  - 编译器路径（QLineEdit + 浏览按钮，默认从 PATH 检测）
  - 编译器类型下拉（pdflatex / xelatex / lualatex）
  - 编译超时（QSpinBox，5-600 秒）
  - 编译输出目录（默认项目内 `build/`）
  - BibTeX 编译器路径
  - 编译链（latex + bibtex + latex + latex 次数可配置）
- [ ] 编辑器设置
  - 字体族（QFontComboBox）
  - 字号（QSpinBox，8-36）
  - Tab 宽度（2/4/8）
  - Tab 转空格开关
  - 自动保存开关 + 延迟（500-10000ms）
  - 自动补全开关
  - 行号显示开关
  - 自动换行开关
  - 括号自动闭合开关
- [ ] 外观设置
  - 应用主题（亮色 / 暗色 / 跟随系统）
  - 编辑器配色方案（内置几种：VS Code Dark、Monokai、Solarized 等）
  - 界面语言（中文 / English）
  - 字体大小（全局缩放）
- [ ] 快捷键设置（远期）
  - 快捷键表：编译(F5)、保存(Ctrl+S)、查找(Ctrl+F)、跳转行(Ctrl+G) 等
  - 自定义快捷键：点击条目 → 按下新组合键 → 冲突检测
  - 导入/导出快捷键配置
- [ ] 设置持久化
  - 读写 `{userDataDir}/config.json`（或 YAML）
  - 启动时加载，运行时通过 Settings 服务统一访问
  - 修改设置后通知相关服务热刷新（编译器、字体等）

### 4. 快捷键体系（远期）

- [ ] 快捷键注册框架
  - `ShortcutManager`：注册/注销/查询/冲突检测
  - 支持多上下文（全局 / 编辑器 / 文件树 / PDF 面板）
  - 每个 action 有唯一 ID、默认快捷键、可本地化名称
- [ ] 默认快捷键
  - `Ctrl+N` — 新建项目
  - `Ctrl+O` — 打开项目
  - `Ctrl+S` — 保存当前文件
  - `Ctrl+Shift+S` — 全部保存
  - `Ctrl+W` — 关闭当前 Tab
  - `Ctrl+Shift+W` — 关闭项目
  - `F5` — 编译
  - `Ctrl+F` — 查找
  - `Ctrl+H` — 查找替换
  - `Ctrl+G` — 跳转到行
  - `Ctrl+P` — 快速打开文件
  - `Ctrl+B` — 切换文件树
  - `Ctrl+J` — 切换 PDF 面板
  - `Ctrl+,` — 打开设置
  - `Alt+Left/Right` — 前进/后退导航
  - `Ctrl+Tab` / `Ctrl+Shift+Tab` — 切换文件 Tab

### 5. 功能体验

- [ ] 多 Tab 编辑
  - QTabWidget / QTabBar 管理打开的多个文件
  - Tab 右键菜单：关闭、关闭其他、关闭所有、复制路径
  - Tab 拖拽排序
  - 未保存 Tab 显示圆点标记
- [ ] 项目模板
  - 内置模板：空白文章、CTeX 论文、Beamer 幻灯片、简历
  - 新建项目时选择模板
  - 用户自定义模板（保存到 `{userDataDir}/templates/`）
- [ ] 图片处理
  - 拖入图片自动生成 `\includegraphics{}` 代码
  - 图片复制到项目目录下的 `figures/` 子目录
  - 支持格式：png、jpg、pdf、eps
- [ ] 最近文件与位置恢复
  - 关闭项目时记录最后打开的文件和光标位置
  - 下次打开项目时恢复到上次状态
- [ ] 项目导入/导出
  - 导入：解压 zip → 创建项目
  - 导出：项目目录 → zip
- [ ] 错误定位
  - 编译日志面板：点击错误行 → 编辑器跳转到对应文件和行
  - 错误/警告行高亮标记（编辑器 gutter 中显示图标）

### 6. 主题与图标体系

- [x] 统一语义图标接口
  - 页面只依赖“新建 / 打开 / 设置 / 返回”等语义，不直接依赖图标文件名
  - Linux 优先通过 `QIcon::fromTheme()` 使用当前桌面图标主题（KDE 下通常为 Breeze）
  - 系统主题缺少图标时回退到内置 Lucide SVG；Windows 当前统一使用 Lucide
- [ ] 主题系统升级为设计令牌
  - 把颜色、圆角、间距、字号和控件状态从页面 QSS 提取为统一主题配置
  - 支持亮色 / 暗色 / 跟随系统，并向图标层提供语义前景色
  - 设置页提供界面主题与图标主题选择：自动 / 系统 / Lucide
- [ ] Linux Breeze 搭配
  - 评估接入 BreezeStyleSheets 作为可选控件风格，不覆盖业务页面布局
  - 在 KDE/Breeze 环境验证系统图标、亮暗色、Wayland 和高 DPI
  - AppImage 环境未安装图标主题时保持 Lucide 回退，不捆绑整套 Breeze 图标
- [ ] Windows Fluent 搭配
  - 调研可用于 Qt Widgets 的 Fluent `QStyle` / QSS；避免为了主题引入 WinUI 3 或迁移 Qt Quick
  - 若无合适依赖，基于现有 QWindowKit + 设计令牌维护轻量 Fluent 风格控件层
  - 对齐 Windows 11 的圆角、状态动画、Mica/Acrylic 与系统强调色

---

## 🔧 平台适配

### Windows

- [ ] MSVC 构建流程文档化
  - 环境要求：Qt 6.11+、Visual Studio 2022、CMake 3.21+
  - `cmake --preset x64-debug && cmake --build --preset x64-debug`
- [ ] NSIS 安装包
  - 利用 Qt Installer Framework 或 NSIS 打包
  - 开始菜单快捷方式、卸载入口
  - `.tex` 文件关联（可选注册）
- [ ] Windows 11 适配
  - Snap Layout 支持（通过 `QWindowKit` 已部分支持）
  - 亚克力/Mica 背景效果（Qt 设置 `windowAttribute`）

### Linux

- [ ] Linux 构建流程
  - 依赖：Qt 6.5+、GCC 11+ / Clang 14+
  - `cmake --preset linux-debug`
- [ ] AppImage 打包
  - `linuxdeployqt` 收集依赖
- [ ] Wayland 兼容测试
  - 无边框窗口在 Wayland 下的行为

### macOS

- [ ] macOS 构建流程
  - `cmake --preset macos-debug`
  - Qt 通过 Homebrew 或官方安装包
- [ ] App Bundle 打包
  - `macdeployqt` 生成 `.app`
  - Info.plist 配置
- [ ] DMG 分发
  - 创建 `.dmg` 安装镜像

### HarmonyOS

- [ ] NAPI 绑定层
  - 通过 `capi/` 的 C ABI 为 ArkTS 提供接口
  - 映射 core 的 Project / File / Compile 能力
- [ ] ArkUI 界面
  - 编辑器页面（文件树 + 代码编辑 + PDF 预览）
  - 设置页面
  - 与 Qt 桌面端共享 `core/` 和 `capi/`

---

## 🧹 工程质量

- [ ] 单元测试
  - core：storage、diff、synctex、project/file service 全覆盖
  - 测试从 `tests/` 下按模块组织
- [ ] CI（GitHub Actions）
  - Windows x64（MSVC + Qt）build
  - Linux（GCC）build
  - clang-format 检查
  - Googletest 运行
- [ ] 应用图标
  - 设计 PureLeaf 品牌图标
  - 生成 .ico（Windows）、.icns（macOS）、PNG 多尺寸
- [ ] 国际化（i18n）
  - 所有用户可见字符串走 `tr()`
  - 首批语言：zh_CN、en_US
  - 翻译文件（.ts）持续维护
- [ ] 错误处理统一
  - core 层返回 `Result<T>` 或抛异常（约定一种）
  - UI 层统一错误弹窗/Toast 组件
- [ ] 编译中间产物管理
  - 编译输出隔离到 `build/`，不污染项目根目录
  - clean 命令清理 aux/log/out 等
