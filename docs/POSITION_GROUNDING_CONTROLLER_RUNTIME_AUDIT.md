# Position / Grounding Controller Runtime Audit

This docs-only audit classifies position and grounding controller risk after `ab14063 Move velocity controller runtime body out of App.cpp`.

## Current Baseline

| Item | Value |
| --- | ---: |
| Branch | `main` |
| Required baseline commit | `ab14063` |
| `App.cpp` count | 8533 |
| Starting `App.cpp` count | 16820 |
| Total removed | 8287 |
| Reduction | 49.3% |
| Remaining to 50% reduction | 123 |

The full runtime verifier gate is green at this checkpoint. `StateControllerVelocityRuntime.h` already owns the `VelSet` / `VelAdd` / `VelMul` loop, so this audit focuses only on the remaining position and grounding-adjacent controller bodies.

## Controller Classification

| Controller | Mutation | Coupling | Readiness | Recommendation |
| --- | --- | --- | --- | --- |
| `PosAdd` | adds to `fighter.x` / `fighter.y`; X is facing-aware | separate simple-trigger fired history when no trigger expression is present | `READY WITH LIMITS` | next code cut |
| `PosSet` | sets `fighter.x` / `fighter.y`; Y assignment updates `fighter.onGround` | grounding and air-state behavior | `NEEDS POSITION/GROUNDING SEAM` | defer |
| `PosFreeze` | updates `fighter.posFreezeX` / `fighter.posFreezeY` | movement-freeze behavior inside `updateStateMovementControllers(...)` | `NEEDS MOVEMENT-FREEZE AUDIT` | defer |

## Audit Findings

`PosAdd` is the only position controller ready for the next implementation pass. It is isolated in `updateStatePosAddControllers(...)`, applies the current facing-aware X offset, adds Y directly, and has its own `statePosAddAlreadyFired(...)` / `markStatePosAddFired(...)` simple-trigger path. A future helper can preserve behavior if it owns only this body and is included where `updateStatePosAddControllers(...)` currently lives.

`PosSet` is not equivalent to `PosAdd`. It sets absolute position and changes `fighter.onGround` from the assigned Y value, so it is tied to grounding and air-state behavior. `PosFreeze` is also separate because it changes movement-freeze flags inside the main movement controller body.

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

`docs/TYPE_EXTRACTION_AUDIT.md` remains historical context. The live `docs/DEVELOPMENT.md` already records the current tiered file-size guard policy and does not need an update for this audit.

## Validation

Expected validation for this docs-only checkpoint:

```powershell
python engine/tools/dev_check.py . --skip-build
python tools/check_file_sizes.py
git diff --check
```

`dev_check.py --skip-build` should pass. `tools/check_file_sizes.py` should still fail only known `App.cpp` hard debt and report `App.cpp` at `8533`. Full runtime verifier reruns are optional because this checkpoint changes documentation only.

## Boundary Note

If `PosAdd` is recommended next, it must remain `PosAdd`-only.
Do not include `PosSet` just because both touch position.

## Required Next Implementation Recommendation

```text
Next code pass:
Move PosAdd-only controller execution into an App.cpp-internal header.

Recommended header:
StateControllerPosAddRuntime.h

Move only:
- PosAdd execution from updateStatePosAddControllers(...)

Do not move:
- PosSet
- PosFreeze
- CtrlSet
- StateTypeSet
- ScreenBound
- Width
- PlayerPush
- SprPriority
- Turn
- VelSet / VelAdd / VelMul
- HitVelSet / HitFallVel / HitFallSet / HitFallDamage
- HitBy / NotHitBy / HitOverride
- HitDef / damage / guard
- KO, time-over, double-KO, or round flow
- helper / projectile / explod lifecycle
- ChangeState / SelfState
- pause / superpause
```
