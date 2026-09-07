# Phase 1J.1 — v1.3.1 Build Fix

Fixed a Windows/MSVC compilation error in `RegressionPredictionResult`: the implementation and UI referenced `movingWindow`, but the result struct did not declare that member. The result struct now declares `int movingWindow{3}`. Version metadata is synchronized to 1.3.1.
