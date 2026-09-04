# StatPro Analytics — Phase 1C.2 stabilization build

Version marker: **0.3.2**. The title bar and status bar show this version so testers can confirm they are running the new build rather than an older installed copy.

## Fixed in 1C.2

- Added an always-visible **Data Editor Commands** strip inside the work area. It contains Add/Edit/Delete Variable, Add/Insert/Delete Row(s), Copy/Paste, Sort Asc/Desc, and Dataset Info.
- Variable editing now works whether the variable is selected in the Variables panel or by selecting a Data Editor column.
- Variable labels are visibly shown beside variable names in the Variables panel and persist in `.stpro` projects.
- Declared missing codes are accepted even for Numeric/Date/Boolean variables and are visibly italicized in the grid. Missing counts appear in Properties/Dataset Info.
- Column-header clicks select the entire column and explicitly do not sort. Sorting only occurs through the Sort buttons.
- Copy/Paste has application-level Ctrl+C/Ctrl+V shortcuts and visible buttons. Paste can extend an unfiltered dataset by adding rows.
- Delete Variable now reports when no variable is selected and confirms the variable name before deletion.
- Import/Export buttons are renamed **Import Data / Export Data**. CSV, TSV and delimited TXT are supported in this build.
- Existing `.stpro` save/open support is preserved.

## Not yet included

Native `.xlsx`, `.xls`, `.dta`, `.sav`, and other proprietary statistical formats are not yet implemented. They require a dedicated file-format layer/dependency and should not be represented as supported until tested.
