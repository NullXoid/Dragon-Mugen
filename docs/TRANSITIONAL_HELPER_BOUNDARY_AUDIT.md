# Remaining Transitional Helper Boundary Audit

This docs-only audit records the remaining App.cpp-internal helper and bridge headers after the front-end, fight, and training overlay conversions.

Baseline:

- HEAD: `aa92131 Convert training debug overlay to normal module`
- `engine/src/App.cpp`: `15037` file-size-guard lines
- `kfm-baseline`: `pass=11 partial=1 fail=0 blocked=0`
- `evilken-smoke`: `pass=7 partial=0 fail=0 blocked=0`
- `kfm-air-state`: `pass=11 partial=0 fail=0 blocked=0`
- `cpu-baseline`: `pass=7 partial=0 fail=0 blocked=0`

This pass does not change source, CMake, content files, runtime behavior, controller behavior, sidecar policy, or `.dragon/*.json`.

## Readiness Labels

- `READY FOR CLEANUP`: can be retired or simplified without creating a new public runtime/resource boundary.
- `NEEDS PUBLIC PRESENTATION SEAM`: needs prepared view data or public non-owning render helpers before becoming a normal module.
- `NEEDS ACTION RETURN SEAM`: needs behavior to be represented as explicit actions before the header can become a normal module.
- `NEEDS RUNTIME SNAPSHOT SEAM`: needs a stable runtime snapshot or probe boundary before conversion.
- `DEFER`: depends on runtime-heavy systems and should remain App.cpp-internal for now.

## Header Audit

| Header | Lines | Responsibility | Main Dependencies | Readiness | Main Blocker | Minimum Seam Needed | Recommended Priority | Notes |
|---|---:|---|---|---|---|---|---:|---|
| `FightPresentationShared.h` | 60 | Shared fight status-line helper and leftover round-pip renderer | `AppState`, `MatchPhase`, round/result label helpers, `fightRoundSettings`, gamepad status helpers, UI primitives | `READY FOR CLEANUP` | `singleFightStatusLine` is AppState-coupled view assembly; `drawRoundWinPips` is no longer used after fight overlay conversion | No public seam. Keep status-line assembly App.cpp-local and delete unused round-pip rendering if confirmed still unused | 1 | Best next cleanup. Retires a transitional helper instead of pretending the AppState-coupled status helper belongs in a normal module. |
| `UiRenderHelpers.h` | 94 | Title/select background drawing and old fixed-opponent placeholder helper | `AppState`, `SystemScreenAssets`, `TextureSprite`, current frame, logical-width helpers, App.cpp-local tiled/parallax/blend sprite helpers | `NEEDS PUBLIC PRESENTATION SEAM` | Resource-backed background drawing still uses App.cpp-local sprite helpers and resource-owned `TextureSprite` data | Public background/sprite presentation seam with non-owning `UiSpriteView` fields and public tiled/parallax/blend draw helpers; App.cpp must keep asset ownership | 2 | Good follow-up after shared fight cleanup. Do not move texture creation, caches, or system-screen ownership. |
| `TrainingDebugViewAssembly.h` | 215 | F1 debug view assembly and screen-space projection | `AppState`, `FighterState`, `StageSlot`, `AnimationFrame`, `CollisionBox`, camera/stage offsets, debug clipboard data | `DEFER` | It computes runtime-derived debug projection from fighter/stage/animation state | Fighter/debug projection boundary after broader runtime state seams exist | 5 | Keep as an internal bridge. It exists specifically so `TrainingDebugOverlay` can remain a normal render-only module. |
| `FrontendFlow.h` | 411 | Existing key-flow dispatcher and gamepad-to-menu-key bridge | `AppState`, SDL key/gamepad types, routing/loading helpers, fight reset/start helpers, training side-effect helpers, pause/result state | `NEEDS ACTION RETURN SEAM` | It mutates state and calls App.cpp-local side-effect helpers directly | Frontend action-return boundary where a normal module returns route/reset/training/menu actions and App.cpp executes side effects | 3 | Worth a later audit/implementation pass, but it is behavior-coupled and should not be normalized by passing AppState into a module. |
| `AppVerificationBridge.h` | 204 | Verification runtime adapter for scripted scenarios | `AppState`, `FighterState`, SDL window/renderer lifetime, audio/visual asset lifecycle, fight setup/update, runtime snapshots | `NEEDS RUNTIME SNAPSHOT SEAM` | It intentionally reaches across many internal runtime systems to drive verifiers | Stable runtime harness or snapshot boundary after AppState/FighterState are less internal | 6 | Keep as a bridge until runtime state boundaries are stronger. Do not weaken verifier coverage to make it look modular. |

## Dependency Notes

`FightPresentationShared.h` is no longer a broad fight-presentation layer. `FightHudOverlay.cpp` and `FightResultOverlay.cpp` now own their render-only helpers privately. The only live use found in this pass is `singleFightStatusLine(state)` while assembling `FightHudView` in `App.cpp`; the old `drawRoundWinPips(...)` helper is not referenced by current source.

`UiRenderHelpers.h` remains the only UI presentation helper with meaningful render/resource coupling. It is a plausible next real boundary after shared fight cleanup, but it needs a background/sprite view seam so normal modules do not receive `AppState`, `SystemScreenAssets`, `TextureSprite`, or cache-owning types.

`TrainingDebugViewAssembly.h` is not a failed extraction. It is the intended App.cpp-internal bridge that keeps raw fighter/stage/collision projection out of `TrainingDebugOverlay.cpp`.

`FrontendFlow.h` is the largest remaining transitional header, but its conversion is behavior work, not render work. A safe conversion needs a frontend action model where App.cpp remains responsible for loading, resetting, routing, and runtime side effects.

`AppVerificationBridge.h` should stay close to App.cpp internals until runtime-state seams are stronger. It protects the current verifier suite and should not be split just for line-count optics.

## Recommended Next Implementation Pass

Recommendation:

```text
Pass 42: Retire FightPresentationShared transitional helper
```

Reason:

- It is the smallest remaining transitional helper with a clear inspection result.
- The only live helper is AppState-coupled HUD view assembly, so keeping that logic App.cpp-local is the correct boundary.
- The unused `drawRoundWinPips(...)` helper can be removed if a fresh `rg` check still confirms no references.
- This reduces hidden transitional-header coupling without moving runtime, match/round decisions, result semantics, or resource ownership.

Expected files to modify:

- `engine/src/App.cpp`
- `engine/src/FightPresentationShared.h`
- docs touched by the pass

Expected source shape:

- Remove the `FightPresentationShared.h` include from `App.cpp`.
- Keep `singleFightStatusLine(...)` App.cpp-local near fight HUD view assembly or another local presentation-helper area.
- Delete `FightPresentationShared.h` if it becomes empty.
- Do not create a normal module for `singleFightStatusLine(...)` while it still reads `AppState`.

Estimated risk: low, if the pass only removes the unused render helper and keeps status-line semantics in App.cpp.

Expected `App.cpp` line impact: flat or a small increase. The value is retiring a transitional helper and deleting unused helper code, not reducing `App.cpp`.

Expected hidden-coupling reduction: one App.cpp-internal presentation helper is removed, and fight status-line ownership is made explicit in App.cpp view assembly.

Verification focus:

- Build
- `dev_check`
- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- `git diff --check`

## Do Not Move

Do not move or redesign:

- CMD/CNS runtime
- hit/damage/guard
- round-flow decisions
- KO/time-over/double-KO logic
- result routing or result-menu mutation
- match score mutation
- victory quote selection policy
- `FighterState`
- `AppState` as a whole
- training debug projection
- routing/loading/fight startup side effects
- SDL/resource ownership
- audio
- CPU behavior
- controller behavior
- Dragon Mode
- store/crafting
- sidecar policy
- `.dragon/*.json`
- boss intro/preemptive attack behavior
