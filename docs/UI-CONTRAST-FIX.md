# StatPro Analytics — Phase 1B UI Contrast Fix

This revision keeps the Phase 1B source and the current Windows installer workflow/installer script supplied with the project, while improving the Qt UI theme styling.

## Changes
- Explicit light-theme text colors for labels, project title, tool buttons, inputs, tables, headers, tabs, dock titles, buttons, and status bar.
- Explicit dark-theme text colors for the same UI areas.
- Improved selected-cell and selected-tab contrast.
- Added the missing `QDockWidget` include in `MainWindow.cpp` so the source builds cleanly with Qt 6.
- Existing Windows installer workflow and Inno Setup configuration are preserved.

## Scope
This is a visual/readability revision. It does not yet implement the Phase 1C data-editor synchronization, undo/redo, or other new data-engine features.
