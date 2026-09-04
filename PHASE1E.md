# StatPro Analytics — Phase 1E / v0.5.0

## Inferential statistics

Phase 1E adds a first production set of inferential procedures to the Analysis → Tests menu:

- Pearson correlation
- One-sample t test
- Independent-samples t test (Welch or equal-variance assumption)
- Paired-samples t test
- Chi-square test of independence
- One-way ANOVA

## Output

Results use the existing structured Results / Output table framework. Numeric cells remain aligned and respect the user's Results Table Formatting preferences. No row shading is applied by default.

Each procedure reports the principal test statistic, degrees of freedom where applicable, p-value, confidence intervals where applicable, and an effect-size measure where appropriate:

- Pearson r with Fisher 95% CI
- Cohen's d for one-sample and independent t tests
- Cohen's dz for paired t tests
- Cramér's V for chi-square
- Eta-squared for one-way ANOVA

Observation accounting is explicit. Blank, declared-missing and non-numeric/invalid values are not silently discarded from the reported observation summary. Inferential calculations use only observations valid for the requested procedure.

## Statistical implementation

The analysis engine includes self-contained numerical routines for the normal, Student-t, chi-square and F distributions using incomplete beta/gamma calculations. No external statistical runtime is required.

## Scope note

This is the first inferential-statistics layer. Regression, non-parametric tests, post-hoc ANOVA procedures, exact tests, power/sample-size tools, and advanced model diagnostics remain planned for later phases.
