# Phase 1I — v0.9.0

## Regression with Categorical Predictors

Adds **Analysis → Regression → Multiple Linear Regression with Categorical Predictors…**.

- Numeric predictors are treated as continuous.
- Text and Boolean predictors are treated as categorical.
- Categorical predictors use reference-cell (dummy) coding.
- The first sorted valid level is the reference category.
- Each non-reference level receives a coefficient interpreted as the adjusted difference from the reference.
- Complete-case observation accounting reports blank, declared-missing, and non-numeric/invalid exclusions.
- Output includes estimates, standard errors, standardized beta, t, p-value, 95% confidence intervals, VIF, R², adjusted R², RMSE, model F-test, model p-value, and Durbin–Watson.
- Existing numeric-only regression procedures remain unchanged.
