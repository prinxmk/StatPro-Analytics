## v0.6.2 regression fix

Fixed Simple Linear Regression variable-to-column mapping so numeric variables that are not the first columns in the dataset are passed to the analysis engine using their actual dataset column indices. This prevents false "fewer than 3 complete observations" errors when the selected numeric variables contain valid data.

# StatPro Analytics — Phase 1F / v0.6.2

## Regression

Phase 1F introduces the first regression procedure under **Analysis → Regression**:

- Simple linear regression
- OLS intercept and slope estimates
- Standard errors, t statistics and two-sided p-values
- 95% confidence intervals for coefficients
- R² and adjusted R²
- Regression ANOVA F statistic and model p-value
- RMSE
- Durbin–Watson residual diagnostic
- Model equation and plain-language interpretation

### Observation accounting

The regression procedure explicitly reports total observations, complete observations and separate X/Y counts for blank, declared-missing and non-numeric/invalid values. Only complete numeric pairs enter the fitted model.

### Scope

This is the first regression layer. Multiple linear regression, categorical predictors, residual plots, influence diagnostics, prediction intervals and additional regression models remain planned.
