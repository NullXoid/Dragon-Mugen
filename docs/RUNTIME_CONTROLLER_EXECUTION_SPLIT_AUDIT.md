# Runtime Controller Execution Split Audit

Superseded note: this audit is historical context. Its recommended `StateControllerUtilityRuntime.h` cut was completed by `de22bec Move state controller utility runtime body out of App.cpp`. The current decision point is `docs/ARCHITECTURE_RECOVERY_ROADMAP_AUDIT.md`, which recommends a bounded variable-controller audit or `StateControllerVariableRuntime.h` extraction next.

This docs-only audit narrows the remaining live controller execution area after the read-only expression and trigger evaluation body moved to `RuntimeExpressionEvaluation.h`.

Baseline:

- HEAD: `2dab683 Improve training command usability`
- `engine/src/App.cpp`: `9186` file-size-guard lines
- Last completed architecture cut: `11f9090 Move runtime expression evaluation body out of App.cpp`
- Current runtime expression seam: `engine/src/RuntimeExpressionEvaluation.h`, 795 lines
- Fresh verifier summaries after this docs-only split audit:
  - `kfm-baseline`: `pass=11 partial=1 fail=0 blocked=0`
  - `evilken-smoke`: `pass=7 partial=0 fail=0 blocked=0`
  - `kfm-air-state`: `pass=11 partial=0 fail=0 blocked=0`
  - `cpu-baseline`: `pass=7 partial=0 fail=0 blocked=0`

## Controller Execution Risk Split

| Area | Current Responsibility | Risk | Suggested Handling | Notes |
|---|---|---|---|---|
| Controller persistence gate | `stateRuntimeControllerAlreadyFired`, `markStateRuntimeControllerFired`, `shouldRunStateRuntimeController` | Medium | Move with first utility controller body | It mutates fired-controller history, so it is not expression evaluation, but it is a small central gate used by most runtime controllers. |
| Audio controllers | PlaySnd/StopSnd runtime dispatch, sound fire-key tracking, state audio controller iteration | Medium | Move with utility controller body | Audio ownership already lives in `AudioRuntime.h`; this body still decides controller-triggered sound playback and must preserve trigger/persistence order. |
| AssertSpecial controllers | Per-fighter assert flag reads, writes, clear, and fight-wide update | Medium | Move with utility controller body | Mutates transient flags used by HUD, round flow, timer freeze, background visibility, and movement restrictions. Keep exact per-tick clear/update order. |
| Visual/display utility controllers | Angle, AngleDraw, Offset, clipboard text, VictoryQuote, RemapPal, Trans, AfterImage, ForceFeedback | Medium | Move with utility/display body | Mutates display/runtime presentation fields and gamepad rumble only. Does not enter states, apply damage, spawn actors, or mutate targets. |
| Variable and meter controllers | VarSet/VarAdd/VarRandom/VarRangeSet, PowerAdd, LifeAdd, HitAdd, AttackDist, attack/defense multipliers | High | Defer | Mutates gameplay state and damage inputs. Keep in `App.cpp` until a dedicated mutation pass. |
| Pos/Add/ChangeAnim controllers | PosAdd and ChangeAnim simple-controller fallback tracking | Medium-high | Defer | They have separate fired-history vectors and directly affect animation/position. Move only after utility/display body is stable. |
| Movement/state-shape controllers | VelSet/VelAdd/VelMul, PosSet, StateTypeSet, ScreenBound, Width, PlayerPush, SprPriority, PosFreeze, Turn | High | Defer | Directly affects movement, collision bounds, camera interaction, and live fighter state. |
| Pause/SuperPause controllers | Global pause timers, owner move time, pause sound | High | Defer | Cross-cuts fighter update order, projectile update allowance, audio, and timers. |
| Helper lifecycle controllers | Helper spawn, BindToParent, BindToRoot, ParentVarAdd, DestroySelf | High | Defer | Spawns and mutates helper actors, owner variables, root binding, and destruction flags. |
| Projectile lifecycle controllers | Projectile spawn and projectile runtime update collision/removal | High | Defer | Touches projectile creation, hitdefs, collision, removal, and hit application. |
| Explod/effect controllers | Explod, ModifyExplod, RemoveExplod, MakeDust, GameMakeAnim | Medium-high | Defer | Mutates runtime effects; safer than HitDef but still lifecycle-heavy. Audit separately after controller utility extraction. |
| ChangeState/SelfState controllers | State transition expression evaluation and `applyParsedChangeState` | High | Defer | Calls `enterState` and changes the active state machine. Do not include in utility moves. |
| Target controllers | TargetDrop, TargetState, TargetBind, TargetFacing, TargetLifeAdd, TargetPowerAdd, TargetVel | High | Defer | Mutates target links and opponent state; too close to hit/damage behavior. |
| Hit/damage/guard runtime | HitDef selection, guard choice, hit override, get-hit vars, spark/sound, hitpause, projectile hit application | Very high | Defer | Requires a dedicated hit runtime audit and stronger live coverage. |
| Round/match flow | Timer, KO/time-over, round transitions, match result, rematch/result routing | Very high | Defer | Not a controller execution extraction target yet. |

## Boundary For The Next Code Cut

The next implementation pass should move a bounded controller utility/display body only. It may move code that mutates transient presentation, audio-controller, assert flag, fired-history, clipboard, victory quote, palette/remap, trans, afterimage, and rumble state.

It must not move:

- `setFighterVariableValue`
- `updateStateVariableControllers`
- `updateStateMeterControllers`
- `updateStatePosAddControllers`
- `updateStateChangeAnimControllers`
- `updateStateMovementControllers`
- `updateStateProjectileControllers`
- `updateStateHelperControllers`
- `updateStateExplodControllers`
- `updateStateChangeStateControllers`
- `updateStateTargetControllers`
- `enterState`
- `applyParsedChangeState`
- HitDef, damage, guard, projectile hit, round flow, CPU behavior, loading, sidecar policy, boss intro behavior, or `.dragon/*.json`

## Recommended Next Implementation Pass

Recommendation:

```text
Move controller utility and display body out of App.cpp
```

Target header:

```text
engine/src/StateControllerUtilityRuntime.h
```

Move only if inspection confirms the functions remain a cohesive internal implementation body:

- state sound fired-history helpers
- `stateSoundTriggerActive`
- `executePlaySoundController`
- controller runtime fired-history helpers
- `shouldRunStateRuntimeController`
- `executeStopSoundController`
- `executeStateAudioControllers`
- `updateStateAudioControllers`
- assert-special read/write/update helpers
- `changeAnimTriggerActive`
- `transBlendMode`
- clipboard formatting helpers
- `selectedVictoryQuoteText`
- palette-remap evaluation helper
- `updateStateVisualControllers`
- `runForceFeedbackController`

Reason:

- This is the smallest useful mutation-bearing controller cut after read-only trigger evaluation moved.
- It should reduce `App.cpp` while avoiding the highest-risk systems.
- It exercises controller execution extraction without touching state transitions, HitDef/damage, target mutation, helper/projectile/explod lifecycle, round flow, or CPU behavior.

Expected verification:

- build
- `dev_check`
- `git diff --check`
- file-size guard showing a lower `App.cpp` count
- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`

Manual smoke if practical:

- Training route reaches fight
- F1/F2 still work
- command/input HUD still renders
- fight HUD still honors timer/background/HUD visibility
- normal input, hit, reset, Esc backout, and clean exit still work
