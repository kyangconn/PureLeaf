# UI Blueprint

This document describes what PureLeaf should become, not the exact Qt implementation.

## Home

Purpose: workspace launcher.

- Left side: logo, welcome copy, New Blank Project, Open Local Folder.
- Right side: recent project list.
- When there are no recent projects: hide the right side and center the left content.
- Recent rows show: project name, root path, character count, Git status, more menu.

## Editor

Purpose: main writing surface.

- Top bar: back, project name, compile, sync, settings entry.
- Left pane: file tree, project outline, search.
- Center: LaTeX editor.
- Right pane: PDF preview.
- Bottom: compile log, error list, status.

## Settings

Purpose: global preferences.

- Compiler path and arguments.
- Theme, icons, window behavior.
- Editor preferences.
- About and version info.

## Navigation

- Home → Editor: create, open folder, open recent project.
- Editor → Home: return to launcher.
- Any page → Settings: global settings entry.
- Settings → Back: return to source page; fallback to Home.

## Rules

- Home does not contain editing features.
- Editor does not contain project creation wizards.
- Settings does not directly modify project content.
- Pages communicate intent through the app layer / navigator and do not own complex shared state.
