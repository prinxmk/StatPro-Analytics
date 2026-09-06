# StatPro Analytics — Phase 1G / v0.7.0

## Multiple Linear Regression

Phase 1G adds **Analysis → Regression → Multiple Linear Regression…**.

The procedure supports one numeric outcome and one or more numeric predictors using ordinary least squares with complete-case estimation. Output includes:

- Intercept and predictor coefficients
- Standard errors, t statistics and two-sided p-values
- 95% confidence intervals
- Standardized beta coefficients
- Variance inflation factor (VIF) for each predictor
- R² and adjusted R²
- Regression ANOVA F statistic and model p-value
- RMSE
- Durbin–Watson statistic
- Model equation
- Explicit complete-case / missing-value accounting

The engine detects singular design matrices (for example, perfect multicollinearity) and reports a clear error rather than producing invalid coefficients.

### Scope

This phase supports numeric predictors. Categorical predictors/factor coding, interactions, residual plots, influence diagnostics, prediction intervals and generalized linear models remain planned.
