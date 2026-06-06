# Architecture Recovery Roadmap Re-Evaluation Audit

This audit is the current decision point for Dragon MUGEN architecture recovery. Older audits remain historical context, but their "next work" conclusions are superseded by the current repository state recorded here.

## Current Checkpoint

Baseline and current file-size guard values:

| Item | Value |
| --- | ---: |
| Starting `App.cpp` count | 16820 |
| Current `App.cpp` count | 9184 |
| Total removed | 7636 |
| Reduction | 45.4% |

`App.cpp` is no longer in the safe UI extraction stage. The project is now in the runtime-core extraction stage. The remaining work is mutation-heavy and must stay split into small, auditable cuts.

## Completed Recovery Areas

The earlier roadmap's safe chunks have already been substantially completed:

- Render overlays and UI seams.
- World rendering body.
- Audio runtime and mixer body.
- Runtime loading and resource load/destroy body.
- Command parsing.
- State-controller parsing.
- Command recognition.
- Command state eligibility.
- Runtime expression and trigger evaluation.
- Fight/session setup and reset.

These completed areas should not be treated as future broad work. Future passes should build on them and avoid reopening their boundaries unless a focused regression or cleanup requires it.

## Current Runtime-Core Boundary

The hardest remaining systems are live mutation paths:

- CNS controller execution.
- Hit, damage, guard, and get-hit behavior.
- Target mutation.
- Helper, projectile, explod, and effect lifecycle.
- Pause and superpause timing.
- Variable, meter, movement, and position controllers.
- Round and match flow.
- `FighterState` and `AppState` ownership.

Architecture recovery remains the active feature. Broad gameplay work should stay limited while these runtime-core boundaries are extracted and documented one responsibility at a time.

## Recommended Next Implementation Pass

Next code pass:

```text
StateControllerUtilityRuntime.h
```

Scope:

Move only utility/audio/assert/display controller execution bodies that do not own state transitions, hit/damage, target mutation, helper/projectile/explod lifecycle, pause/superpause timing, or round flow.

Allowed examples:

- Audio/display/clipboard/assert-style utility controllers.
- Force-feedback helper path if present.
- Victory quote/display-only utility when it does not affect round flow.
- Palette/remap/trans/afterimage helpers only if inspection confirms they are display/runtime-effect utility and not lifecycle-heavy.

Do not move:

- `ChangeState` / `SelfState`.
- `HitDef`.
- `Target*`.
- `Helper`.
- `Projectile`.
- `Explod` lifecycle.
- `Pause` / `SuperPause`.
- Variable/meter controllers.
- Movement/position controllers.
- Round flow.
- `FighterState` / `AppState` extraction.

This is intentionally narrower than "move CNS runtime." It is a controller utility/audio/assert/display execution cut only.

## Follow-Up Sequence

Do not collapse the remaining runtime work into one pass. After `StateControllerUtilityRuntime.h`, continue with focused audits or small implementation cuts in this order unless a new blocker requires a documented change:

1. Variable / meter controller audit.
2. Movement / position controller audit.
3. Pause / superpause audit.
4. Helper / projectile / explod audit.
5. Target controller audit.
6. HitDef / damage audit.
7. Round flow audit.

## Validation For This Audit

This audit is documentation-only. It should not change source, CMake, content, runtime behavior, controller behavior, sidecar policy, boss intro behavior, or `.dragon/*.json`.

Expected validation:

```powershell
python engine/tools/dev_check.py .
python tools/check_file_sizes.py
git diff --check
```

`tools/check_file_sizes.py` is expected to continue failing on known `App.cpp` hard debt until the next source extraction brings it below the guard threshold. Treat any new failure outside the known `App.cpp` debt as a blocker.

## Dirty File Handling

If unrelated dirty files appear during this docs-only audit, leave them alone. In particular:

```text
engine/src/TrainingOptionsOverlay.cpp
```

is unrelated dirty work if present. Do not modify it, stage it, or resolve it inside this audit.
