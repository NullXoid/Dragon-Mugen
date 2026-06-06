# Runtime Controller Execution Audit

This docs-only audit records the under-10k `App.cpp` checkpoint and maps the remaining live runtime controller execution area before the next code move.

Baseline:

- HEAD: `9b0e722 Move command state eligibility body out of App.cpp`
- `engine/src/App.cpp`: `9922` file-size-guard lines
- Architecture reduction: `16820 -> 9922`, `6898` lines removed, `41.0%`
- Fresh verifier summaries:
  - `kfm-baseline`: `pass=11 partial=1 fail=0 blocked=0`
  - `evilken-smoke`: `pass=7 partial=0 fail=0 blocked=0`
  - `kfm-air-state`: `pass=11 partial=0 fail=0 blocked=0`
  - `cpu-baseline`: `pass=7 partial=0 fail=0 blocked=0`

Completed body cuts now include:

- world/stage/actor/effect/projectile rendering
- audio runtime and mixer body
- runtime loading and resource load/destroy body
- command parsing
- StateDef/controller parsing
- fight/session setup and reset
- live command recognition and input-buffer matching
- command-state eligibility and active command selection

The remaining `App.cpp` code is now mostly runtime core. The next moves must separate read-only evaluation from live mutation before moving broader CNS/controller execution.

## Runtime Core Inventory

| Area | Current Responsibility | Mutation | Readiness | Notes |
|---|---|---|---|---|
| Expression value helpers | M.U.G.E.N expression parsing/evaluation, variable reads, helper/projectile/target lookup, constants, distance values, `GetHitVar`, `NumHelper`, `NumProj`, `ProjContact`, `RoundState` | Read-only | Ready for internal body move | Uses `AppState`, `FighterState`, `StageSlot`, helper/projectile lookup helpers, and command/history reads, but does not mutate state. |
| Trigger evaluation helpers | Trigger groups, `triggerall`, command `=` / `!=` groups, subject comparisons, boolean expressions, `stateControllerTriggerActive`, `simpleControllerTriggerActive` | Read-only | Ready for internal body move | This is the safest next cut if include order preserves early forward declarations. |
| Controller persistence gate | `stateRuntimeControllerAlreadyFired`, `markStateRuntimeControllerFired`, `shouldRunStateRuntimeController` | Mutates fired-controller history | Defer | This is not pure evaluation because it writes to `fighter.firedStateRuntimeControllerIds`. Keep with controller execution for now. |
| Audio and Ctrl controllers | PlaySnd/StopSnd dispatch, `CtrlSet`, audio channel stop, state sound fire keys | Mutates audio and fighter controller history/ctrl flags | Defer | Audio runtime body is separate, but gameplay decides when sounds fire here. |
| AssertSpecial controllers | Adds/clears per-fighter assert flags and fight-wide flags | Mutates fighter flags | Defer | This belongs to live controller execution, not read-only trigger evaluation. |
| Variable and meter controllers | `VarSet`, `VarAdd`, `VarRandom`, `VarRangeSet`, `PowerAdd`, `LifeAdd`, `HitAdd`, attack/defense multipliers | Mutates fighter vars, life, power, hit count, multipliers | Defer | High behavior risk; do not move in the trigger-evaluation pass. |
| Visual and movement controllers | PalFX/BGPalFX/EnvColor, ChangeAnim/ChangeAnim2, PosAdd/PosSet/VelSet/VelAdd/VelMul, Offset, Angle, Trans, AfterImage, Width, PlayerPush, ScreenBound | Mutates animation, palette/display/runtime movement state | Defer | Some helper functions are pure, but grouped controller execution is mutation-heavy. |
| Helper/projectile/explod controllers | Helper spawn/destroy, Projectile spawn/update, Explod/ModifyExplod/RemoveExplod, MakeDust/GameMakeAnim | Mutates runtime actor/effect/projectile collections | Defer | Requires a separate lifecycle audit before movement. |
| State transition controllers | ChangeState/SelfState and anim-end state changes | Mutates state through `enterState` / `applyParsedChangeState` | Defer | Keep transitions in `App.cpp` until a dedicated state-transition boundary exists. |
| Pause/SuperPause controllers | Global pause/superpause timers, owner move time, optional pause sound | Mutates global pause state and can play sound | Defer | Needs its own small pass if extracted later. |
| Target controllers | TargetState, TargetBind, TargetDrop, TargetFacing, TargetLifeAdd, TargetPowerAdd, TargetVel* | Mutates target links and target fighter state | Defer | Too close to hit/damage/guard behavior for the next cut. |
| Hit/damage/guard runtime | HitDef resolution, hit/guard selection, get-hit vars, sparks/sounds, pause, target state, projectile hit application | Mutates fighters, effects, sounds, targets, projectiles | Defer | Requires stronger dedicated tests before extraction. |
| Round/match flow | KO/time-over, round transitions, match result, timer, rematch/result routing | Mutates match phase and routing state | Defer | Keep out of runtime-controller execution moves. |

## Read-Only Versus Mutation Boundary

Read-only evaluation can move first:

- numeric/string expression evaluation
- trigger subject value lookup
- trigger group evaluation
- command trigger checks that read current input history
- animation/time/distance comparisons
- state-controller trigger activation checks

Runtime mutation must stay in `App.cpp` for now:

- marking controller IDs as fired
- running controller side effects
- state transition through `enterState`
- life/power/var/velocity/animation mutation
- target/helper/projectile/explod lifecycle mutation
- hit/damage/guard/get-hit behavior
- round/match phase decisions

## Recommended Next Implementation Pass

Recommendation:

```text
Recovery Pass: Move Runtime Expression Evaluation Body Out Of App.cpp
```

Reason:

- It is the largest safe read-only runtime-core cut left.
- It separates expression/trigger evaluation from controller execution without moving gameplay side effects.
- It should reduce `App.cpp` while preserving the live controller mutation boundary.

Move into an App.cpp-internal implementation header, suggested name:

```text
engine/src/RuntimeExpressionEvaluation.h
```

Move only read-only evaluation helpers:

- `compareIntValue`
- `compareFloatValue`
- `animationTimeValue`
- `stateTriggerSubjectValue`
- `mugenRoundStateValue`
- `parseMugenFloatRangeExpression`
- `evalMugenFunctionExpression`
- `evalMugenExpression`
- `evalMugenExpressionCondition`
- `stateTriggerGroupActive`
- `anyStateTriggerGroupActive`
- `stateControllerTriggerActive`
- `simpleControllerTriggerActive`

Keep early forward declarations in `App.cpp` for helpers used before the include point, then include the header where the existing expression/evaluation body currently lives.

Do not move:

- `stateRuntimeControllerAlreadyFired`
- `markStateRuntimeControllerFired`
- `shouldRunStateRuntimeController`
- any `updateState*Controllers` body
- `applyParsedChangeState`
- `enterState`
- hit/damage/guard helpers
- helper/projectile/explod lifecycle helpers
- target controller helpers
- round/match flow helpers
- CPU/controller/input routing
- sidecar policy
- boss intro behavior
- `.dragon/*.json`

Expected verification:

- build
- `dev_check`
- file-size guard showing a lower `App.cpp` count
- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- `git diff --check`

Expected risk:

- Medium because expression evaluation is used broadly.
- Lower than moving controller execution because the recommended cut is read-only and leaves persistence/state mutation in place.
