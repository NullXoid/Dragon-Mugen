# Bugs

Record runtime failures here before fixing them. This file is for observed behavior, not speculative compatibility gaps.

## Bug Report Template

```text
## Bug: short title

Status:
Severity:
Area:
Character:
Mode:
Stage:
Input device:
Build/commit:

Reproduction:
1.
2.
3.

Expected:
Actual:
Notes:
Possible suspect files:
Blocking:
```

## Current Pass

No Dragon MUGEN runtime bugs have been recorded yet in this pass.

Computer Use could verify event-style keys such as `F1` and `F2`, but it could not reliably exercise SDL polled gameplay controls such as held movement or `A/S/D/Z/X/C` attacks. That is recorded as a verification limitation in `docs/LIVE_VERIFICATION_MATRIX.md`, not as a Dragon MUGEN runtime bug.
