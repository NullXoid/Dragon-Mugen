# PosSet / Grounding Controller Runtime Audit

This docs-only audit classifies `PosSet` and grounding-sensitive controller behavior after `d933a4d Move PosAdd controller runtime body out of App.cpp`.

## Current Baseline

| Item | Value |
| --- | ---: |
| Branch | `main` |
| Required baseline commit | `d933a4d` |
| `App.cpp` count | 8517 |
| Starting `App.cpp` count | 16820 |
| Total removed | 8303 |
| Reduction | 49.4% |
| Remaining to 50% reduction | 107 |

The full runtime verifier gate is green at this checkpoint. `StateControllerPosAddRuntime.h` already owns the `PosAdd` loop, and `StateControllerVelocityRuntime.h` already owns the `VelSet` / `VelAdd` / `VelMul` loop. `PosSet` remains inside `updateStateMovementControllers(...)`.

## Current PosSet Body

Current `PosSet` execution in `updateStateMovementControllers(...)` does only the following when `shouldRunStateRuntimeController(...)` passes:

- If `x` is present, assign `fighter.x = posSet.x`.
- If `y` is present, assign `fighter.y = posSet.y`.
- When `y` is present, assign `fighter.onGround = fighter.y >= 0.0f`.

It does not directly call landing helpers, change state, change animation, modify velocity, trigger round flow, mutate hit state, or touch helper/projectile/explod lifecycle.

## Grounding Touchpoints

`PosSet` is still grounding-sensitive because the `onGround` result is consumed by nearby and later runtime code:

| Area | Coupling | Audit Result |
| --- | --- | --- |
| `updateFighterPhysics(...)` | clamps `y >= 0`, zeros vertical velocity, sets `onGround`, clears jump peak state, and may enter common landing state | high confidence risk area |
| `updateStateZeroFromMovement(...)` | derives `stateType`, `physics`, and action from `onGround` | grounding-sensitive |
| `kfm-air-state` verifier | proves normal diagonal jumps and air attack landing end grounded without re-entering air | useful but not PosSet-specific |
| `PosFreeze` | prevents X/Y physics updates but does not set `onGround` directly | independent; keep separate |
| `StateTypeSet` | can set `stateType` and then updates `onGround` from that type | separate state-shape seam |
| hit/get-hit landing paths | set `onGround` during fall, recovery, and common get-hit behavior | hit/get-hit seam, not PosSet |
| player push | only applies when both fighters are grounded | downstream consumer, not PosSet ownership |

## Audit Answers

Does `PosSet` only mutate `x`, `y`, and `onGround`?

Yes. Inspection shows the controller body assigns only `fighter.x`, `fighter.y`, and `fighter.onGround`, with normal `shouldRunStateRuntimeController(...)` gating.

Does it directly trigger landing/physics behavior or only change state consumed later?

It only changes state consumed later. The landing clamp, common landing entry, velocity reset, and jump peak reset live in `updateFighterPhysics(...)`, not in the `PosSet` loop.

Can moving it preserve diagonal jump and air-attack landing fixes?

Probably, if moved as a direct body extraction with identical include order and call order. However, because `onGround` is a shared input to landing, movement animation, command eligibility, and player push, this should not be moved until the verifier explicitly covers a PosSet-triggered grounding path.

Does the existing `kfm-air-state` verifier cover PosSet-triggered grounding changes?

At audit time, not enough. `kfm-air-state` proved normal diagonal jumps and air attack landing behavior, but it did not explicitly prove that an authored `PosSet` controller fired and updated `onGround`. KFM has authored `PosSet` controllers in special landing states such as `1052` and `1056`, but the then-current air-state verifier did not assert those states or a PosSet fire path. The follow-up hardening below closes that verifier gap.

Is extra verifier coverage required before a code move?

Yes. A dedicated grounding check should prove an authored PosSet path sets `y` to ground level and leaves the fighter grounded without regressing the existing diagonal jump and air-attack checks. That can be a new verifier scenario or an extension to `kfm-air-state`, but it should explicitly exercise a PosSet-backed landing state rather than only generic physics landing.

Is `PosFreeze` independent enough to move later?

Yes. `PosFreeze` updates `fighter.posFreezeX` and `fighter.posFreezeY` and affects later physics movement, but it does not assign `x`, `y`, or `onGround` directly. It should remain separate for a movement-freeze audit.

## Verifier Hardening Follow-Up

The follow-up grounding verifier pass extended `kfm-air-state` with `kung_fu_knee_posset_grounding`. The new row drives KFM's authored `FF_a` path through normal symbolic input, CMD command recognition, CNS `ChangeState`, states `1050 -> 1051 -> 1052`, the `PosSet` controller in state `1052`, and grounded recovery back to controllable idle.

The row may pass only when all of these are observed:

- State `1050`.
- State `1051`.
- State `1052`.
- `y` approximately `0` in or after state `1052`.
- `onGround == true` in or after state `1052`.
- No airborne final state.
- Return to controllable idle.

The first verifier run exposed a real grounding semantics gap: KFM entered `1050` and `1051`, but the authored `1051 -> 1052` path did not fire before the runtime returned him to idle through generic behavior. The scoped fix keeps custom `physics = N` air states from being implicitly ground-clamped or auto-finished to idle before their authored landing controllers can run.

No controller execution moved. `PosSet`, `PosAdd`, `PosFreeze`, `StateTypeSet`, `CtrlSet`, hit/get-hit controllers, HitDef/damage/guard, round flow, helper/projectile/explod lifecycle, `ChangeState` / `SelfState`, and pause/superpause controller bodies stayed in `App.cpp`.

Follow-up validation:

```text
kfm-air-state:
pass=12 partial=0 fail=0 blocked=0

kung_fu_knee_posset_grounding:
saw_1050=1 saw_1051=1 saw_1052=1 posset_grounding=1 returned_idle=1 final_airborne=0
```

The full verifier gate passed with all advertised runtime scenarios at `partial=0 fail=0 blocked=0`. `App.cpp` remains `8517` file-size-guard lines, and `VerificationScenario.cpp` is `741` lines after the added proof row.

The verifier requirement is now satisfied for a future `PosSet`-only body move.

## PosSet Body Move Follow-Up

The follow-up implementation pass moved only `PosSet` execution from `updateStateMovementControllers(...)` into `engine/src/StateControllerPosSetRuntime.h`.

Moved helper:

```cpp
void updateStatePosSetControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage)
```

The helper preserves the original `shouldRunStateRuntimeController(...)` gate, optional absolute `fighter.x` assignment, optional absolute `fighter.y` assignment, and `fighter.onGround = fighter.y >= 0.0f`.

Measured follow-up result:

| Item | Value |
| --- | ---: |
| `App.cpp` before | 8517 |
| `App.cpp` after | 8507 |
| `App.cpp` reduction | 10 |
| `StateControllerPosSetRuntime.h` count | 24 |
| Remaining to 50% reduction | 97 |

The pass did not cross the 50% reduction threshold, so no separate 50% checkpoint was recorded.

Follow-up validation:

```text
kfm-baseline: pass=12 partial=0 fail=0 blocked=0
evilken-smoke: pass=9 partial=0 fail=0 blocked=0
kfm-air-state: pass=12 partial=0 fail=0 blocked=0
cpu-baseline: pass=7 partial=0 fail=0 blocked=0

kung_fu_knee_posset_grounding:
idle_before=1 saw_1050=1 saw_1051=1 saw_1052=1 posset_grounding=1 grounding_y=0.000000 grounding_on_ground=1 returned_idle=1 final_airborne=0
```

`PosAdd`, `PosFreeze`, `StateTypeSet`, `CtrlSet`, `ScreenBound`, `Width`, `PlayerPush`, `SprPriority`, `Turn`, velocity controllers, hit/get-hit controllers, HitDef/damage/guard, round flow, helper/projectile/explod lifecycle, `ChangeState` / `SelfState`, pause/superpause, CPU/input behavior, sidecar policy, grounding physics, content, CMake, branch topology, and `.dragon/*.json` stayed unchanged.

## Original Audit Boundaries

The audit commit itself was docs-only:

- No source edits.
- No CMake edits.
- No content edits.
- No sidecar policy changes.
- No `.dragon/*.json` changes.
- No DLC branch or branch topology changes.
- No boss intro changes.
- No Dragon Mode changes.
- No store or crafting changes.

Do not move or recommend moving in the same pass:

- HitDef / damage / guard.
- Hit/get-hit velocity controllers.
- Round flow.
- Helper / projectile / explod lifecycle.
- `ChangeState` / `SelfState`.
- Pause / superpause.
- `StateTypeSet`.
- `CtrlSet`.
- Broad movement physics.
- `FighterState`.
- `AppState`.

## Original Docs-Only Validation

Expected validation for this docs-only checkpoint:

```powershell
python engine/tools/dev_check.py . --skip-build
python tools/check_file_sizes.py
git diff --check
```

`dev_check.py --skip-build` should pass. `tools/check_file_sizes.py` should still fail only known `App.cpp` hard debt and report `App.cpp` at `8517`. Full runtime verifier reruns are optional because this checkpoint changes documentation only.

## Updated Follow-Up Conclusion

```text
Follow-up audit completed:
docs/MISC_MOVEMENT_STATE_CONTROLLER_RUNTIME_AUDIT.md

Follow-up implementation completed:
StateControllerSprPriorityRuntime.h
StateControllerPosFreezeRuntime.h

Next pass:
Audit Turn / facing controller behavior before any source movement.

Do not move:
- CtrlSet
- StateTypeSet
- ScreenBound
- Width
- PlayerPush
- Turn
- VelSet / VelAdd / VelMul
- PosAdd
- HitVelSet / HitFallVel / HitFallSet / HitFallDamage
- HitBy / NotHitBy / HitOverride
- HitDef / damage / guard
- KO, time-over, double-KO, or round flow
- helper / projectile / explod lifecycle
- ChangeState / SelfState
- pause / superpause
```
