# StatPro Analytics — Phase 1C

Phase 1C upgrades the Phase 1B grid into a functional data-management workspace.

## Included

- Direct cell editing synchronized with the underlying DataSet.
- Type validation for Numeric, Date, and Boolean variables.
- Variable Editor for name, label, type, format, and missing-value metadata.
- Add, insert, and delete rows.
- Extended multi-cell selection.
- Clipboard copy/paste using tab-delimited data.
- Cell edit undo/redo.
- Column sorting ascending/descending from the active column.
- Dataset information view with missing-cell counts.
- Missing-value indicators in the grid and Properties panel.
- Row filtering with text search and simple expressions such as `age > 30`, `sex == Male`, and `income >= 1000`.
- Improved CSV import with basic type inference and unique variable names.
- Improved status bar showing observations and variables.
- Light/dark theme support retained.

## Deliberately deferred

Excel XLSX import/export, full command-history undo for every structural operation, true frozen panes, advanced multi-condition filter builder, large-dataset virtualization, and statistical procedures are planned for subsequent phases.

The statistical engine remains button-driven; no command prompt is introduced.
