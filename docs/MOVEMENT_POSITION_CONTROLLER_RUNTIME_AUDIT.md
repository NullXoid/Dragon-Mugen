# Movement / Position Controller Runtime Audit

This audit classified movement, position, control, state-shape, bounds, and adjacent controller execution after `ff73242 Move PowerAdd controller runtime body out of App.cpp`.

## Current Baseline

| Item | Value |
| --- | ---: |
| Branch | `main` |
| Required baseline commit | `ff73242` |
| `App.cpp` count at audit | 8561 |
| `App.cpp` count after velocity cut | 8533 |
| Starting `App.cpp` count | 16820 |
| Remaining to 50% reduction after velocity cut | 123 |

The full verifier gate is currently green at this checkpoint. `updateStateMovementControllers(...)` still owns the velocity, state-shape, bounds, and adjacent hit/get-hit controller loops. `updateStatePosAddControllers(...)` and `updateStateCtrlControllers(...)` are nearby controller bodies with separate simple-trigger fired-history handling.

## Controller Classification

| Controller | Mutation | Coupling | Readiness | Recommendation |
| --- | --- | --- | --- | --- |
| `VelSet` / `VelAdd` / `VelMul` | mutates `fighter.vx` / `fighter.vy` | movement-state mutation only in the current body | `COMPLETE WITH LIMITS` | moved to `StateControllerVelocityRuntime.h` |
| `PosAdd` | mutates `fighter.x` / `fighter.y` | separate simple-trigger fired history | `NEEDS POSITION SEAM` | defer to position-only cut |
| `PosSet` | mutates `fighter.x` / `fighter.y` and `fighter.onGround` | position and grounding semantics | `NEEDS POSITION/GROUNDING SEAM` | defer |
| `PosFreeze` | mutates position-freeze flags | live fighter-state movement behavior | `NEEDS SEPARATE AUDIT` | defer |
| `SprPriority` | mutates sprite priority | presentation ordering inside live fighter state | `NEEDS SEPARATE AUDIT` | defer |
| `Turn` | mutates facing and jump-action state | facing and movement-state coupling | `NEEDS MOVEMENT SEAM` | defer |
| `CtrlSet` | mutates `fighter.ctrl` | control and command-runtime coupled, with separate fired history | `DEFER` | defer |
| `StateTypeSet` | mutates `stateType`, `moveType`, `physics`, and `onGround` | state-shape, physics, and grounding behavior | `DEFER` | defer |
| `ScreenBound` | mutates screen-bound and camera flags | camera and bounds behavior | `NEEDS CAMERA/CONTACT SEAM` | defer |
| `Width` | mutates edge and player width data | collision, push, and contact-adjacent behavior | `NEEDS HIT/CONTACT SEAM` | defer |
| `PlayerPush` | mutates player-push behavior | collision and contact-adjacent behavior | `NEEDS HIT/CONTACT SEAM` | defer |
| `HitVelSet` / `HitFallVel` / `HitFallSet` / `HitFallDamage` | mutates get-hit velocity, fall, and damage state | hit, damage, and get-hit behavior | `NEEDS HIT/DAMAGE SEAM` | defer |
| `HitBy` / `NotHitBy` / `HitOverride` | mutates hit protection and override state | hit and get-hit eligibility | `NEEDS HIT/GET-HIT SEAM` | defer |

## Audit Findings

Velocity-only controllers were the smallest completed cut. They share one loop over velocity controller definitions, use the shared runtime trigger gate, and write only `fighter.vx` and `fighter.vy`. `StateControllerVelocityRuntime.h` preserves behavior by calling the helper exactly where the `VelSet` / `VelAdd` / `VelMul` loop previously lived inside `updateStateMovementControllers(...)`.

Do not move position, control, state-shape, bounds, hit/get-hit, pause, or lifecycle controllers with velocity. `PosAdd` and `CtrlSet` have separate fired-history helpers. `StateTypeSet` and `PosSet` change grounding or physics state. `ScreenBound`, `Width`, and `PlayerPush` affect camera, collision, or contact-adjacent behavior. `HitVelSet`, `HitFall*`, `HitBy`, `NotHitBy`, and `HitOverride` belong with hit/get-hit seams.

## Boundaries

Audit boundaries:

- No CMake edits.
- No content edits.
- No sidecar policy changes.
- No `.dragon/*.json` changes.
- No DLC branch or branch topology changes.
- No boss intro changes.
- No Dragon Mode changes.
- No store or crafting changes.

## Validation

Expected validation for the follow-up velocity source cut:

```powershell
python engine/tools/dev_check.py .
python tools/check_file_sizes.py
git diff --check
```

`dev_check.py` should pass from a Visual Studio developer shell. `tools/check_file_sizes.py` should still fail only known `App.cpp` hard debt and report `App.cpp` at `8533`.

## Completed Implementation

```text
Completed code pass:
Moved velocity-only controller execution into an App.cpp-internal header.

Header:
StateControllerVelocityRuntime.h

Moved only:
- VelSet / VelAdd / VelMul execution from updateStateMovementControllers(...)

Still not moved:
- PosAdd
- PosSet
- PosFreeze
- CtrlSet
- StateTypeSet
- ScreenBound
- Width
- PlayerPush
- SprPriority
- Turn
- HitVelSet / HitFallVel / HitFallSet / HitFallDamage
- HitBy / NotHitBy / HitOverride
- HitDef / damage / guard
- KO, time-over, double-KO, or round flow
- helper / projectile / explod lifecycle
- ChangeState / SelfState
- pause / superpause
```

`StateControllerVelocityRuntime.h` is 43 lines. The cut reduced `App.cpp` from `8561` to `8533` file-size-guard lines. Full `dev_check.py` passed through a Visual Studio developer shell with `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` all reporting `partial=0 fail=0 blocked=0`.

## Current Follow-Up Recommendation

```text
Next audit:
Position / grounding controller audit.

Audit before moving:
- PosAdd
- PosSet
- PosFreeze

Do not move:
- CtrlSet
- StateTypeSet
- ScreenBound
- Width
- PlayerPush
- SprPriority
- Turn
- HitVelSet / HitFallVel / HitFallSet / HitFallDamage
- HitBy / NotHitBy / HitOverride
- HitDef / damage / guard
- KO, time-over, double-KO, or round flow
- helper / projectile / explod lifecycle
- ChangeState / SelfState
- pause / superpause
```
