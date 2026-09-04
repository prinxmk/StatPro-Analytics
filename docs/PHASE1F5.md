# StatPro Analytics — Phase 1F.5 / v0.6.5

## Independent-Samples t Test and version-label correction

- Independent-Samples t Test now allows a grouping variable with two or more valid levels.
- The user selects Group 1 and Group 2 explicitly rather than requiring the grouping variable to contain exactly two levels.
- Numeric and string/categorical grouping values are normalized consistently.
- Blank, declared-missing, and invalid grouping values are excluded.
- Outcome values are accounted for separately within each selected group.
- Each selected group must have at least two valid numeric outcome observations.
- Welch unequal-variance and equal-variance options are retained.
- Corrected the application title so the displayed version matches the built application version.
