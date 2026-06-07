# CtrlSet / Command-Control Controller Runtime Audit

## Summary

This docs-only audit classifies `CtrlSet` runtime execution before any source movement.

Current baseline:

| Item | Value |
| --- | ---: |
| Branch | `main` |
| Current `App.cpp` count | 8510 |
| Starting `App.cpp` count | 16820 |
| Total removed | 8310 |
| Reduction | 49.4% |
| Remaining to 50% reduction | 100 |

`CtrlSet` is still implemented in `updateStateCtrlControllers(...)` in `engine/src/App.cpp`. It is not just a display or movement-value assignment: it writes `fighter.ctrl`, which gates command-state eligibility, fallback walk/jump behavior, CPU input choice, and controllable-idle verification.

## Live Execution Shape

Current `CtrlSet` loading creates `StateCtrlController` entries only when a `value` is present and either a full parsed trigger or simple `time` / `animelem` trigger is available. Runtime execution:

- Iterates active runtime state definitions through `forEachRuntimeControllerStateDefinition(...)`.
- Evaluates either `stateControllerTriggerActive(...)` or `simpleControllerTriggerActive(...)`.
- Uses `firedStateCtrlControllerIds` through `stateCtrlAlreadyFired(...)` / `markStateCtrlFired(...)`.
- Marks the controller fired before assignment.
- Mutates only `fighter.ctrl = ctrl.value`.

The body is mechanically small, but it has command-control coupling because `fighter.ctrl` is consumed by command-state eligibility and by normal player/CPU control flow.

## Risk Classification

| Area | Current coupling | Risk | Recommendation |
| --- | --- | --- | --- |
| Trigger and persistence | Full trigger path or simple trigger path plus custom `firedStateCtrlControllerIds` history | Medium | Preserve exact order before any move |
| `fighter.ctrl` mutation | Enables/disables controllability | Medium-high | Needs direct verifier proof |
| Command recognition | `canEnterCommandState(...)` rejects `requiresCtrl` entries when `fighter.ctrl` is false | Medium-high | Verify command lockout and restoration |
| Player movement | Walk/jump fallback requires `fighter.ctrl`; walk state exits when control is lost | Medium | Verify normal movement after restoration |
| CPU behavior | `cpuCanChooseInput(...)` requires `cpu.ctrl` | Medium | Existing `cpu-baseline` gives broad coverage, not authored `CtrlSet` proof |
| State transitions | `enterState(...)` initializes from state `ctrl`, and `ChangeState` can override `ctrl` | Medium-high | Keep separate from `ChangeState` / `SelfState` |
| Helpers | Helpers call `updateStateCtrlControllers(...)` too | Medium | Do not move with helper lifecycle work |
| Round flow | Round-finish paths can force `ctrl=false` outside `CtrlSet` | Medium | Keep round-flow out of scope |

## Verifier Coverage

Current verifier coverage is useful but not specific enough for a `CtrlSet` body move:

- `kfm-baseline` proves controllable idle readiness, hit evidence, spark/sound/hitpause proof, and clean scenario completion.
- `evilken-smoke` proves controllable idle, active fight phase, timer stability, hit/guard evidence, and clean completion.
- `kfm-air-state` proves diagonal jump and authored PosSet-backed landing through `kung_fu_knee_posset_grounding`.
- `cpu-baseline` proves CPU can approach/attack under normal control conditions.

Those checks prove the control flag is healthy in broad runtime scenarios, but they do not prove an authored `CtrlSet` controller disables control, blocks `requiresCtrl` command entry, then restores control through the normal CNS path. Moving `CtrlSet` without that proof would preserve a small body mechanically but leave a command-control behavior claim under-covered.

## Required Future Proof

Before moving `CtrlSet`, add or extend verifier coverage to exercise an authored control-toggle path through normal runtime mechanisms. The proof should require:

- A real authored `CtrlSet` execution is observed.
- `fighter.ctrl` becomes false because of that controller.
- A `requiresCtrl` command path is blocked while control is false.
- A later authored state or controller restores controllable idle.
- Normal movement or command entry works again after restoration.
- The scenario summary remains `partial=0 fail=0 blocked=0`.

If the existing roster cannot provide a deterministic authored `CtrlSet` path, the verifier pass should stop and report the blocker rather than weakening the claim.

## Boundaries

This audit moved no source code and changed no runtime behavior.

Do not move or modify during the next proof pass:

- `CtrlSet` execution body.
- `StateTypeSet`.
- `ScreenBound`, `Width`, or `PlayerPush`.
- Hit/get-hit controllers.
- HitDef, damage, or guard behavior.
- Round flow.
- Helper, projectile, or explod lifecycle.
- Target controllers.
- `ChangeState` / `SelfState`.
- Pause / superpause.
- CPU behavior or input routing, except for verifier observation.
- Content, CMake, sidecar policy, DLC branches, or `.dragon/*.json`.

## Conclusion

Next code pass:
Create or extend command-control verifier coverage before moving `CtrlSet`.

Recommended verifier target:
An authored `CtrlSet` path that proves control disables command eligibility, then restores controllable idle through normal CNS/runtime execution.

Do not move:
- `CtrlSet`
- `StateTypeSet`
- `ScreenBound`
- `Width`
- `PlayerPush`
- hit/get-hit controllers
- HitDef / damage / guard
- KO, time-over, double-KO, or round flow
- helper / projectile / explod lifecycle
- target controllers
- ChangeState / SelfState
- pause / superpause

