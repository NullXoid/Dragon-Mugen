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

Expression evaluation move update:

- `engine/src/RuntimeExpressionEvaluation.h` now owns read-only runtime expression and trigger evaluation.
- `App.cpp` dropped from `9922` to `9135` file-size-guard lines in the committed expression-evaluation cut.
- `RuntimeExpressionEvaluation.h` is 795 lines and remains App.cpp-internal.
- Controller fired-history helpers, `shouldRunStateRuntimeController`, controller execution bodies, state transitions, hit/damage, helper/projectile/explod lifecycle, target mutation, round flow, sidecar policy, boss intro behavior, and `.dragon/*.json` stayed out of the moved header.
- Build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed after the move.

## Runtime Core Inventory

| Area | Current Responsibility | Mutation | Readiness | Notes |
|---|---|---|---|---|
| Expression value helpers | M.U.G.E.N expression parsing/evaluation, variable reads, helper/projectile/target lookup, constants, distance values, `GetHitVar`, `NumHelper`, `NumProj`, `ProjContact`, `RoundState` | Read-only | Moved | Now lives in `RuntimeExpressionEvaluation.h`. |
| Trigger evaluation helpers | Trigger groups, `triggerall`, command `=` / `!=` groups, subject comparisons, boolean expressions, `stateControllerTriggerActive`, `simpleControllerTriggerActive` | Read-only | Moved | Now lives in `RuntimeExpressionEvaluation.h`. |
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
Runtime Controller Execution Breakdown Audit
```

Reason:

- The read-only expression/trigger evaluation body has moved.
- The next remaining controller area is mutation-heavy and must not be moved as one broad cut.
- A focused audit should classify controller execution into safe/simple helpers, state-transition helpers, hit/damage helpers, helper/projectile/explod lifecycle helpers, target helpers, and pause/superpause helpers before the next implementation pass.

The audit should choose exactly one next completed code cut. Candidate cuts to evaluate:

- controller fired-history and persistence gate extraction
- simple visual/movement controller helpers only if they can move without changing mutation order
- pause/superpause body extraction
- defer all controller execution and instead audit hit/damage or helper/projectile/explod lifecycle

Do not move:

- broad controller execution
- `ChangeState` / `SelfState`
- `enterState`
- hit/damage/guard
- helper/projectile/explod lifecycle
- target mutation
- round/match flow
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
