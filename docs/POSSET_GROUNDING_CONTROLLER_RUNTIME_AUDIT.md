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

Not enough. `kfm-air-state` proves normal diagonal jumps and air attack landing behavior, but it does not explicitly prove that an authored `PosSet` controller fired and updated `onGround`. KFM does have authored `PosSet` controllers in special landing states such as `1052` and `1056`, but the current air-state verifier does not assert those states or a PosSet fire path.

Is extra verifier coverage required before a code move?

Yes. A dedicated grounding check should prove an authored PosSet path sets `y` to ground level and leaves the fighter grounded without regressing the existing diagonal jump and air-attack checks. That can be a new verifier scenario or an extension to `kfm-air-state`, but it should explicitly exercise a PosSet-backed landing state rather than only generic physics landing.

Is `PosFreeze` independent enough to move later?

Yes. `PosFreeze` updates `fighter.posFreezeX` and `fighter.posFreezeY` and affects later physics movement, but it does not assign `x`, `y`, or `onGround` directly. It should remain separate for a movement-freeze audit.

## Boundaries

Docs-only:

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

## Validation

Expected validation for this docs-only checkpoint:

```powershell
python engine/tools/dev_check.py . --skip-build
python tools/check_file_sizes.py
git diff --check
```

`dev_check.py --skip-build` should pass. `tools/check_file_sizes.py` should still fail only known `App.cpp` hard debt and report `App.cpp` at `8517`. Full runtime verifier reruns are optional because this checkpoint changes documentation only.

## Required Audit Conclusion

```text
Next code pass:
Option B

Option B:
Create or extend a grounding verifier first.
```
