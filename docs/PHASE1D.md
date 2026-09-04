# Phase 1D — Analysis Engine & Descriptive Statistics

Version 0.4.0 establishes the first reusable statistical-analysis layer.

## Included
- `AnalysisEngine` separated from the Qt UI.
- Descriptive statistics for numeric variables.
- N and missing count.
- Mean, sample standard deviation and variance.
- Minimum, Q1, median, Q3 and maximum.
- Skewness and excess kurtosis.
- Linear-interpolated quartiles.
- Analysis output displayed in the Results / Output tab.
- Describe → Descriptive Statistics is available from the compact Analysis menu.
- When numeric columns are selected in the Data Editor, only those selected numeric variables are analyzed; otherwise all numeric variables are used.

## Architecture
DataSet → AnalysisEngine → Statistical Result → Results / Output

Future tests, regression, graphs and export features will reuse this analysis layer.
