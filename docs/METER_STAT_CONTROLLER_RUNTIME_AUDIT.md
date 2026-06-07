# Meter / Stat Controller Runtime Audit

This docs-only audit classifies the remaining meter/stat controller execution after `6b04904 Move variable controller runtime body out of App.cpp`.

## Current Baseline

| Item | Value |
|---|---:|
| Branch | `main` |
| Required baseline commit | `6b04904` |
| `App.cpp` count | 8571 |
| Current body | `updateStateMeterControllers(...)` |

`updateStateMeterControllers(...)` still lives in `engine/src/App.cpp` after `StateControllerVariableRuntime.h` and before movement/position controller execution. This audit does not move source code, change runtime behavior, or alter controller order.

## Controller Classification

| Controller | Mutation | Coupling | Readiness | Recommendation |
|---|---|---|---|---|
| `PowerAdd` | Mutates `fighter.power`, clamped to `[0, maxPower]`. | Meter-only in the current body unless later inspection finds hidden coupling. | `READY WITH LIMITS` | Next code pass. |
| `LifeAdd` | Mutates `fighter.life`, clamped by the controller `kill` flag. | KO, double-KO grace, timer decision, and round finish checks consume life directly. | `NEEDS ROUND-FLOW SEAM` | Defer. |
| `HitAdd` | Mutates `fighter.hitCount`. | Hit and projectile contact also increment hit count, and hitcount can affect trigger behavior. | `NEEDS HIT/DAMAGE SEAM` | Defer. |
| `AttackDist` | Mutates `fighter.attackDistanceOverride`. | Guard/contact distance uses the override during hit checks. | `NEEDS HIT/CONTACT SEAM` | Defer. |
| `AttackMulSet` | Mutates `fighter.attackMultiplier`. | Outgoing damage scaling consumes the multiplier during hit and projectile damage calculation. | `NEEDS HIT/DAMAGE SEAM` | Defer. |
| `DefenceMulSet` | Mutates `fighter.defenceMultiplier`. | Incoming hit and guard damage scaling consumes the multiplier. | `NEEDS HIT/DAMAGE SEAM` | Defer. |

## Audit Findings

`PowerAdd` is the only controller in this group that appears bounded enough for the next completed body move. Its current execution evaluates its trigger and value, then clamps `fighter.power` against `[Data] power` through `state.characterConstants.maxPower`. It does not directly touch life, damage, hit flags, target links, helper/projectile/explod lifecycle, state transitions, or round flow.

`LifeAdd` should not move with `PowerAdd`. Even though its body is small, it changes `fighter.life`, and life is the direct input to KO, double-KO, time-over decision, round finish, match result, HUD, and related grace-period handling.

`HitAdd`, `AttackDist`, `AttackMulSet`, and `DefenceMulSet` should remain deferred until hit/contact/damage boundaries are audited. Their fields are consumed by hit count triggers, guard distance, and damage scaling, so moving them as a general meter/stat bundle would blur the hit/damage seam.

`TargetPowerAdd` and `TargetLifeAdd` are not part of this audit. They live with target-controller mutation and should remain deferred to the target-controller audit.

## Boundaries

This audit is documentation only:

- No source edits.
- No CMake edits.
- No content edits.
- No sidecar policy changes.
- No `.dragon/*.json` edits.
- No DLC branch or branch topology changes.

Do not recommend broad CNS/runtime movement, HitDef/damage movement, round-flow movement, boss intro/preemptive attack work, Dragon Mode, store, or crafting work from this checkpoint.

## Required Next Implementation Recommendation

```text
Next code pass:
Move PowerAdd-only controller execution into an App.cpp-internal header.

Recommended header:
StateControllerPowerRuntime.h

Move only:
- PowerAdd execution from updateStateMeterControllers(...)

Do not move:
- LifeAdd
- HitAdd
- AttackDist
- AttackMulSet
- DefenceMulSet
- TargetPowerAdd / TargetLifeAdd
- HitDef / damage / guard
- KO, time-over, double-KO, or round flow
- movement / position controllers
- helper / projectile / explod lifecycle
- ChangeState / SelfState
- pause / superpause
```
