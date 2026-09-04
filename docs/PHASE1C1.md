# StatPro Analytics — Phase 1C.1 Data Editor Hardening

This maintenance release fixes the Phase 1C data-editor controls and makes them directly accessible from dedicated toolbars.

## Fixed
- Variable Editor reliably updates name, label, type, format, and missing-value metadata.
- Variable type changes validate against the NEW type and report the exact failing row.
- Copy/Paste actions are on the dedicated Data Editor toolbar and support Ctrl+C/Ctrl+V.
- Pasting can automatically append rows when needed.
- Add Row / Insert Row / Delete Row(s) are on the dedicated Data Editor toolbar.
- Sort Ascending / Descending are on the dedicated Data Editor toolbar.
- Double-clicking a variable opens Variable Editor.
- Project reopening uses the correct DataSet row insertion method.
- Sorting comparator was corrected to provide stable ascending/descending ordering.
