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

Original verifier coverage was useful but not specific enough for a `CtrlSet` body move:

- `kfm-baseline` proves controllable idle readiness, hit evidence, spark/sound/hitpause proof, and clean scenario completion.
- `evilken-smoke` proves controllable idle, active fight phase, timer stability, hit/guard evidence, and clean completion.
- `kfm-air-state` proves diagonal jump and authored PosSet-backed landing through `kung_fu_knee_posset_grounding`.
- `cpu-baseline` proves CPU can approach/attack under normal control conditions.

Those checks proved the control flag was healthy in broad runtime scenarios, but they did not prove an authored `CtrlSet` controller disables control, blocks `requiresCtrl` command entry, then restores control through the normal CNS path.

## Completed Command-Control Proof

The command-control proof is now satisfied by `kfm-baseline` row `taunt_ctrlset_control_restore`.

The row exercises KFM's authored taunt path through normal symbolic input:

- `start` command enters state `195`.
- State `195` starts from `ctrl = 0`.
- A normal command attempt is blocked while `ctrl` is false.
- The authored state `195` `CtrlSet` at `Time = 40` restores `ctrl = 1`.
- KFM returns to grounded controllable idle.
- A normal command works again after restoration.

Full `dev_check.py .` passed after this proof with `kfm-baseline` at `pass=13 partial=0 fail=0 blocked=0`; the other verifier summaries stayed at `partial=0 fail=0 blocked=0`.

## Boundaries

This audit moved no source code and changed no runtime behavior.

Do not move or modify during the next body-move pass:

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
Move `CtrlSet`-only controller execution into an App.cpp-internal header.

Recommended header:
StateControllerCtrlRuntime.h

Move only:
- `CtrlSet` execution from `updateStateCtrlControllers(...)`

Do not move:
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
