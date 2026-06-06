# Training Overlay Conversion Audit

This docs-only audit checks the remaining training overlay headers before converting them into normal render-only modules.

Baseline:

- HEAD: `8911e63 Convert fight result overlay to normal module`
- `engine/src/App.cpp`: `14886` file-size-guard lines
- Current verifier baseline:
  - `kfm-baseline`: `pass=11 partial=1 fail=0 blocked=0`
  - `evilken-smoke`: `pass=7 partial=0 fail=0 blocked=0`
  - `kfm-air-state`: `pass=11 partial=0 fail=0 blocked=0`
  - `cpu-baseline`: `pass=7 partial=0 fail=0 blocked=0`

The front-end and fight overlay conversion pattern is now proven:

- `App.cpp` assembles prepared view data.
- Normal overlay modules render prepared data.
- Runtime decisions and resource ownership stay in `App.cpp` or existing runtime modules.
- `AppState`, `FighterState`, resource-owning types, and routing behavior do not enter normal overlay modules.

Training overlays are more runtime-adjacent than menu overlays because they touch training options, command display, input history, and debug hitbox data. They should be converted only through prepared display views.

Pass 36 update:

- `TrainingOptionsOverlay.h` is now a normal declaration header and `TrainingOptionsOverlay.cpp` owns render-only F2 Training Options and command move-list drawing.
- `App.cpp` assembles `TrainingOptionsMenuView` and `TrainingMoveListView` from training options and command-entry display helpers, then calls the normal module.
- `TrainingOptionsOverlay` no longer depends on `AppState`, `FighterState`, `CommandStateEntry`, include order, command runtime, or training runtime behavior.
- `App.cpp` changed from `14886` to `14975` file-size-guard lines because explicit view assembly replaced hidden internal-header coupling.
- `TrainingOptionsOverlay.h` is 48 lines and `TrainingOptionsOverlay.cpp` is 138 lines.
- Build passed. Manual F2/move-list GUI smoke was not rerun in this pass.

Pass 37 update:

- `TrainingCommandView.h` now defines the display-only command/input HUD seam for the next training overlay conversion.
- The seam contains prepared string/flag view structs only: `TrainingCommandRowView`, `TrainingInputHudView`, and `TrainingCommandHudView`.
- `App.cpp` compile-checks the header but does not assemble the view yet; `TrainingCommandOverlay.h` remains transitional and all call sites are unchanged.
- `App.cpp` changed from `14975` to `14976` file-size-guard lines because this seam-only pass adds one compile-check include.
- `TrainingCommandView.h` is 27 lines.
- Build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed.
- Command matching, input history reading, `FighterState`, `CommandStateEntry`, CMD/CNS data, training runtime behavior, hit/damage, input handling, loading, resources, sidecar policy, and `.dragon/*.json` stayed unchanged.

## Readiness Labels

- `NEEDS SMALL VIEW SEAM`: needs prepared view structs and App.cpp-local view assembly, with no runtime/resource ownership change.
- `NEEDS COMMAND HUD VIEW`: needs a prepared command/input HUD snapshot because current drawing reads command and fighter input runtime data.
- `NEEDS FIGHTER DEBUG VIEW`: needs precomputed screen-space debug data because current drawing reads fighter, stage, animation, camera, and collision-box data.
- `DEFER`: should not be converted until a safer seam exists because direct conversion would expose runtime-heavy types.

## Header Audit

| Header | Lines | Responsibility | Main Dependencies | Readiness | Main Blocker | Minimum View Seam Needed | Recommended Priority | Notes |
|---|---:|---|---|---|---|---|---|---|
| `TrainingOptionsOverlay.h` / `TrainingOptionsOverlay.cpp` | 48 / 138 | F2 Training Options menu and command move-list page rendering | Prepared `TrainingOptionsMenuView`, prepared `TrainingMoveListView`, `UiRenderContext`, public UI primitives | `READY` | None for current prepared-view API; App.cpp still owns training option state, command-entry inspection, move-list filtering, and view assembly | No remaining seam for F2 options/move-list rendering; future command HUD/debug seams remain separate | 0 | Pass 36 converted this overlay to a normal module. No `AppState`, `FighterState`, `CommandStateEntry`, resource ownership, command runtime, or gameplay behavior enters the module. |
| `TrainingCommandOverlay.h` | 80 | Training command/input HUD rendering | `AppState`, first `FighterState`, opponent `FighterState`, input history, command definitions, active command matching, command display helpers | `NEEDS COMMAND HUD VIEW` | Current drawing still computes command history and active command state from live fighter/runtime data | `TrainingCommandView.h` now provides `TrainingCommandHudView`; Pass 38 should add App.cpp-local view assembly and convert the overlay | 1 | Public view seam exists. Next pass converts the overlay without moving command matching, input history ownership, or CMD/CNS behavior. |
| `TrainingDebugOverlay.h` | 178 | F1 debug overlay, floor line, origins, collision boxes, fighter readout | `AppState`, `FighterState`, `StageSlot`, `AnimationClip`, `AnimationFrame`, `CollisionBox`, camera/stage offsets, debug clipboard | `NEEDS FIGHTER DEBUG VIEW` | Current drawing projects collision boxes and origins from live fighter/stage/animation data | `TrainingDebugView` with precomputed screen-space floor/origins/boxes/readout lines | 3 | Defer until a debug view seam exists. This overlay is render-only but strongly coupled to runtime state shape. |

## Needed View Seams

| View Seam | Needed By | Fields / Data | Assembled By | Risk | Notes |
|---|---|---|---|---|---|
| `TrainingOptionsMenuView` | `TrainingOptionsOverlay` | selected option, option rows, labels, statuses, footer text | `App.cpp` using `TrainingOptionsBehavior` and `TrainingState` | Low | No runtime mutation or fighter state is needed. The overlay should only draw prepared rows. |
| `TrainingMoveListView` | `TrainingOptionsOverlay` | selected character label, category label, visible move rows, selected detail text, perform/input text, page text | `App.cpp` using existing command display helpers and command entry data | Medium | Keep `CommandStateEntry` and command/CNS data in `App.cpp`; pass only prepared strings and selected/highlight flags. |
| `TrainingCommandHudView` | `TrainingCommandOverlay` | current input token, recent input tokens, command rows, active command row, active command label, panel visibility flags | `App.cpp` after existing input-history and active-command matching | Medium | `TrainingCommandView.h` defines this seam. Do not move command matching, input history ownership, or CMD/CNS behavior into the overlay module. |
| `TrainingDebugView` | `TrainingDebugOverlay` | floor line, fighter origins, screen-space collision boxes, debug readout lines, show/hide flags | `App.cpp` after projecting from fighter/stage/animation/camera state | High | Precompute screen-space data before drawing. Do not pass `FighterState`, `StageSlot`, `AnimationFrame`, or `CollisionBox` into a normal module. |

## Boundaries For Future Conversion

Any future training overlay conversion must keep `App.cpp` responsible for assembling display data from runtime state.

Do not move or expose as part of training overlay conversion:

- CMD/CNS runtime
- command matching or active-command decisions
- `FighterState`
- HitDef, damage, contact, or guard logic
- round-flow decisions
- dummy behavior or training runtime side effects
- input handling
- runtime loading
- audio scheduling
- resource ownership
- CPU behavior
- controller behavior
- Dragon Mode
- store/crafting
- sidecar policy
- `.dragon/*.json`

Future normal training overlay modules must not take:

- `AppState`
- `FighterState`
- `StageSlot`
- `CommandStateEntry`
- `AnimationFrame`
- `CollisionBox`
- resource-owning types

## Recommended Next Implementation Pass

Recommendation:

```text
Pass 38: Convert TrainingCommandOverlay to normal module
```

Reason:

- `TrainingOptionsOverlay` is now converted, proving the training prepared-view pattern.
- `TrainingCommandView.h` now provides the command/input HUD view seam.
- `TrainingCommandOverlay` is the next practical conversion target but still reads fighter input history, command definitions, active command matching, and App.cpp-local command display helpers.
- Pass 38 should add App.cpp-local `TrainingCommandHudView` assembly, then move render-only drawing into a normal module.
- `TrainingDebugOverlay` should wait until a screen-space fighter debug view seam exists.

Expected files for Pass 38:

- `engine/src/App.cpp`
- `engine/src/TrainingCommandOverlay.h`
- `engine/src/TrainingCommandOverlay.cpp`
- `CMakeLists.txt`
- documentation updates after build/verifier evidence

Expected `App.cpp` line reduction:

- Likely small or neutral. Explicit command/input HUD view assembly may offset extracted drawing lines.

Verification focus:

- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- manual Training F2 options and move-list smoke if practical

Do not convert:

- command matching, input history ownership, `FighterState`, and CMD/CNS behavior during `TrainingCommandOverlay` conversion.
- `TrainingDebugOverlay` until a fighter debug/screen-space hitbox view seam exists.
