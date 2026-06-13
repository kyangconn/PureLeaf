# goleaf — 待办事项

## ✅ 已完成

- [x] Wails 桌面端迁移（Go + Vue 3，直接函数绑定）
- [x] 集中式依赖注入（`internal/factory/`）
- [x] transport 层分离（`internal/transport/wails/`）
- [x] `pkg/` → `internal/` 重组
- [x] 文件落盘架构（`data/projects/{id}/`）
- [x] 补偿式项目创建（磁盘失败回滚 DB）
- [x] SCSS 变量 + mixin 体系
- [x] API transport 兼容层（已简化为 Wails-only）
- [x] 全局顶栏布局（BaseLayout 在 App.vue 持久化）
- [x] 项目列表从卡片改为紧凑列表
- [x] 编辑器 VS Code 风格暗色主题
- [x] "新建项目" 默认名"未命名项目"，直接进编辑器
- [x] 编辑器项目名双击重命名
- [x] 可复用 Loading 组件
- [x] IP 过滤中间件（已随 Web 模式删除，桌面端不需要）
- [x] 修复前端 axios 风格 `{ data }` 解构，改为 Wails 直接返回值
- [x] 顶栏增加设置入口与设置页面骨架

## 📁 文件版本管理

- [ ] 基础快照
  - 每次 `UpdateContent` 前 copy 旧文件到 `.goleaf/snapshots/{timestamp}-{filename}`
  - UI 展示快照列表 + diff 对比
- [ ] Git 集成
  - 新建项目时可选择 `git init`
  - 保存时自动 `git add -A && git commit -m "auto save"`
  - UI 展示 commit log
  - `config.yaml` 添加 `git.enabled` 和 `git.bin` 配置项

## 🧹 工程优化

- [x] 删除 `ProjectCard.vue`（已被 `ProjectList.vue` 替代）
- [x] 移除用户系统（User, Collaborator 相关代码全部删除）
- [x] 修复 Makefile 帮助文本
- [x] 移除 axios 依赖（纯 Wails 应用，不需要 HTTP 库）
- [ ] 测试（Go: `internal/service/` 单元测试；前端: Playwright E2E）
- [ ] GitHub Actions CI（build + lint）
- [ ] 应用图标（替换默认 Wails 图标）
- [ ] 统一前端错误处理：Wails 异常转换为 Element Plus message / 编译日志
- [ ] Wails 生成绑定检查：后端导出方法变更后自动或文档化运行生成命令
- [ ] 梳理并移除 Vite dev proxy 残留，避免误以为仍有 HTTP API
- [ ] Go service 增加路径安全校验，禁止 `..`、绝对路径、路径分隔符逃逸项目目录
- [ ] 文件创建 / 重命名增加同级重名校验与非法字符校验
- [ ] 文件操作补偿：DB 写入成功但磁盘写入失败时回滚元数据
- [ ] 编译中间产物清理策略（aux/log/out 等文件保留或隐藏）

## ✨ 功能

- [ ] 设置持久化
  - UI 修改 `latex.compiler`、`latex.timeout`
  - 保存到 `config.yaml` 或单独 settings 表
  - 运行时刷新 FileService 编译配置
- [ ] PDF 预览增强：放大/缩小、页码导航、下载按钮
- [ ] 多 Tab 编辑（同时打开多个 `.tex` 文件）
- [ ] LaTeX 语法高亮增强
- [ ] 自动补全（`\ref{}`、`\cite{}`、环境名）
- [ ] 项目模板（论文、简历、Beamer 幻灯片）
- [ ] 图片拖入自动 `\includegraphics`
- [ ] 暗色/亮色主题切换
- [ ] 最近打开文件 / 上次编辑位置恢复
- [ ] 未保存状态提示与手动保存按钮
- [ ] 文件树拖拽移动文件 / 文件夹
- [ ] 编译前选择主 `.tex` 文件
- [ ] 编译错误定位：从 log 跳转到对应文件行
- [ ] 项目导入 / 导出为 zip
