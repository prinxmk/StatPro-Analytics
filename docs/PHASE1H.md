# StatPro Analytics — Phase 1H / v0.8.0

## Regression Diagnostics

Phase 1H adds **Analysis → Diagnostics → Regression Diagnostics…**.

The procedure fits the same complete-case OLS model used by Multiple Linear Regression and provides observation-level diagnostics:

- Actual outcome
- Predicted outcome
- Raw residual
- Standardized residual
- Externally studentized residual
- Leverage (hat value)
- Cook's distance
- Automatic flags for high leverage, influential observations, and large residuals

The output also reports:

- Complete N and predictor count
- RMSE, R² and adjusted R²
- Durbin–Watson statistic
- Jarque–Bera residual normality statistic and p-value
- Maximum leverage and Cook's distance
- Counts of flagged observations
- Explicit exclusion accounting for blank, declared-missing and invalid/non-numeric values

### Diagnostic rules

- High leverage: leverage > 2p/n
- Influential: Cook's distance > 4/n
- Large residual: absolute externally studentized residual > 2
- Residual normality: Jarque–Bera test; p < 0.05 is reported as evidence against normality

### Scope

This phase adds diagnostic output but does not yet add residual/influence plots, categorical factor coding, interaction terms, prediction intervals, or generalized linear models. Those remain planned.
