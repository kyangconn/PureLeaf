# 应用架构契约

本文只定义边界和数据归属，避免 UI、轻逻辑、SQLite、QSettings、JSON 等存储到处双写。

## 分层

| 层 | 职责 |
|---|---|
| Qt UI | 展示、用户输入、页面导航。 |
| Desktop App Layer | 桌面端页面编排、启动器、轻量交互逻辑。 |
| Core Services | 项目、文件、编译、快照、历史等业务规则。 |
| Repositories / Ports | SQLite、文件系统、Git、编译器、平台路径等外部能力封装。 |
| Storage | SQLite、项目文件夹、QSettings、cache 目录。 |

## 数据归属

| 数据 | 权威来源 | 说明 |
|---|---|---|
| 项目元数据 | SQLite / ProjectService | name、rootPath、mainTex、updatedAt 等。 |
| 文件正文 | 文件系统 / FileService | `.tex`、`.bib` 等正文不复制进 SQLite。 |
| 最近项目 | RecentProjectService | UI 不直接写；实现可先薄，入口要统一。 |
| 用户偏好 | QSettings / SettingsService | 主题、窗口尺寸、编译器路径等。 |
| 编译产物 | cache 或项目 build 目录 | 可删除、可重建，不作为业务真相。 |
| Git 状态 | GitStatusProvider | 只读状态，失败时返回 Unknown。 |

## 规则

- UI 不直接写 SQLite、JSON、QSettings。
- 同一种数据只能有一个权威来源。
- fallback 只能出现在边界层，不散落在页面代码里。
- 不为“保险”做双写；需要迁移时写明确迁移逻辑。
- 临时实现可以简单，但必须藏在 service/repository 后面。

## 当前过渡状态

桌面首页目前仍有轻量 launcher 逻辑。后续应收敛到 Desktop App Layer，例如 `ProjectLauncherService`，再由它调用 core 或临时存储实现。
