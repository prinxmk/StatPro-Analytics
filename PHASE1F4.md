# Phase 1F4 — Independent-Samples t Test Group Selection

Version: 0.6.4

## Fix
Independent-Samples t Test no longer requires the dataset to contain a grouping variable with exactly two levels. A grouping variable may contain two or more valid levels; the user explicitly selects Group 1 and Group 2 for the comparison.

The selected outcome and grouping variables are resolved against their actual dataset columns. Group levels are normalized for numeric grouping variables and invalid/missing group labels are excluded. Each selected group must have at least two valid numeric outcome observations.
