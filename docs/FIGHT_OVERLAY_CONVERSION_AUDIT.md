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
- `FightHudOverlay.h`, `FightResultOverlay.h`, and `FightPresentationShared.h` remain transitional until the next conversion pass.
- `App.cpp` still owns all runtime-derived view assembly, hit/damage, round flow, result routing, loading, resources, CPU behavior, controller behavior, and sidecar policy.

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
| `FightPresentationShared.h` | 60 | Shared fight presentation helpers: single-fight status line text and round-win pip rendering | `singleFightStatusLine` reads match phase, phase ticks, round settings, gamepad state, pending mode, and result-text helpers. `drawRoundWinPips` is render-only after primitive inputs. | `FightPresentationView.h` now supplies prepared round-pip inputs; keep status-line assembly in `App.cpp` until HUD/result conversion. | `NEEDS SMALL VIEW SEAM` | 1 | The view seam exists, but this header is still transitional because the drawing helpers and status-line helper have not been split yet. |
| `FightHudOverlay.h` | 190 | Fight HUD rendering: life bars, power gauges, timer, round wins, combo counters, bottom status/hit-log lines, input/footer hints | `AppState`, `FighterState` life/power, `FightRoundSettings`, `FightDisplayState`, `FightMessageState`, match timer/round wins, selected/opponent labels, gamepad labels, and shared status-line helpers. | Use the new `FightHudView`, `FighterHudView`, `FightPowerGaugeView`, `FightComboCounterView`, and `FightRoundPipsView`; assemble all values in `App.cpp`. | `NEEDS FIGHT HUD CONVERSION` | 2 | This is now the recommended next implementation pass. `App.cpp` must still compute strings, read runtime state, and own combo mutation/timer logic. |
| `FightResultOverlay.h` | 119 | Round start/finish/result overlays and match-complete result menu rendering | `AppState`, match phase/ticks, round settings, round wins, result text helpers, fighters, victory quote helper, selected stage name, match winner/method/score helpers, and result-menu selection. | Use the new `FightRoundCalloutView`, `FightRoundResultView`, `FightMatchResultView`, and `FightResultMenuRowView`; assemble all values in `App.cpp`. | `NEEDS RESULT VIEW ASSEMBLY` | 3 | Convert after the HUD conversion or after the shared render helper split proves stable. Keep round-flow decisions, match-completion logic, rematch/routing behavior, and result-menu actions in `App.cpp`. |

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
Pass 33: Convert FightHudOverlay using prepared FightHudView
```

Reason:

- `FightPresentationView.h` now provides the display-only view types needed by the HUD path.
- `FightHudOverlay.h` is less result/routing-adjacent than `FightResultOverlay.h`, so it is the safer first fight overlay conversion.
- The conversion should keep `App.cpp` as the only place that reads `AppState`, `FighterState`, fight settings, match timer, training options, gamepad state, and selection labels.

Required seam:

The required view seam now exists:

- `FightHudView`
- `FighterHudView`
- `FightPowerGaugeView`
- `FightComboCounterView`
- `FightRoundPipsView`
- `App.cpp` remains the only place that reads runtime state and calls match/result/status helper logic.

Expected files for Pass 33:

- convert `engine/src/FightHudOverlay.h` to a normal declaration header
- create `engine/src/FightHudOverlay.cpp`
- update `engine/src/App.cpp` to assemble `FightHudView` and call the normal module
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

- Removes `FightHudOverlay` from the App.cpp-internal include-order group.
- Proves fight overlays can follow the prepared-view module pattern without taking `AppState` or `FighterState` dependencies.

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
