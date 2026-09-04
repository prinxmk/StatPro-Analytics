# Phase 1F.3 — Independent-Samples t Test Fix

Version 0.6.3 fixes the Independent-Samples t Test group-selection and group-detection logic.

- Resolves grouping and outcome variables using their actual dataset column indexes.
- Detects valid grouping levels from the selected grouping column.
- Normalizes numeric group values so equivalent values are not split into separate groups.
- The grouping-variable selector now offers only variables with exactly two valid groups.
- The test reports the valid outcome count separately for each group when a group has insufficient data.
- Preserves Welch and equal-variance options, Cohen's d, confidence interval, and observation accounting.
