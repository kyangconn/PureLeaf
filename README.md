# PureLeaf

**本地优先、有安全感的 LaTeX 写作环境。**

PureLeaf 是一个跨平台桌面 LaTeX 编辑器，致力于提供专注、响应迅速、数据完全由你掌控的写作体验。它不是通用 IDE，也不以堆叠功能或实时协作为目标。

> 当前处于早期开发阶段。Windows / Linux 桌面端基于 Qt 6 构建；HarmonyOS 移动端通过 C ABI 共享核心逻辑。

---

## ✨ 特性

- **真正的本地优先** — 项目以普通文件夹形式存储在本地磁盘，随时可用其他工具打开
- **文件历史与快照** — 每次保存自动生成内容寻址快照，随时回溯任意版本
- **SyncTeX 双向定位** — 源码 ↔ PDF 精确跳转
- **无边框原生窗口** — Windows 11 圆角/阴影/Snap Layout，Linux 适配主流桌面环境
- **跨平台核心** — C++17 核心库通过 C ABI 供桌面 (Qt) 和移动端 (HarmonyOS) 共享
- **离线完整** — 不依赖网络服务，所有数据留在本地

---

## 🏗️ 架构

```
apps/                    ← 应用层（Qt Widgets / HarmonyOS ArkTS）
  desktop-qt/            ← Qt 6 桌面应用
  harmony/               ← HarmonyOS 移动应用
platform/                ← 平台抽象层
  desktop/               ← Windows / Linux 路径解析
  harmony/               ← HarmonyOS 平台适配（规划中）
capi/                    ← C ABI 稳定接口层（供 NAPI / FFI 消费）
core/                    ← 核心库（C++17，平台无关）
  include/pureleaf/      ← 公共头文件
  src/                   ← 实现
third_party/             ← 第三方依赖
  qwindowkit/            ← 无边框窗口（Apache 2.0）
  sqlite3/               ← SQLite amalgamation（Public Domain）
tests/                   ← 单元测试（GoogleTest）
```

## 🚀 快速开始

### 环境要求

| 组件 | 版本 |
|------|------|
| CMake | ≥ 3.21 |
| Qt 6 | ≥ 6.5（Core、Svg、Widgets） |
| C++ 编译器 | MSVC 2022 / GCC 11+ / Clang 14+ |
| 构建工具 | Ninja（推荐）或 Visual Studio |

### 构建（Windows / MSVC）

```powershell
# 配置（Debug）
cmake --preset x64-debug

# 构建
cmake --build --preset x64-debug

# 运行
out\build\x64-debug\apps\desktop-qt\desktop-qt.exe
```

### 构建（Linux）

```bash
# 安装 Qt 6 开发包（以 Ubuntu 为例）
sudo apt install qt6-base-dev libqt6svg6-dev

# 配置 & 构建
cmake --preset linux-debug
cmake --build --preset linux-debug

# 运行
out/build/linux-debug/apps/desktop-qt/desktop-qt
```

### 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `PURELEAF_BUILD_QT` | `ON` | 构建 Qt 桌面应用 |
| `PURELEAF_BUILD_TESTS` | `OFF` | 构建测试（会从 GitHub 下载 GoogleTest） |
| `PURELEAF_USE_BUNDLED_QWINDOWKIT` | `ON` | 使用 bundled QWindowKit 源码 |

```bash
cmake -B out -DPURELEAF_BUILD_TESTS=ON
```

---

## 📁 项目结构

```
PureLeaf/
├── CMakeLists.txt              # 根 CMake 配置
├── CMakePresets.json           # 构建预设（x64-debug、x64-release 等）
├── apps/desktop-qt/            # Qt 桌面应用源码
│   ├── main.cpp                # 入口
│   ├── mainwindow.*            # 主窗口（QWindowKit 无边框）
│   ├── pages/                  # 页面（home / editor / settings）
│   ├── components/             # UI 组件
│   └── resources/              # 资源文件（图标等）
├── core/                       # 核心库
│   ├── include/pureleaf/       # 公共头文件
│   └── src/                    # 实现
├── capi/                       # C ABI 层
├── platform/                   # 平台层
├── tests/                      # 测试
├── third_party/                # 第三方依赖
└── docs/                       # 文档
```

---

## 🤝 贡献

项目处于早期开发阶段，欢迎 Issue 和 PR。请先阅读 `todo.md` 了解当前路线图。

### 代码风格

- C++：Google 风格，4 空格缩进（见 `.clang-format`）
- Qt：遵循 Qt 命名约定（`camelCase` 成员、`Q_OBJECT` 宏）
- 本地格式化：运行 `make format` 自动修正；运行 `make lint` 只检查
- 提交信息：中文，格式 `类型: 简述`

---

## 📄 许可证

PureLeaf 本体使用 [MIT License](LICENSE)。

第三方组件的许可证见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

---

## 🙏 致谢

PureLeaf 建立在以下优秀开源项目之上：

- [Qt](https://www.qt.io/) — 跨平台应用框架
- [QWindowKit](https://github.com/stdware/qwindowkit) — 无边框窗口框架
- [SQLite](https://www.sqlite.org/) — 嵌入式数据库引擎
- [Lucide Icons](https://lucide.dev/) — 开源 SVG 图标集
- [GoogleTest](https://github.com/google/googletest) — C++ 测试框架
