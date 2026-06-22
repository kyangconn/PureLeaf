# PureLeaf — 待办事项

> **产品定位**：专注、本地优先、有安全感的 LaTeX 写作环境。
> 不是通用 IDE，也不以堆叠功能或实时协作为目标。
> 当前仓库正处于 C++/Qt 与 HarmonyOS 架构落地阶段；旧版 Go/Wails 的完成情况不代表当前实现进度。

## 当前架构

| 路径 | 职责 | 当前状态 |
|---|---|---|
| `core/` | 平台无关的领域类型、存储、Diff、SyncTeX 等核心逻辑 | C++17 骨架，少量纯函数已实现 |
| `capi/` | 面向 HarmonyOS NAPI 和其他 FFI 的稳定 C ABI | 已暴露版本、坐标换算和 blob 相对路径 |
| `platform/desktop/` | Windows/Linux 桌面平台能力 | 已实现数据目录和临时目录解析 |
| `apps/desktop-qt/` | Qt 6 Widgets 桌面应用 | 页面导航与三栏编辑器布局骨架 |
| `apps/harmony/` | HarmonyOS ArkTS + NAPI 应用 | 仍为 Hello World/NAPI 示例 |
| `third_party/qwindowkit/` | 桌面无边框窗口与标题栏美化 | 已接入原生窗口代理和自定义标题栏 |
| `tests/` | GoogleTest 测试 | 当前只有跨层链接和纯函数 smoke tests |

### 构建入口

- 根目录 `CMakeLists.txt`：完整工程入口，可选择构建 Qt 应用和测试。
- `apps/desktop-qt/CMakeLists.txt`：Qt Creator 独立入口；会按需引入 `core`、桌面平台层和 bundled QWindowKit。
- HarmonyOS 由 DevEco Studio/Hvigor 独立构建，不进入桌面 CMake superbuild。

### CMake 选项

| 选项 | 默认值 | 说明 |
|---|---:|---|
| `PURELEAF_BUILD_QT` | `ON` | 构建 Qt 桌面应用 |
| `PURELEAF_BUILD_TESTS` | `OFF` | 构建测试；首次配置会下载 GoogleTest |
| `PURELEAF_USE_BUNDLED_QWINDOWKIT` | `ON` | 使用 `third_party/qwindowkit` 源码 |

## 已落地能力

- [x] `core` / `capi` / `platform` / `apps` 分层及 CMake 目标关系
- [x] Core 保持 C++17，桌面应用使用 C++20
- [x] SHA-256 blob 路径分桶规则：`{hash[:2]}/{hash}`
- [x] SyncTeX big point 与 PDF CSS 像素的正向坐标换算
- [x] Windows/Linux 用户数据目录和临时目录解析
- [x] C ABI 的版本、坐标换算和 blob 路径接口
- [x] Qt Home / Editor / Settings 页面与集中式 Navigator
- [x] 编辑器页面的文件树 / 编辑器 / PDF 三栏布局骨架
- [x] QWindowKit bundled 源码构建与桌面目标链接
- [x] QWindowKit 无边框拖拽、系统按钮、Windows 11 圆角/阴影和 Snap Layout 基础接入
- [x] 基础 smoke tests

## 当前缺口

这些能力目前只有类型、接口或 UI 占位，不应视为已经实现：

- [ ] 明确 `Workspace` 与 `Project` 的产品边界；当前代码仍以 `Project` 为根类型
- [ ] 打开本地文件夹、最近工作区和工作区元数据
- [ ] 安全的相对路径校验、文件树读取、原子保存和外部文件变化处理
- [ ] SHA-256 计算、blob 实际读写、索引和垃圾回收
- [ ] 文件历史、项目快照、版本还原和事务边界
- [ ] Myers 行级 Diff；当前 `computeDiff()` 返回空结果
- [ ] LaTeX 编译进程、主文件选择、超时、中间产物和错误定位
- [ ] SyncTeX CLI 输出解析、反向搜索及桌面 UI 联动
- [ ] 可编辑文本控件、自动保存、未保存状态和编辑位置恢复
- [ ] PDF 渲染、缩放、页码及正反向定位
- [ ] 设置读取、持久化和运行时刷新
- [ ] 标题栏主题切换、图标资源和可选 Mica/Acrylic 材质
- [ ] HarmonyOS 链接 `pureleaf_capi`，替换示例 NAPI 接口

## 近期路线

### 1. 先闭合最小写作循环

- [ ] 从首页打开本地工作区
- [ ] 展示文件树并打开单个 `.tex` 文件
- [ ] 编辑、保存并处理外部修改冲突
- [ ] 选择主文件并调用本地 LaTeX 编译器
- [ ] 展示 PDF 和编译日志

### 2. 补足安全感

- [ ] 路径防穿越与平台文件名校验
- [ ] 保存前后生成内容哈希与 revision
- [ ] 删除、覆盖和恢复前创建 snapshot
- [ ] 实现 Diff 与 RestoreRevision
- [ ] 为文件系统、存储和恢复流程增加真实单元测试

### 3. 完成桌面体验

- [x] 正式接入 QWindowKit 无边框窗口
- [ ] 统一导航栏、窗口栏和三栏编辑布局的视觉规范
- [ ] 设置页持久化编译器、超时、主题和工作区偏好
- [ ] 最近工作区与上次编辑位置恢复
- [ ] Windows 部署 Qt 与 QWindowKit 运行时依赖

### 4. 再推进 HarmonyOS

- [ ] 将 `core`/`capi` 纳入 HarmonyOS CMake
- [ ] 用真实 C API 替换 `add(2, 3)` 示例
- [ ] 先复用文件、Diff、快照等纯核心能力，再设计移动端界面

## 测试与质量

- [ ] 为 SHA-256 与 blob I/O 增加测试
- [ ] 为 Diff 的插入、删除、修改和 hunk 合并增加测试
- [ ] 为路径校验加入 Windows、Linux 与穿越攻击用例
- [ ] 为 Navigator 和页面进入/离开生命周期增加 Qt Test
- [ ] 增加根工程的 MSVC/MinGW 构建检查
- [ ] 增加 HarmonyOS C API 链接检查

## 远期探索

### 类 Git 的局域网异步协作

- 局域网发现、manifest 交换、变更集拉取/推送和人工冲突处理
- 保持本地文件系统为真实数据源，不引入中心服务或实时 OT/CRDT
- 等工作区、快照和 Diff 边界稳定后再确定协议与仓库拆分方式

### AI / MCP

- 通过独立边界暴露文件上下文、诊断、历史和快照
- 不让 Agent 能力侵入核心写作主循环

## 明确暂不做

- 实时多人协作、账号系统和云端托管
- 自研 LaTeX 自动补全或完整 LSP
- 为了扩展性而提前引入复杂插件系统
- 在真实应用边界出现前进行大规模仓库重组
