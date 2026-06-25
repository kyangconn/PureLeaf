# Feature Contracts

This document helps humans and AI decide where a feature belongs and what must not be changed.

## Create Blank Project

Input:

- parent directory
- project name

Behavior:

- Create the project directory.
- Write a default `main.tex` if missing.
- Register the project.
- Update recent projects.
- Open Editor.

Do not:

- Write multiple stores directly from `MainWindow`.
- Treat QSettings and SQLite as two sources of truth.
- Silently overwrite a non-empty directory.

## Open Local Folder

Input:

- local directory path

Behavior:

- Validate that the directory exists.
- Register it as a project or recent project if needed.
- Update recent projects.
- Open Editor.

Do not:

- Block opening because Git status detection failed.
- Freeze the UI while scanning or indexing a large directory.
- Copy user files into the app data directory.

## Recent Projects

Behavior:

- Sort by last opened time.
- Show project name, path, character count, Git status.
- Removing a recent project only affects the list; it does not delete files.

Do not:

- Treat “remove from recent” as project deletion.
- Keep a second persistent recent list inside UI widgets.

## Git Status

Behavior:

- Read-only detection.
- Support Clean, Modified, Conflict, NoRepository, Unknown.
- Return Unknown on detection failure.

Do not:

- Run Git commands from Home that modify the repository.
- Block project opening because Git is unavailable.

## Settings

Behavior:

- Store user preferences.
- Do not store project business data.

Do not:

- Use QSettings for project metadata, file indexes, or history snapshots.
