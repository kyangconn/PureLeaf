# App Architecture Contract

This document defines boundaries and data ownership. It exists to prevent UI code, light logic, SQLite, QSettings, JSON, and fallback stores from becoming multiple sources of truth.

## Layers

| Layer | Responsibility |
|---|---|
| Qt UI | Rendering, user input, page navigation. |
| Desktop App Layer | Desktop page orchestration, launcher flows, light interaction logic. |
| Core Services | Project, file, compile, snapshot, and history business rules. |
| Repositories / Ports | SQLite, filesystem, Git, compiler, and platform path adapters. |
| Storage | SQLite, project folders, QSettings, cache directories. |

## Data Ownership

| Data | Source of truth | Notes |
|---|---|---|
| Project metadata | SQLite / ProjectService | name, rootPath, mainTex, updatedAt, etc. |
| File content | Filesystem / FileService | `.tex`, `.bib`, etc. are not duplicated into SQLite. |
| Recent projects | RecentProjectService | UI must not write this directly. |
| User preferences | QSettings / SettingsService | Theme, window size, compiler path, etc. |
| Build artifacts | Cache or project build directory | Disposable and rebuildable. |
| Git status | GitStatusProvider | Read-only; failures become Unknown. |

## Rules

- UI must not write SQLite, JSON, or QSettings directly.
- Each data type has exactly one source of truth.
- Fallbacks live at boundaries, not inside page code.
- Do not double-write for safety; write migrations when storage changes.
- Temporary implementations may be simple, but must sit behind a service or repository.

## Current Transition

The desktop Home page still contains light launcher logic. It should move into a Desktop App Layer service such as `ProjectLauncherService`, which can then call core services or a temporary storage implementation.
