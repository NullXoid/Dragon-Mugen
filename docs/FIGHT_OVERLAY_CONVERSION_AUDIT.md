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

## Readiness Labels

- `READY`: can become a normal `.h/.cpp` module with existing public headers and prepared view data.
- `NEEDS SMALL VIEW SEAM`: needs one small public view or render helper seam, with no runtime/resource ownership change.
- `NEEDS FIGHT PRESENTATION VIEW`: needs prepared fight HUD/result data assembled by `App.cpp` before the overlay can be converted.
- `DEFER`: would require moving or exposing high-risk runtime systems and should not be converted now.

## Conversion Inventory

| Header | Current Lines | Responsibility | Main Dependencies | Minimum View Seam | Conversion Readiness | Recommended Order | Notes |
|---|---:|---|---|---|---|---:|---|
| `FightPresentationShared.h` | 60 | Shared fight presentation helpers: single-fight status line text and round-win pip rendering | `singleFightStatusLine` reads match phase, phase ticks, round settings, gamepad state, pending mode, and result-text helpers. `drawRoundWinPips` is render-only after primitive inputs. | Small public fight-presentation seam with prepared status strings and a render-only round-pip view/helper. Keep status-line assembly in `App.cpp`. | `NEEDS SMALL VIEW SEAM` | 1 | This is the smallest shared blocker. Split presentation data from runtime-derived text before converting HUD/result overlays. |
| `FightHudOverlay.h` | 190 | Fight HUD rendering: life bars, power gauges, timer, round wins, combo counters, bottom status/hit-log lines, input/footer hints | `AppState`, `FighterState` life/power, `FightRoundSettings`, `FightDisplayState`, `FightMessageState`, match timer/round wins, selected/opponent labels, gamepad labels, and shared status-line helpers. | `FightHudView` with prepared fighter labels, life/power values, powerbar config, timer/round data, combo display data, hit-log/status line text, and footer text. | `NEEDS FIGHT PRESENTATION VIEW` | 2 | Likely first fight overlay conversion after the seam. `App.cpp` must still compute strings, read runtime state, and own combo mutation/timer logic. |
| `FightResultOverlay.h` | 119 | Round start/finish/result overlays and match-complete result menu rendering | `AppState`, match phase/ticks, round settings, round wins, result text helpers, fighters, victory quote helper, selected stage name, match winner/method/score helpers, and result-menu selection. | `FightResultView` plus prepared round-callout/result views with ready text, colors, score/stage/winner strings, quote text, selected result option, and option labels. | `NEEDS FIGHT PRESENTATION VIEW` | 3 | Convert after HUD or after the shared seam proves stable. Keep round-flow decisions, match-completion logic, rematch/routing behavior, and result-menu actions in `App.cpp`. |

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
Pass 32: Create FightPresentationView seam
```

Reason:

- `FightHudOverlay.h` and `FightResultOverlay.h` both still depend on `AppState` and App.cpp-local helpers for runtime-derived labels and match/round state.
- `FightPresentationShared.h` contains one render-only helper shape and one runtime-derived status-line helper. Splitting prepared view data from runtime text assembly is the smallest safe step.
- Converting `FightHudOverlay` directly would require either passing `AppState`/`FighterState` into a normal module or duplicating runtime interpretation in the overlay. Both should be avoided.

Required seam:

- A small public header for prepared fight presentation data, with no `AppState`, `FighterState`, runtime, loading, or resource ownership types.
- Suggested first scope:
  - prepared fighter HUD values
  - prepared timer/round-win display values
  - prepared status/hit-log/footer strings
  - prepared combo counter display values
  - prepared round-pip rendering inputs
- `App.cpp` remains the only place that reads runtime state and calls match/result/status helper logic.

Expected files for Pass 32:

- create `engine/src/FightPresentationView.h`
- optionally create a tiny render-only shared helper module if inspection shows `drawRoundWinPips` can be made AppState-free without pulling runtime state
- update `docs/FIGHT_OVERLAY_CONVERSION_AUDIT.md`
- update normal progress docs after evidence

Estimated risk:

- Low to medium.
- Low if the pass creates prepared view structs only.
- Medium if it tries to move HUD/result drawing before all runtime-derived data is prepared by `App.cpp`.

Expected `App.cpp` line reduction:

- Small or none for the seam pass.
- The payoff is hidden-coupling reduction and a safer path to converting `FightHudOverlay` next.

Expected hidden-coupling reduction:

- Establishes a public fight-presentation API that normal overlay modules can consume.
- Prevents `FightHudOverlay` and `FightResultOverlay` from taking dependencies on `AppState`, `FighterState`, round-flow helpers, or hit/damage internals.

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
