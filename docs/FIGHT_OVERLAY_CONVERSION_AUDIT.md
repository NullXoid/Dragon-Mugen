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
- `FightResultOverlay.h` and `FightPresentationShared.h` remain transitional until their conversion passes.
- `App.cpp` still owns all runtime-derived view assembly, hit/damage, round flow, result routing, loading, resources, CPU behavior, controller behavior, and sidecar policy.

Pass 33 update:

- `FightHudOverlay.h` is now a normal declaration header and `FightHudOverlay.cpp` owns render-only HUD drawing.
- `App.cpp` assembles `FightHudView` from existing runtime state, fight settings, labels, messages, and display state, then calls the normal module.
- The module has no `AppState`, `FighterState`, `FightRoundSettings`, `FightPowerbarSettings`, training options, gamepad device, resource ownership, CMD/CNS, hit/damage, or round-flow dependency.
- `App.cpp` changed from `14705` to `14795` file-size-guard lines because explicit view assembly replaced hidden App.cpp-internal header coupling.
- `FightHudOverlay.h` is 10 lines and `FightHudOverlay.cpp` is 197 lines.
- Build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. Manual GUI smoke was not rerun.

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
| `FightPresentationShared.h` | 60 | Shared fight presentation helpers: single-fight status line text and round-win pip rendering | `singleFightStatusLine` reads match phase, phase ticks, round settings, gamepad state, pending mode, and result-text helpers. | Keep status-line assembly in `App.cpp`; result conversion may retire or shrink the remaining shared helper header. | `NEEDS RESULT VIEW ASSEMBLY` | 1 | Round-pip rendering moved into the normal HUD module as a private prepared-view helper. The status-line helper remains transitional until result conversion. |
| `FightResultOverlay.h` | 119 | Round start/finish/result overlays and match-complete result menu rendering | `AppState`, match phase/ticks, round settings, round wins, result text helpers, fighters, victory quote helper, selected stage name, match winner/method/score helpers, and result-menu selection. | Use the new `FightRoundCalloutView`, `FightRoundResultView`, `FightMatchResultView`, and `FightResultMenuRowView`; assemble all values in `App.cpp`. | `NEEDS RESULT VIEW ASSEMBLY` | 2 | Convert after the HUD conversion. Keep round-flow decisions, match-completion logic, rematch/routing behavior, and result-menu actions in `App.cpp`. |

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
Pass 34: Convert FightResultOverlay using prepared result views
```

Reason:

- `FightHudOverlay` now proves the prepared fight-presentation view pattern as a normal module.
- `FightResultOverlay.h` is the remaining fight presentation overlay that is still App.cpp-internal.
- The conversion should keep `App.cpp` as the only place that reads match phase, round settings, fighters, victory quotes, selected stage data, match result helpers, result-menu selection, and routing state.

Required seam:

The required result view seam now exists:

- `FightRoundCalloutView`
- `FightRoundResultView`
- `FightMatchResultView`
- `FightResultMenuRowView`
- `App.cpp` remains the only place that reads runtime state and calls match/result/status helper logic.

Expected files for Pass 34:

- convert `engine/src/FightResultOverlay.h` to a normal declaration header
- create `engine/src/FightResultOverlay.cpp`
- update `engine/src/App.cpp` to assemble round callout/result/match result views and call the normal module
- update CMake for the new `.cpp`
- update normal progress docs after evidence

Estimated risk:

- Medium.
- Low if the module receives only prepared view data.
- High if it takes `AppState`, `FighterState`, fight settings, or runtime helpers directly.

Expected `App.cpp` line reduction:

- Small to moderate.
- View assembly may offset some extracted drawing lines.

Expected hidden-coupling reduction:

- Removes `FightResultOverlay` from the App.cpp-internal include-order group.
- Completes the main fight-presentation overlay conversion pair without taking `AppState` or `FighterState` dependencies.

Verification focus:

- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- manual fight HUD smoke if practical
- manual match result smoke if practical

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
