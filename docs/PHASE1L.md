# Phase 1L — v1.5.0

## Generalized Linear Models

Added a first GLM block under **Analysis → Regression → Generalized Linear Models**:

- Poisson Regression with log link for non-negative integer count outcomes.
- Negative Binomial Regression (NB2) with log link and estimated dispersion parameter.
- Numeric predictors with complete-case accounting.
- Coefficients, standard errors, z statistics, p-values, incidence-rate ratios (IRR), and 95% IRR confidence intervals.
- Log likelihood, AIC, BIC, deviance, Pearson chi-square, Pearson/df, and a likelihood-based pseudo-R² diagnostic.
- Singular-design and convergence safeguards.

Poisson reports Pearson/df as an overdispersion diagnostic. Negative Binomial models variance as μ + αμ². This is an initial GLM block; offsets, exposure terms, categorical predictors, and additional GLM families will be added in later phases.
