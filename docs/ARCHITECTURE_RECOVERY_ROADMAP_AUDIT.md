# Architecture Recovery Roadmap Re-Evaluation Audit

This audit is the current decision point for Dragon MUGEN architecture recovery. Older audits remain historical context, but their "next work" conclusions are superseded by the current repository state recorded here.

## Current Checkpoint

Baseline and current file-size guard values:

| Item | Value |
| --- | ---: |
| Previous source baseline commit | `de22bec` |
| Starting `App.cpp` count | 16820 |
| Current `App.cpp` count | 8561 |
| Total removed | 8259 |
| Reduction | 49.1% |

`App.cpp` is no longer in the safe UI extraction stage. The project is now in the runtime-core extraction stage. The remaining work is mutation-heavy and must stay split into small, auditable cuts.

The current verifier gate is also stricter than earlier roadmap checkpoints. Full `python engine/tools/dev_check.py .` now configures/builds, runs the console roster check, then runs `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline`. Runtime verifier summaries should report `partial=0`, `fail=0`, and `blocked=0` for advertised claims unless a future optional check is explicitly documented.

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
- Controller utility/audio/assert/display runtime body.
- Variable-controller runtime body.
- Meter/stat controller runtime audit.
- `PowerAdd` controller runtime body.

These completed areas should not be treated as future broad work. Future passes should build on them and avoid reopening their boundaries unless a focused regression or cleanup requires it.

## Remaining Runtime-Core Classification

Next audit target:

- Movement/position controller audit.

Ready or near-ready after separate inspection:

- Movement/position controller body, only if kept away from physics update and round-flow logic.

Needs a narrower audit before movement:

- Remaining meter/stat controllers: `LifeAdd`, `HitAdd`, `AttackDist`, `AttackMulSet`, and `DefenceMulSet`.
- Pause and superpause.
- Helper, projectile, and explod lifecycle.
- Target controllers.
- State transition controllers.

Deferred:

- HitDef, damage, guard, and get-hit behavior.
- Round and match flow.
- `FighterState` as a whole.
- `AppState` as a whole.
- Broad CNS runtime movement.

## Current Runtime-Core Boundary

The hardest remaining systems are live mutation paths:

- CNS controller execution.
- Hit, damage, guard, and get-hit behavior.
- Target mutation.
- Helper, projectile, explod, and effect lifecycle.
- Pause and superpause timing.
- Remaining meter/stat, movement, and position controllers.
- Round and match flow.
- `FighterState` and `AppState` ownership.

Architecture recovery remains the active feature. Broad gameplay work should stay limited while these runtime-core boundaries are extracted and documented one responsibility at a time.

The first bounded runtime-core code cut is now complete: `StateControllerUtilityRuntime.h` owns the controller utility/audio/assert/display runtime body without moving state transitions, HitDef/damage, target mutation, helper/projectile/explod lifecycle, pause/superpause timing, variable/meter mutation, movement/position mutation, or round flow.

The variable-controller body cut is also complete: `StateControllerVariableRuntime.h` owns `updateStateVariableControllers(...)` for `VarSet`, `VarAdd`, `VarRandom`, and `VarRangeSet` execution only. `setFighterVariableValue(...)` remains in `App.cpp` because `ParentVarAdd` still uses it, and `ParentVarAdd` remains deferred with helper/parent lifecycle work.

The meter/stat controller audit is complete: `docs/METER_STAT_CONTROLLER_RUNTIME_AUDIT.md` classifies `PowerAdd`, `LifeAdd`, `HitAdd`, `AttackDist`, `AttackMulSet`, and `DefenceMulSet`. It recommends a `PowerAdd`-only implementation cut and defers the remaining stat/damage-adjacent controllers to round-flow, hit/contact, or hit/damage seams.

The `PowerAdd` controller runtime body cut is complete: `StateControllerPowerRuntime.h` owns only the `PowerAdd` loop from `updateStateMeterControllers(...)`. `LifeAdd`, `HitAdd`, `AttackDist`, `AttackMulSet`, `DefenceMulSet`, `TargetPowerAdd`, and `TargetLifeAdd` remain in `App.cpp`.

## Completed Utility Runtime Pass

Completed code pass:

```text
StateControllerUtilityRuntime.h
```

Completed scope:

Moved only utility/audio/assert/display controller execution bodies that do not own state transitions, hit/damage, target mutation, helper/projectile/explod lifecycle, pause/superpause timing, or round flow.

Moved examples:

- PlaySnd / StopSnd controller dispatch and fire-key helpers.
- AssertSpecial flag helpers.
- Display, clipboard, victory quote, angle, offset, remap palette, trans, and afterimage visual controllers.
- `changeAnimTriggerActive`.
- Force-feedback helper path.
- Afterimage effect ticking.
- Shared runtime fired-history gate as a behavior-preserving placement move.

Still not moved:

- `ChangeState` / `SelfState`.
- `HitDef`.
- `Target*`.
- `Helper`.
- `Projectile`.
- `Explod` lifecycle.
- `Pause` / `SuperPause`.
- Remaining meter/stat controllers.
- Movement/position controllers.
- Round flow.
- `FighterState` / `AppState` extraction.

This completed pass stayed intentionally narrower than "move CNS runtime." It was a controller utility/audio/assert/display execution cut only.

## Follow-Up Sequence

Do not collapse the remaining runtime work into one pass. After the completed `StateControllerUtilityRuntime.h`, `StateControllerVariableRuntime.h`, and `StateControllerPowerRuntime.h` cuts and the meter/stat audit, continue with focused audits or small implementation cuts in this order unless a new blocker requires a documented change:

1. Movement / position controller audit.
2. Pause / superpause audit.
3. Helper / projectile / explod audit.
4. Target controller audit.
5. HitDef / damage audit.
6. Round flow audit.

## Current Roadmap Conclusion

The old `StateControllerUtilityRuntime.h` recommendation is complete as of `de22bec`.

The old `StateControllerVariableRuntime.h` recommendation is complete as the variable-only controller body move.

The old `Meter/stat controller audit` recommendation is complete in `docs/METER_STAT_CONTROLLER_RUNTIME_AUDIT.md`.

The old `PowerAdd`-only implementation recommendation is complete in `StateControllerPowerRuntime.h`.

Next recommended implementation:

```text
Movement / position controller audit
```

Audit before moving:

- PosAdd / PosSet style position controllers.
- VelSet / VelAdd / VelMul style velocity controllers.
- StateTypeSet / ScreenBound / Width / PlayerPush / SprPriority / PosFreeze / Turn.

Do not include:

- `LifeAdd`.
- `HitAdd`.
- `AttackDist`.
- `AttackMulSet`.
- `DefenceMulSet`.
- `TargetPowerAdd` / `TargetLifeAdd`.
- `HitDef`.
- Damage / guard.
- Target controllers.
- Helper / Projectile / Explod lifecycle.
- `ChangeState` / `SelfState`.
- `Pause` / `SuperPause`.
- KO, time-over, double-KO, or round-flow mutation.
- Round flow.
- `FighterState` / `AppState` extraction.
- Boss intro / preemptive attack behavior.
- Dragon Mode, store, crafting, or other broad gameplay systems.

## Validation For Current Checkpoint

This checkpoint should not change CMake, content, sidecar policy, boss intro behavior, branch topology, or `.dragon/*.json`. The source change is limited to the internal `StateControllerPowerRuntime.h` extraction and its `App.cpp` include/call-site update.

Expected validation:

```powershell
python engine/tools/dev_check.py .
python tools/check_file_sizes.py
git diff --check
```

`tools/check_file_sizes.py` is expected to continue failing on known `App.cpp` hard debt and should report `App.cpp` at `8561` until the next source extraction. Treat any new failure outside the known `App.cpp` debt as a blocker.

## Dirty File Handling

If unrelated dirty files appear during this docs-only audit, leave them alone. In particular:

```text
engine/src/TrainingOptionsOverlay.cpp
```

is unrelated dirty work if present. Do not modify it, stage it, or resolve it inside this audit.
