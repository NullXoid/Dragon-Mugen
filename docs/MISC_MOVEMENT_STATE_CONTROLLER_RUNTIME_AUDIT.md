# Misc Movement State Controller Runtime Audit

This docs-only audit classifies the remaining smaller movement, state-shape, bounds, and presentation-adjacent controller execution after `2a096fb Move PosSet controller runtime body out of App.cpp`.

## Current Baseline

| Item | Value |
| --- | ---: |
| Branch | `main` |
| Required baseline commit | `2a096fb` |
| `App.cpp` count | 8507 |
| Starting `App.cpp` count | 16820 |
| Total removed | 8313 |
| Reduction | 49.4% |
| Remaining to 50% reduction | 97 |

The full runtime verifier gate is green at this checkpoint. `StateControllerVelocityRuntime.h`, `StateControllerPosAddRuntime.h`, and `StateControllerPosSetRuntime.h` already own their completed bounded movement/position loops. `docs/DEVELOPMENT.md` already points to the current roadmap audits and already records the tiered file-size policy, so it does not need a stale-doc update for this pass.

## Controller Classification

| Controller | Current mutation | Coupling | Readiness | Recommendation |
| --- | --- | --- | --- | --- |
| `SprPriority` | assigns `fighter.sprPriority` | presentation ordering only unless later inspection finds hidden side effects | `READY WITH LIMITS` | next code pass |
| `PosFreeze` | ORs `fighter.posFreezeX` / `fighter.posFreezeY` with controller flags | movement-freeze flags are consumed by later physics movement | `NEEDS MOVEMENT-FREEZE CHECK` | defer |
| `Turn` | flips `fighter.facing`, clears `jumpBaseAction`, clears `jumpPeakActionApplied` | command facing, input interpretation, jump animation, and contact-facing sensitive | `NEEDS FACING VERIFIER` | defer |
| `CtrlSet` | sets `fighter.ctrl` through separate fired-history helpers | command/control-runtime coupled and uses different trigger/fired semantics from the standard runtime helper | `DEFER` | defer |
| `StateTypeSet` | mutates `stateType`, `moveType`, `physics`, and derives `onGround` from `stateType` | state-shape, physics, grounding, command eligibility, and landing sensitive | `DEFER` | defer |
| `ScreenBound` | mutates screen-bound and camera-move flags | camera/bounds adjacent | `NEEDS CAMERA/BOUNDS SEAM` | defer |
| `Width` | mutates edge and player width values | collision/body/contact-adjacent | `NEEDS CONTACT/COLLISION SEAM` | defer |
| `PlayerPush` | mutates `fighter.playerPush` | collision/contact-adjacent and consumed when fighters are grounded | `NEEDS CONTACT/COLLISION SEAM` | defer |

## Audit Notes

`SprPriority` is the only controller in this group that is currently small enough for the next completed body move without adding a new verifier first. Its live body is:

```text
shouldRunStateRuntimeController(...)
fighter.sprPriority = sprPriority.value
```

It does not mutate position, velocity, control, state type, physics, hit state, target state, lifecycle state, pause timing, round flow, or camera bounds.

`PosFreeze` is also small, but it is not only presentation state. It sets movement-freeze flags that alter later physics movement, so it should get its own movement-freeze audit before any body move. Do not bundle it with `SprPriority`.

`Turn` is not a safe presentation-only move. It flips facing and also resets jump-action tracking, so it can affect command direction interpretation, facing-sensitive controller math, hit/contact direction, and jump presentation. It needs a facing verifier or a separate facing audit before movement.

`CtrlSet` should stay separate because it does not use the same `shouldRunStateRuntimeController(...)` path as the movement-loop controllers. It has its own trigger evaluation and fired-history helpers, and it directly changes command/control availability.

`StateTypeSet`, `ScreenBound`, `Width`, and `PlayerPush` sit near the high-risk areas already deferred by the active roadmap: grounding/physics, camera/bounds, and collision/contact behavior. They should not be moved as part of a misc controller bundle.

## Boundaries

This audit is docs-only:

- No source edits.
- No CMake edits.
- No content edits.
- No sidecar changes.
- No `.dragon/*.json` changes.
- No DLC branch or branch topology changes.
- No boss intro, Dragon Mode, store, or crafting work.

Do not recommend moving:

- `PosFreeze`.
- `Turn`.
- `CtrlSet`.
- `StateTypeSet`.
- `ScreenBound`.
- `Width`.
- `PlayerPush`.
- HitDef / damage / guard.
- Hit/get-hit controllers.
- Round flow.
- Helper / projectile / explod lifecycle.
- Target controllers.
- `ChangeState` / `SelfState`.
- Pause / superpause.
- Broad movement physics.
- `FighterState` / `AppState` extraction.

## Validation

Expected validation for this docs-only checkpoint:

```powershell
python engine/tools/dev_check.py . --skip-build
python tools/check_file_sizes.py
git diff --check
```

`dev_check.py --skip-build` should pass. `tools/check_file_sizes.py` should still fail only known `App.cpp` hard debt and report `App.cpp` at `8507`. Full runtime verifier reruns are optional because this checkpoint changes documentation only.

## SprPriority Body Move Follow-Up

The follow-up implementation pass moved only `SprPriority` execution from `updateStateMovementControllers(...)` into `engine/src/StateControllerSprPriorityRuntime.h`.

Moved helper:

```cpp
void updateStateSprPriorityControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage)
```

The helper preserves the original `shouldRunStateRuntimeController(...)` gate and `fighter.sprPriority = sprPriority.value`.

Measured follow-up result:

| Item | Value |
| --- | ---: |
| `App.cpp` before | 8507 |
| `App.cpp` after | 8503 |
| `App.cpp` reduction | 4 |
| `StateControllerSprPriorityRuntime.h` count | 18 |
| Remaining to 50% reduction | 93 |

The pass did not cross the 50% reduction threshold, so no separate 50% checkpoint was recorded.

Follow-up validation:

```text
kfm-baseline: pass=12 partial=0 fail=0 blocked=0
evilken-smoke: pass=9 partial=0 fail=0 blocked=0
kfm-air-state: pass=12 partial=0 fail=0 blocked=0
cpu-baseline: pass=7 partial=0 fail=0 blocked=0
```

`PosFreeze`, `Turn`, `CtrlSet`, `StateTypeSet`, `ScreenBound`, `Width`, `PlayerPush`, velocity controllers, `PosAdd`, `PosSet`, hit/get-hit controllers, HitDef/damage/guard, round flow, helper/projectile/explod lifecycle, target controllers, `ChangeState` / `SelfState`, pause/superpause, CPU/input behavior, sidecar policy, grounding physics, content, CMake, branch topology, and `.dragon/*.json` stayed unchanged.

## Updated Audit Conclusion

```text
Next pass:
Audit PosFreeze / movement-freeze controller behavior before any source movement.

Recommended audit:
PosFreeze / movement-freeze audit

Do not move:
- Turn
- CtrlSet
- StateTypeSet
- ScreenBound
- Width
- PlayerPush
- VelSet / VelAdd / VelMul
- PosAdd
- PosSet
- HitVelSet / HitFallVel / HitFallSet / HitFallDamage
- HitBy / NotHitBy / HitOverride
- HitDef / damage / guard
- KO, time-over, double-KO, or round flow
- helper / projectile / explod lifecycle
- target controllers
- ChangeState / SelfState
- pause / superpause
- broad movement physics
- FighterState / AppState extraction
```
