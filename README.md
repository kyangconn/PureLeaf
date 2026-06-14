# goleaf

goleaf 是一个轻量级桌面 LaTeX 编辑器，基于 Wails v3、Go 和 Vue 3 构建。

项目目标是提供一个本地优先的 LaTeX 写作环境：项目文件直接保存在磁盘，桌面端通过 Go 后端管理项目、文件和编译流程，前端提供编辑、文件树和 PDF 预览体验。

## 功能

- 桌面端 LaTeX 项目管理
- 项目列表、创建、重命名和删除
- 文件树管理，支持文件和文件夹的创建、重命名、删除
- CodeMirror 编辑器
- 自动保存
- LaTeX 编译并预览生成的 PDF
- 全局浅色 / 深色主题切换
- 编辑器独立浅色 / 深色主题切换
- 无边框桌面窗口

## 运行

```bash
wails3 dev
```

## 构建

```bash
wails3 build
```

## 配置

`config.yaml`：

```yaml
database:
  path: ""

latex:
  compiler: pdflatex
  timeout: 60
```

`database.path` 留空时使用系统用户配置目录。`latex.compiler` 可以使用 `pdflatex`、`xelatex`、`lualatex` 或编译器的完整路径。

## 开发文档

架构说明、开发命令、目录结构和实现约束见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## Special Thanks
