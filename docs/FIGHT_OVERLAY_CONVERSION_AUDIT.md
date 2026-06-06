# Fight Overlay Conversion Audit

This docs-only audit checks whether the fight presentation headers can follow the normal-module pattern used by the front-end overlays.

Baseline:

- HEAD: `a509d77 Convert character select overlay to normal module`
- `engine/src/App.cpp`: `14704` file-size-guard lines
- Current verifier baseline:
  - `kfm-baseline`: `pass=11 partial=1 fail=0 blocked=0`
  - `evilken-smoke`: `pass=7 partial=0 fail=0 blocked=0`
  - `kfm-air-state`: `pass=11 partial=0 fail=0 blocked=0`
  - `cpu-baseline`: `pass=7 partial=0 fail=0 blocked=0`

The front-end overlay conversion pattern is now proven:

- `App.cpp` assembles prepared view data.
- Normal overlay modules render prepared data.
- Resource ownership and runtime decisions stay in `App.cpp`.
- `AppState`, `FighterState`, resource-owning types, and routing behavior do not enter normal overlay modules.

Fight overlays are more runtime-adjacent than menu/select overlays. They can likely use the same prepared-view pattern, but only after a small fight-presentation view seam exists.

Pass 32 update:

- `FightPresentationView.h` now provides display-only prepared view structs for future fight HUD/result module conversion.
- `App.cpp` compile-checks the header, but no fight overlay call sites changed.
- `FightPresentationShared.h` remains transitional; `FightResultOverlay.h` was converted later in Pass 34.
- `App.cpp` still owns all runtime-derived view assembly, hit/damage, round flow, result routing, loading, resources, CPU behavior, controller behavior, and sidecar policy.

Pass 33 update:

- `FightHudOverlay.h` is now a normal declaration header and `FightHudOverlay.cpp` owns render-only HUD drawing.
- `App.cpp` assembles `FightHudView` from existing runtime state, fight settings, labels, messages, and display state, then calls the normal module.
- The module has no `AppState`, `FighterState`, `FightRoundSettings`, `FightPowerbarSettings`, training options, gamepad device, resource ownership, CMD/CNS, hit/damage, or round-flow dependency.
- `App.cpp` changed from `14705` to `14795` file-size-guard lines because explicit view assembly replaced hidden App.cpp-internal header coupling.
- `FightHudOverlay.h` is 10 lines and `FightHudOverlay.cpp` is 197 lines.
- Build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. Manual GUI smoke was not rerun.

Pass 34 update:

- `FightResultOverlay.h` is now a normal declaration header and `FightResultOverlay.cpp` owns render-only round/result/match-result drawing.
- `App.cpp` assembles `FightRoundCalloutView`, `FightRoundResultView`, and `FightMatchResultView` from existing runtime state, result helpers, victory quote text, stage label, and result-menu selection, then calls the normal module.
- The module has no `AppState`, `FighterState`, match settings struct, routing state, resource ownership, CMD/CNS, hit/damage, CPU, loading, audio, controller, or round-flow dependency.
- `App.cpp` changed from `14795` to `14886` file-size-guard lines because explicit result view assembly replaced hidden App.cpp-internal header coupling.
- `FightResultOverlay.h` is 13 lines and `FightResultOverlay.cpp` is 133 lines.
- Build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. Manual GUI smoke and dedicated result-screen smoke were not rerun.

## Readiness Labels

- `READY`: can become a normal `.h/.cpp` module with existing public headers and prepared view data.
- `NEEDS SMALL VIEW SEAM`: needs one small public view or render helper seam, with no runtime/resource ownership change.
- `NEEDS FIGHT PRESENTATION VIEW`: needs prepared fight HUD/result data assembled by `App.cpp` before the overlay can be converted.
- `NEEDS FIGHT HUD CONVERSION`: the prepared view seam exists; the next work is converting HUD drawing to consume `FightHudView`.
- `NEEDS RESULT VIEW ASSEMBLY`: result view structs exist, but App.cpp-local result/callout assembly should wait until the HUD conversion proves the pattern.
- `DEFER`: would require moving or exposing high-risk runtime systems and should not be converted now.

## Conversion Inventory

| Header | Current Lines | Responsibility | Main Dependencies | Minimum View Seam | Conversion Readiness | Recommended Order | Notes |
|---|---:|---|---|---|---|---:|---|
| `FightHudOverlay.h` / `FightHudOverlay.cpp` | 10 / 197 | Fight HUD rendering: life bars, power gauges, timer, round wins, combo counters, bottom status/hit-log lines, input/footer hints | Prepared `FightHudView`, `UiRenderContext`, public UI primitives, and standard library headers. `App.cpp` still assembles runtime-derived values. | No remaining HUD render seam; future refinements can reduce App.cpp view assembly when broader fight state boundaries exist. | `READY` | 0 | Pass 33 converted the HUD to a normal module. Runtime state, mutation, labels, timer, combo data, and gamepad text remain assembled in `App.cpp`. |
| `FightResultOverlay.h` / `FightResultOverlay.cpp` | 13 / 133 | Round start/finish/result overlays and match-complete result menu rendering | Prepared `FightRoundCalloutView`, `FightRoundResultView`, `FightMatchResultView`, `UiRenderContext`, public UI primitives, and standard library headers. `App.cpp` still assembles runtime-derived values. | No remaining result render seam; future match/round state boundaries may shrink App.cpp view assembly later. | `READY` | 0 | Pass 34 converted the result overlay to a normal module. Runtime state, result semantics, victory quote selection, result-menu selection, and routing remain assembled in `App.cpp`. |
| `FightPresentationShared.h` | 60 | Shared fight presentation helpers: single-fight status line text | `singleFightStatusLine` reads match phase, phase ticks, round settings, gamepad state, pending mode, and result-text helpers. | Keep status-line assembly in `App.cpp`; reassess with later match/round state boundaries. | `DEFER` | 1 | Round-pip rendering moved into normal overlay modules as private prepared-view helpers. The remaining status-line helper is still runtime-adjacent and should not be cleaned up in this pass series. |

## Boundaries For Future Conversion

Any future fight overlay conversion must keep `App.cpp` responsible for assembling display data from runtime state.

Do not move or expose as part of fight overlay conversion:

- CMD/CNS runtime
- HitDef, damage, contact, or guard logic
- round-flow decisions
- KO/time-over decisions
- helper, projectile, or explod runtime
- `FighterState`
- runtime loading
- audio scheduling
- resource ownership
- CPU behavior
- controller behavior
- Dragon Mode
- store/crafting
- sidecar policy
- `.dragon/*.json`

## Recommended Next Implementation Pass

Recommendation:

```text
Pass 35: Training Overlay Conversion Audit
```

Reason:

- `FightHudOverlay` and `FightResultOverlay` now prove the prepared fight-presentation view pattern as normal modules.
- The remaining training overlays are more coupled to command lists, input history, hitbox/debug data, and training options than the fight result overlay was.
- A docs-only audit should identify the smallest prepared-view seams before converting `TrainingOptionsOverlay`, `TrainingCommandOverlay`, or `TrainingDebugOverlay`.

Required seam:

The required fight result seam exists and has been exercised:

- `FightRoundCalloutView`
- `FightRoundResultView`
- `FightMatchResultView`
- `FightResultMenuRowView`
- `App.cpp` remains the only place that reads runtime state and calls match/result/status helper logic.

Expected files for Pass 35:

- create `docs/TRAINING_OVERLAY_CONVERSION_AUDIT.md`
- audit `TrainingOptionsOverlay.h`, `TrainingCommandOverlay.h`, and `TrainingDebugOverlay.h`
- choose the next safe training overlay implementation pass or view seam
- keep training runtime behavior, command/CNS runtime, hitbox/debug source data, `FighterState`, hit/damage, loading, audio, resources, CPU behavior, and sidecar policy out of scope

Estimated risk:

- Low as a docs-only audit.
- Medium for any later training overlay conversion using prepared view data.
- High if a training module takes `AppState`, `FighterState`, command runtime, or debug collision data directly.

Expected `App.cpp` line reduction:

- Small to moderate.
- View assembly may offset some extracted drawing lines.

Expected hidden-coupling reduction:

- Maps the next training presentation seams before converting runtime-adjacent overlays.
- Prevents training overlays from taking `AppState` or `FighterState` dependencies as normal modules.

Verification focus:

- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- no build required for docs-only unless local policy requires it
- manual smoke not needed for the audit

Do not move:

- hit/damage/guard logic
- round-flow decisions
- match-completion decisions
- result-menu actions
- `FighterState`
- CMD/CNS runtime
- loading
- audio
- resource ownership
- CPU behavior
