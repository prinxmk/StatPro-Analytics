# Phase 1F.2 — Inferential test variable-index fix (v0.6.2)

## Fix
Corrected the variable-selection index mapping for:
- Pearson Correlation
- One-Sample t Test
- Paired-Samples t Test

The dialogs present only numeric variables, but the underlying dataset contains all variables. The selected variable name is now resolved against the dataset's full column list before calling the analysis engine.

This is the same class of defect previously corrected for Simple Linear Regression in v0.6.1.

## Audit
Summary by Group, Independent-Samples t Test, One-Way ANOVA, and Chi-Square Test already resolve selections against the full dataset column list.
