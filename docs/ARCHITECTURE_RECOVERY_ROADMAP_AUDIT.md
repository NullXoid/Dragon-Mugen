# Architecture Recovery Roadmap Re-Evaluation Audit

This audit is the current decision point for Dragon MUGEN architecture recovery. Older audits remain historical context, but their "next work" conclusions are superseded by the current repository state recorded here.

## Current Checkpoint

Baseline and current file-size guard values:

| Item | Value |
| --- | ---: |
| Previous source baseline commit | `de22bec` |
| Starting `App.cpp` count | 16820 |
| Current `App.cpp` count | 8492 |
| Total removed | 8328 |
| Reduction | 49.5% |
| Remaining to 50% reduction | 82 |

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
- Movement/position controller runtime audit.
- Velocity-controller runtime body.
- Position/grounding controller runtime audit.
- `PosAdd` controller runtime body.
- `PosSet` / grounding controller runtime audit.
- PosSet-backed grounding verifier coverage.
- `PosSet` controller runtime body.
- Misc movement/state controller runtime audit.
- `SprPriority` controller runtime body.
- `PosFreeze` controller runtime body.
- `Turn` controller runtime body.

These completed areas should not be treated as future broad work. Future passes should build on them and avoid reopening their boundaries unless a focused regression or cleanup requires it.

## Remaining Runtime-Core Classification

Next code target:

- Audit `CtrlSet` / command-control behavior before any source movement.

Completed with limits:

- `VelSet` / `VelAdd` / `VelMul` velocity controller execution now lives in `StateControllerVelocityRuntime.h`.
- `PosAdd` execution now lives in `StateControllerPosAddRuntime.h`.
- `PosSet` execution now lives in `StateControllerPosSetRuntime.h`.
- `SprPriority` execution now lives in `StateControllerSprPriorityRuntime.h`.
- `PosFreeze` execution now lives in `StateControllerPosFreezeRuntime.h`.
- `Turn` execution now lives in `StateControllerTurnRuntime.h`.
- PosSet-backed grounding verifier coverage now proves KFM's authored `FF_a` path reaches states `1050`, `1051`, and `1052`, runs the state `1052` `PosSet` landing path, and returns to grounded controllable idle.

Needs a narrower audit or seam before movement:

- Remaining meter/stat controllers: `LifeAdd`, `HitAdd`, `AttackDist`, `AttackMulSet`, and `DefenceMulSet`.
- Control and state-shape controllers: `CtrlSet` and `StateTypeSet`.
- Bounds/contact-adjacent controllers: `ScreenBound`, `Width`, and `PlayerPush`.
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

The movement/position controller audit is complete: `docs/MOVEMENT_POSITION_CONTROLLER_RUNTIME_AUDIT.md` classifies velocity, position, control, state-shape, bounds, and adjacent hit/get-hit controllers. It recommends a velocity-only implementation cut and defers position/grounding, control, state-shape, camera/bounds, collision/contact, hit/get-hit, and round-flow-adjacent behavior.

The velocity-controller runtime body cut is complete: `StateControllerVelocityRuntime.h` owns only the `VelSet` / `VelAdd` / `VelMul` loop from `updateStateMovementControllers(...)`. `PosAdd`, `PosSet`, `PosFreeze`, `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, `PlayerPush`, `SprPriority`, `Turn`, hit/get-hit controllers, pause/superpause, hit/damage, lifecycle, target, and round-flow behavior remain in `App.cpp`.

The position/grounding controller audit is complete: `docs/POSITION_GROUNDING_CONTROLLER_RUNTIME_AUDIT.md` classifies `PosAdd`, `PosSet`, and `PosFreeze`. It recommends a `PosAdd`-only implementation cut and explicitly defers `PosSet` because it affects `onGround`, plus `PosFreeze` because it belongs with movement-freeze behavior.

The `PosAdd` controller runtime body cut is complete: `StateControllerPosAddRuntime.h` owns only the `PosAdd` loop from `updateStatePosAddControllers(...)`. `PosSet`, `PosFreeze`, `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, `PlayerPush`, `SprPriority`, `Turn`, hit/get-hit controllers, pause/superpause, hit/damage, lifecycle, target, and round-flow behavior remain in `App.cpp`.

The `PosSet` / grounding controller runtime audit is complete: `docs/POSSET_GROUNDING_CONTROLLER_RUNTIME_AUDIT.md` confirms the current `PosSet` body mutates only `fighter.x`, `fighter.y`, and `fighter.onGround`. At audit time, `kfm-air-state` did not explicitly prove a PosSet-triggered grounding path, so the audit recommended creating or extending a grounding verifier before moving `PosSet`.

The PosSet-backed grounding verifier hardening pass is complete: `kfm-air-state` now includes `kung_fu_knee_posset_grounding` and passes with `pass=12 partial=0 fail=0 blocked=0`. The row drives KFM's authored `FF_a` path through normal symbolic input, CMD command recognition, CNS `ChangeState`, states `1050 -> 1051 -> 1052`, the state `1052` `PosSet` landing controller, and grounded controllable idle recovery. The pass also fixed a verifier-exposed no-physics landing gap so custom `physics = N` air states are not implicitly ground-clamped or auto-finished to idle before authored landing controllers can run.

The `PosSet` controller runtime body cut is complete: `StateControllerPosSetRuntime.h` owns only the `PosSet` loop from `updateStateMovementControllers(...)`. It preserves the original `shouldRunStateRuntimeController(...)` gate, optional absolute `fighter.x` assignment, optional absolute `fighter.y` assignment, and `fighter.onGround = fighter.y >= 0.0f`. `App.cpp` dropped from `8517` to `8507` file-size-guard lines, and `StateControllerPosSetRuntime.h` is `24` lines. `PosAdd`, `PosFreeze`, `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, `PlayerPush`, `SprPriority`, `Turn`, velocity controllers, hit/get-hit controllers, pause/superpause, grounding physics, hit/damage, lifecycle, target, and round-flow behavior remain unchanged.

The misc movement/state controller runtime audit is complete: `docs/MISC_MOVEMENT_STATE_CONTROLLER_RUNTIME_AUDIT.md` classifies `PosFreeze`, `SprPriority`, `Turn`, `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, and `PlayerPush`. It marks `SprPriority` as `READY WITH LIMITS` because it only assigns `fighter.sprPriority` behind the standard runtime gate, and it defers `PosFreeze`, `Turn`, control/state-shape, and bounds/contact-adjacent controllers to narrower audits or seams.

The `SprPriority` controller runtime body cut is complete: `StateControllerSprPriorityRuntime.h` owns only the `SprPriority` loop from `updateStateMovementControllers(...)`. It preserves the original `shouldRunStateRuntimeController(...)` gate and `fighter.sprPriority = sprPriority.value` assignment. `App.cpp` dropped from `8507` to `8503` file-size-guard lines, and `StateControllerSprPriorityRuntime.h` is `18` lines. `PosFreeze`, `Turn`, `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, `PlayerPush`, velocity controllers, position controllers, hit/get-hit controllers, pause/superpause, grounding physics, hit/damage, lifecycle, target, and round-flow behavior remain unchanged.

The `PosFreeze` controller runtime body cut is complete: `StateControllerPosFreezeRuntime.h` owns only the `PosFreeze` loop from `updateStateMovementControllers(...)`. It preserves the original `shouldRunStateRuntimeController(...)` gate and the `fighter.posFreezeX = fighter.posFreezeX || posFreeze.freezeX` / `fighter.posFreezeY = fighter.posFreezeY || posFreeze.freezeY` assignment semantics. `App.cpp` dropped from `8503` to `8498` file-size-guard lines, and `StateControllerPosFreezeRuntime.h` is `21` lines. `Turn`, `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, `PlayerPush`, velocity controllers, position controllers, hit/get-hit controllers, pause/superpause, grounding physics, hit/damage, lifecycle, target, and round-flow behavior remain unchanged.

The `Turn` controller runtime body cut is complete: `StateControllerTurnRuntime.h` owns only the `Turn` loop from `updateStateMovementControllers(...)`. It preserves the original `shouldRunStateRuntimeController(...)` gate, `fighter.facing *= -1`, `fighter.jumpBaseAction = 0`, and `fighter.jumpPeakActionApplied = false` behavior. `App.cpp` dropped from `8498` to `8492` file-size-guard lines, and `StateControllerTurnRuntime.h` is `22` lines. `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, `PlayerPush`, velocity controllers, position controllers, hit/get-hit controllers, pause/superpause, input routing, command recognition, CPU behavior, hit/damage, lifecycle, target, and round-flow behavior remain unchanged.

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

Do not collapse the remaining runtime work into one pass. After the completed `StateControllerUtilityRuntime.h`, `StateControllerVariableRuntime.h`, `StateControllerPowerRuntime.h`, `StateControllerVelocityRuntime.h`, `StateControllerPosAddRuntime.h`, `StateControllerPosSetRuntime.h`, `StateControllerSprPriorityRuntime.h`, `StateControllerPosFreezeRuntime.h`, and `StateControllerTurnRuntime.h` cuts plus the meter/stat, movement/position, position/grounding, and misc movement/state audits, continue with focused audits or small implementation cuts in this order unless a new blocker requires a documented change:

1. `CtrlSet` / command-control audit.
2. `StateTypeSet` / state-shape audit.
3. `ScreenBound` / `Width` / `PlayerPush` bounds-contact audit.
4. Pause / superpause audit.
5. Helper / projectile / explod audit.
6. Target controller audit.
7. HitDef / damage audit.
8. Round flow audit.

## Current Roadmap Conclusion

The old `StateControllerUtilityRuntime.h` recommendation is complete as of `de22bec`.

The old `StateControllerVariableRuntime.h` recommendation is complete as the variable-only controller body move.

The old `Meter/stat controller audit` recommendation is complete in `docs/METER_STAT_CONTROLLER_RUNTIME_AUDIT.md`.

The old `PowerAdd`-only implementation recommendation is complete in `StateControllerPowerRuntime.h`.

The old `Movement / position controller audit` recommendation is complete in `docs/MOVEMENT_POSITION_CONTROLLER_RUNTIME_AUDIT.md`.

The old velocity-only implementation recommendation is complete in `StateControllerVelocityRuntime.h`.

The old `Position / grounding controller audit` recommendation is complete in `docs/POSITION_GROUNDING_CONTROLLER_RUNTIME_AUDIT.md`.

The old `PosAdd`-only implementation recommendation is complete in `StateControllerPosAddRuntime.h`.

The old `PosSet` / grounding seam audit recommendation is complete in `docs/POSSET_GROUNDING_CONTROLLER_RUNTIME_AUDIT.md`.

The old grounding-verifier-first recommendation is complete in `kfm-air-state` with `kung_fu_knee_posset_grounding`.

The old `PosSet`-only implementation recommendation is complete in `StateControllerPosSetRuntime.h`.

The old `PosFreeze` / movement-freeze audit recommendation is superseded by the broader misc movement/state controller audit in `docs/MISC_MOVEMENT_STATE_CONTROLLER_RUNTIME_AUDIT.md`.

The old `SprPriority`-only implementation recommendation is complete in `StateControllerSprPriorityRuntime.h`.

The old `PosFreeze` / movement-freeze recommendation is complete in `StateControllerPosFreezeRuntime.h`.

The old `Turn` / facing recommendation is complete in `StateControllerTurnRuntime.h`.

Next recommended pass:

```text
Audit CtrlSet / command-control behavior before any source movement.
```

Continue to defer:

- `CtrlSet`.
- `StateTypeSet`.
- `ScreenBound`.
- `Width`.
- `PlayerPush`.
- `HitVelSet` / `HitFallVel` / `HitFallSet` / `HitFallDamage`.
- `HitBy` / `NotHitBy` / `HitOverride`.
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

This checkpoint moved only `Turn` controller execution into `StateControllerTurnRuntime.h` after the completed PosFreeze body move. It did not change CMake, content, sidecar policy, boss intro behavior, branch topology, input routing, command recognition, CPU behavior, opponent-relative behavior, or `.dragon/*.json`.

Expected validation:

```powershell
python engine/tools/dev_check.py .
python tools/check_file_sizes.py
git diff --check
```

The full verifier gate should keep all advertised runtime scenarios at `partial=0 fail=0 blocked=0`. `tools/check_file_sizes.py` is expected to continue failing on known `App.cpp` hard debt and should report `App.cpp` at `8492`. Treat any new hard failure outside the known `App.cpp` debt as a blocker.

## Dirty File Handling

If unrelated dirty files appear during this docs-only audit, leave them alone. In particular:

```text
engine/src/TrainingOptionsOverlay.cpp
```

is unrelated dirty work if present. Do not modify it, stage it, or resolve it inside this audit.
