# StatPro Analytics — Phase 1D.2 / v0.4.2

## Results and Output Presentation

This phase introduces a reusable results-table presentation layer. Analysis results are displayed as real Qt tables with aligned numeric columns, consistent sizing, selectable cells, scrolling, titles, summaries and notes.

### Formatting
- Results tables have no row shading by default.
- View → Results Table Formatting… lets the user choose no/alternating row shading, grid visibility, font size, decimal places, header color, row color and alternate-row color.
- Formatting preferences persist with QSettings.

### Observation accounting
Descriptive statistics, frequencies and grouped summaries now retain explicit accounting for observations that are blank, declared missing, or non-numeric/invalid rather than silently dropping them from the displayed result.

### Analysis output
- Descriptive Statistics reports observations, valid, blank, declared missing and non-numeric counts.
- Frequencies displays special categories and reports percentages of all observations plus valid percentages.
- Summary by Group reports observation accounting separately within every group.
- Dataset Information is also displayed as a structured table.
