# Phase 1K — v1.4.0 Nonparametric Statistics

Adds a nonparametric testing layer under **Analysis → Tests → Nonparametric Tests**.

## Procedures
- Mann–Whitney U Test for two independent groups.
- Wilcoxon Signed-Rank Test for paired measurements.
- Kruskal–Wallis Test for two or more independent groups.
- Spearman Rank Correlation for monotonic association.

## Statistical details
- Average ranks are used for ties.
- Mann–Whitney and Wilcoxon use tie-corrected asymptotic normal approximations with continuity correction.
- Kruskal–Wallis uses a tie-corrected chi-square approximation.
- Spearman uses Pearson correlation on ranked values and a Student-t approximation for the p-value.
- Observation accounting remains visible for blank, declared-missing and invalid observations.
- Effect sizes are reported where applicable.

This block does not remove or alter the existing parametric tests, regression, time-series or econometrics procedures.
