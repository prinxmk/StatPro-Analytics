# StatPro Analytics — Phase 1C3

Version marker: **0.3.3**.

This UI refinement build keeps the Phase 1C2 data-editor functionality and reorganizes the interface for more workspace.

## Changes

- Removed the duplicate project/file name label above the Data Editor. The active project name remains in the Windows title bar.
- Replaced the wide multi-toolbar layout with compact drop-down menus: File, Data, Analysis and View.
- Analysis procedures are grouped as submenus under Analysis, ready for later statistical-engine actions.
- Moved Data Editor Commands into a dockable panel on the right, stacked below Properties.
- The Data Editor Commands panel can be closed and restored from View > Data Editor Commands.
- Variables / Elements and Properties docks can also be shown/hidden from View.
- Added a persistent bottom status bar showing Ready/Modified state, observation count, variable count, selected-cell count and Offline state.
- Preserved light/dark themes and the existing Phase 1C2 data-management behavior.

## Layout goal

The central Data Editor now receives more horizontal and vertical workspace while high-frequency commands remain accessible from the right-side command dock and the Data menu.
