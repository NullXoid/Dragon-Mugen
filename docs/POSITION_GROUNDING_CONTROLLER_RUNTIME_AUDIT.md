# Position / Grounding Controller Runtime Audit

This docs-only audit classifies position and grounding controller risk after `ab14063 Move velocity controller runtime body out of App.cpp`.

## Audit Baseline

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
| `PosAdd` | adds to `fighter.x` / `fighter.y`; X is facing-aware | separate simple-trigger fired history when no trigger expression is present | `READY WITH LIMITS` | completed follow-up |
| `PosSet` | sets `fighter.x` / `fighter.y`; Y assignment updates `fighter.onGround` | grounding and air-state behavior | `NEEDS POSITION/GROUNDING SEAM` | defer |
| `PosFreeze` | updates `fighter.posFreezeX` / `fighter.posFreezeY` | movement-freeze behavior inside `updateStateMovementControllers(...)` | `NEEDS MOVEMENT-FREEZE AUDIT` | defer |

## Audit Findings

`PosAdd` was the only position controller ready for the next implementation pass. It is isolated in `updateStatePosAddControllers(...)`, applies the current facing-aware X offset, adds Y directly, and has its own `statePosAddAlreadyFired(...)` / `markStatePosAddFired(...)` simple-trigger path. The completed helper preserves behavior by owning only this body and being included where `updateStatePosAddControllers(...)` currently lives.

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

The current recovery source of truth is `docs/ARCHITECTURE_RECOVERY_ROADMAP_AUDIT.md` plus the focused runtime-controller audits. The live `docs/DEVELOPMENT.md` already records the current tiered file-size guard policy and does not need an update for this audit.

## Audit Validation

Expected validation for this docs-only checkpoint:

```powershell
python engine/tools/dev_check.py . --skip-build
python tools/check_file_sizes.py
git diff --check
```

At audit time, `dev_check.py --skip-build` passed. `tools/check_file_sizes.py` still failed only known `App.cpp` hard debt and reported `App.cpp` at `8533`. Full runtime verifier reruns were optional because that checkpoint changed documentation only.

## Boundary Note

The completed `PosAdd` follow-up remained `PosAdd`-only.
Do not include `PosSet` just because both touch position.

## Completed Implementation Follow-Up

The recommended `PosAdd`-only follow-up is complete.

| Item | Value |
| --- | ---: |
| `App.cpp` before | 8533 |
| `App.cpp` after | 8517 |
| Removed in follow-up | 16 |
| Total removed from starting baseline | 8303 |
| Reduction from starting baseline | 49.4% |
| Remaining to 50% reduction | 107 |
| `StateControllerPosAddRuntime.h` count | 31 |

`StateControllerPosAddRuntime.h` owns only `updateStatePosAddControllersForDefinition(...)`, which contains the original `PosAdd` loop from `updateStatePosAddControllers(...)`. The helper preserves trigger checks, simple-trigger fired-history behavior, facing-aware X adjustment, direct Y addition, and the existing call order for `statePosAddAlreadyFired(...)` / `markStatePosAddFired(...)`.

Full `python engine/tools/dev_check.py .` passed from a Visual Studio developer environment with these runtime verifier summaries:

- `kfm-baseline`: `pass=12 partial=0 fail=0 blocked=0`.
- `evilken-smoke`: `pass=9 partial=0 fail=0 blocked=0`.
- `kfm-air-state`: `pass=11 partial=0 fail=0 blocked=0`.
- `cpu-baseline`: `pass=7 partial=0 fail=0 blocked=0`.

The follow-up did not move `PosSet`, `PosFreeze`, control/state-shape controllers, collision/contact-adjacent controllers, hit/get-hit controllers, HitDef/damage/guard, round flow, helper/projectile/explod lifecycle, `ChangeState` / `SelfState`, pause/superpause, CPU/input behavior, sidecar policy, or `.dragon/*.json`.

## Completed Implementation Recommendation

```text
Completed code pass:
Move PosAdd-only controller execution into an App.cpp-internal header.

Header:
StateControllerPosAddRuntime.h

Moved only:
- PosAdd execution from updateStatePosAddControllers(...)

Still not moved:
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

## Next Audit Recommendation

```text
Completed roadmap target:
PosSet / grounding seam audit.

Result:
Create or extend a grounding verifier before moving PosSet.
```
