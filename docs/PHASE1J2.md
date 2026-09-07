# Phase 1J.2 — v1.3.2 Logistic Regression Fix

Fixed Binary Logistic Regression returning zero complete observations for valid binary text/categorical outcomes whose labels were not in the previous hard-coded list.

## Changes
- Accept any outcome with exactly two non-missing levels.
- Numeric outcomes may use any two distinct numeric values; the lower value is coded 0 and the higher value 1.
- String/Boolean outcomes may use any two labels.
- Common labels such as yes/no, true/false, success/failure and positive/negative are mapped semantically.
- Other two-level text outcomes are deterministically sorted for 0/1 coding.
- The Results / Output note reports the actual 0/1 outcome coding.
- A clear message is shown when the selected outcome has more than two levels.
- Version metadata is synchronized to 1.3.2.
