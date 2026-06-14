# Contributing

This document describes the current architecture, development commands, and implementation boundaries for goleaf.

## Stack

| Layer | Technology |
| ---- | ---- |
| Desktop | Wails v3 |
| Backend | Go, GORM, SQLite |
| Frontend | Vue 3, TypeScript, Vite |
| UI | Element Plus |
| Editor | CodeMirror 6 |
| State | Pinia |
| Router | Vue Router |
| Styles | SCSS |

## Commands

```bash
wails3 dev       # Desktop development mode
wails3 build     # Production desktop build
go build ./...  # Compile Go only
go test ./...   # Run Go tests
```

Wails bindings must be regenerated after backend method changes. From the
project root:

```bash
wails3 generate bindings -clean=true -ts -d web/bindings
```

Frontend commands must be run from `web/` and must use pnpm:

```bash
pnpm dev
pnpm build
pnpm lint
```

## Project Layout

```text
main.go                         Wails entrypoint, embeds web/dist
internal/
  config/                       Viper configuration + platform data dir resolution
  database/                     SQLite initialization
  domain/                       Domain models (project, file, file_revision, project_snapshot)
  factory/                      Single dependency initialization point
  log/                          Logging
  repository/                   GORM data access
  service/                      Business logic + blob store + path guard + diff
  bindings/                     Wails v3 method bindings
web/
  src/
    api/                        Frontend API wrapper over Wails bindings
    components/                 Reusable Vue components
    layouts/                    Shell layout and title bar
    router/                     Vue Router routes
    stores/                     Pinia stores
    styles/                     Global SCSS and theme variables
    views/                      Page views
  bindings/                     Generated Wails bindings
```

On-disk layout under `dataRoot`:

```text
{dataRoot}/
  goleaf.db                     SQLite metadata + history indexes
  projects/{id}/                project working files (real source of truth)
  .backup/blobs/{hash[:2]}/{hash}   content-addressed snapshot storage
  downloads/texlive/            cached TeX Live installers
```

## Backend Boundaries

- `internal/factory/factory.go` is the only dependency initialization point.
- `internal/bindings/` must stay thin. It should forward arguments to services and adapt return values for Wails.
- `internal/service/` owns business logic.
- `internal/repository/` owns database access.
- `internal/domain/` contains domain models.
- Project files are stored on disk under `{dataRoot}/projects/{id}/`.
- `dataRoot` resolves to the user data directory (not the config directory):
  - Linux: `$XDG_DATA_HOME/goleaf`, fallback `~/.local/share/goleaf`
  - macOS: `~/Library/Application Support/goleaf`
  - Windows: `%LOCALAPPDATA%/goleaf`, fallback `%APPDATA%/goleaf`
- `dataRoot` is `filepath.Dir(config.Database.Path)` (the `.db` lives there).
- File content snapshots are content-addressed under `{dataRoot}/.backup/blobs/{hash[:2]}/{hash}`.
- SQLite stores file and directory metadata only, plus `file_revisions` /
  `project_snapshots` indexes for history. Do not store file content in SQLite.
- Desktop app is single user. Do not add auth, JWT, roles, or multi-user ownership concepts.

## Frontend Boundaries

- Frontend root is `web/`.
- Use Element Plus and `@element-plus/icons-vue`; do not add another UI framework.
- Components should use `<style lang="scss" scoped>`.
- Use `@` for `web/src/`.
- Use `@bindings` for generated Wails bindings.
- Components should import APIs from `web/src/api/index.ts`.
- Do not call `window.go...` directly in Vue components.
- Do not add axios or HTTP API calls for app business logic.
- Do not manually edit `web/bindings/`; regenerate Wails bindings after backend method changes.

## Data Flow

```text
Vue components
  -> web/src/api/index.ts
  -> Wails generated bindings
  -> internal/bindings/
  -> internal/service/*
  -> internal/repository/*
  -> SQLite metadata + disk files
```

## Window Shell

The app uses a Wails frameless window by setting `Frameless: true` in `main.go`.

The custom title bar in `web/src/layouts/BaseLayout.vue` is responsible for:

- marking the draggable region with `--wails-draggable: drag`
- excluding interactive controls with `--wails-draggable: no-drag`
- calling Wails runtime window actions for minimize, maximize/restore, and quit

## Theme Management

Theme state is centralized in `web/src/stores/theme.ts`.

- `appTheme` controls the application chrome and ordinary pages.
- `editorTheme` controls the editor workspace and CodeMirror theme.
- Both values are persisted in `localStorage`.
- Theme colors are exposed through CSS variables in `web/src/styles/index.scss`.

## Configuration

`config.yaml`:

```yaml
database:
  path: ""

latex:
  compiler: pdflatex
  timeout: 60
```

Environment variable prefix: `GOLEAF_`.

Examples:

- `GOLEAF_DATABASE_PATH`
- `GOLEAF_LATEX_COMPILER`
- `GOLEAF_LATEX_TIMEOUT`

## Style

- Go code must be `gofmt` formatted.
- Vue and TypeScript code should pass ESLint and Prettier.
- Keep changes scoped to the requested behavior.
- Prefer existing patterns over new abstractions unless the abstraction removes real complexity.
