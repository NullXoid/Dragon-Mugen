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

Pass 38 update:

- `TrainingCommandOverlay.h` is now a normal declaration header and `TrainingCommandOverlay.cpp` owns render-only command/input HUD drawing.
- `App.cpp` assembles `TrainingCommandHudView` from existing input history, command display helpers, displayable move-list entries, and active command matching, then calls the normal module.
- `TrainingCommandOverlay` no longer depends on `AppState`, `FighterState`, `CommandStateEntry`, input history objects, command definitions, include order, command runtime, or training runtime behavior.
- `App.cpp` changed from `14976` to `15035` file-size-guard lines because explicit command/input HUD view assembly replaced hidden internal-header coupling.
- `TrainingCommandOverlay.h` is 10 lines and `TrainingCommandOverlay.cpp` is 72 lines.
- Build passed. Manual command/input HUD GUI smoke was not rerun in this pass.
- Command matching, command-list filtering semantics, input history ownership, `FighterState`, `CommandStateEntry`, CMD/CNS data, training runtime behavior, hit/damage, input handling, loading, resources, sidecar policy, and `.dragon/*.json` stayed unchanged.

Pass 39 update:

- `TrainingDebugView.h` now defines the display-only screen-space debug view seam for the remaining training overlay conversion.
- The seam contains prepared debug presentation structs only: `TrainingDebugColorView`, `TrainingDebugLineView`, `TrainingDebugRectView`, `TrainingDebugReadoutView`, and `TrainingDebugView`.
- `App.cpp` compile-checks the header but does not assemble the view yet; `TrainingDebugOverlay.h` remains transitional and all call sites are unchanged.
- `App.cpp` changed from `15035` to `15036` file-size-guard lines because this seam-only pass adds one compile-check include.
- `TrainingDebugView.h` is 44 lines.
- Build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed.
- `AppState`, `FighterState`, `StageSlot`, `AnimationFrame`, `CollisionBox`, camera/stage runtime objects, debug clipboard behavior, CMD/CNS data, hit/damage, input handling, loading, resources, sidecar policy, and `.dragon/*.json` stayed unchanged.

Pass 40 update:

- `TrainingDebugOverlay.h` is now a normal declaration header and `TrainingDebugOverlay.cpp` owns render-only F1 debug overlay drawing from prepared `TrainingDebugView` data.
- `TrainingDebugViewAssembly.h` is an App.cpp-internal view assembly bridge. It projects fighter/stage/animation/camera/debug clipboard data into screen-space lines, rectangles, and readout strings, then calls the normal overlay module.
- `TrainingDebugOverlay` no longer depends on `AppState`, `FighterState`, `StageSlot`, `AnimationFrame`, `CollisionBox`, debug clipboard, include order, CMD/CNS data, or training runtime behavior.
- `App.cpp` changed from `15036` to `15037` file-size-guard lines because the conversion keeps projection assembly in a marked internal header instead of inlining it into `App.cpp`.
- `TrainingDebugOverlay.h` is 10 lines, `TrainingDebugOverlay.cpp` is 72 lines, and `TrainingDebugViewAssembly.h` is 215 lines.
- Build passed. Manual F1 debug GUI smoke was not rerun in this pass.
- Projection semantics, command matching, input history ownership, `FighterState`, `StageSlot`, animation frame lookup, `CollisionBox`, CMD/CNS data, training runtime behavior, hit/damage, input handling, loading, resources, sidecar policy, and `.dragon/*.json` stayed unchanged.

## Readiness Labels

- `NEEDS SMALL VIEW SEAM`: needs prepared view structs and App.cpp-local view assembly, with no runtime/resource ownership change.
- `NEEDS COMMAND HUD VIEW`: needs a prepared command/input HUD snapshot because current drawing reads command and fighter input runtime data.
- `NEEDS FIGHTER DEBUG VIEW`: needs precomputed screen-space debug data because current drawing reads fighter, stage, animation, camera, and collision-box data.
- `DEFER`: should not be converted until a safer seam exists because direct conversion would expose runtime-heavy types.

## Header Audit

| Header | Lines | Responsibility | Main Dependencies | Readiness | Main Blocker | Minimum View Seam Needed | Recommended Priority | Notes |
|---|---:|---|---|---|---|---|---|---|
| `TrainingOptionsOverlay.h` / `TrainingOptionsOverlay.cpp` | 48 / 138 | F2 Training Options menu and command move-list page rendering | Prepared `TrainingOptionsMenuView`, prepared `TrainingMoveListView`, `UiRenderContext`, public UI primitives | `READY` | None for current prepared-view API; App.cpp still owns training option state, command-entry inspection, move-list filtering, and view assembly | No remaining seam for F2 options/move-list rendering; future command HUD/debug seams remain separate | 0 | Pass 36 converted this overlay to a normal module. No `AppState`, `FighterState`, `CommandStateEntry`, resource ownership, command runtime, or gameplay behavior enters the module. |
| `TrainingCommandOverlay.h` / `TrainingCommandOverlay.cpp` | 10 / 72 | Training command/input HUD rendering | Prepared `TrainingCommandHudView`, prepared `TrainingInputHudView`, prepared command rows, `UiRenderContext`, public UI primitives | `READY` | None for current prepared-view API; App.cpp still owns command matching, input history reading, command-list filtering, and view assembly | No remaining seam for command/input HUD rendering; debug view seam remains separate | 0 | Pass 38 converted this overlay to a normal module. No `AppState`, `FighterState`, `CommandStateEntry`, input history object, command definition, command runtime, or gameplay behavior enters the module. |
| `TrainingDebugOverlay.h` / `TrainingDebugOverlay.cpp` | 10 / 72 | F1 debug overlay rendering | Prepared `TrainingDebugView`, prepared lines/boxes/readout strings, `UiRenderContext`, public UI primitives | `READY` | None for current prepared-view API; `TrainingDebugViewAssembly.h` remains App.cpp-internal for runtime projection and view assembly | No remaining seam for F1 debug overlay rendering; later work can reassess the internal assembly bridge when `FighterState`/debug projection boundaries exist | 0 | Pass 40 converted this overlay to a normal module. No `AppState`, `FighterState`, `StageSlot`, `AnimationFrame`, `CollisionBox`, debug clipboard behavior, command runtime, or gameplay behavior enters the module. |

## Needed View Seams

| View Seam | Needed By | Fields / Data | Assembled By | Risk | Notes |
|---|---|---|---|---|---|
| `TrainingOptionsMenuView` | `TrainingOptionsOverlay` | selected option, option rows, labels, statuses, footer text | `App.cpp` using `TrainingOptionsBehavior` and `TrainingState` | Low | No runtime mutation or fighter state is needed. The overlay should only draw prepared rows. |
| `TrainingMoveListView` | `TrainingOptionsOverlay` | selected character label, category label, visible move rows, selected detail text, perform/input text, page text | `App.cpp` using existing command display helpers and command entry data | Medium | Keep `CommandStateEntry` and command/CNS data in `App.cpp`; pass only prepared strings and selected/highlight flags. |
| `TrainingCommandHudView` | `TrainingCommandOverlay` | current input token, recent input tokens, command rows, active command row, active command label, panel visibility flags | `App.cpp` after existing input-history and active-command matching | Medium | Pass 38 converted this overlay. Command matching, input history ownership, and CMD/CNS behavior stay in App.cpp. |
| `TrainingDebugView` | `TrainingDebugOverlay` | screen-space lines, screen-space collision boxes, debug readout lines, visibility flags, display colors | `TrainingDebugViewAssembly.h` included from `App.cpp` after projecting from fighter/stage/animation/camera state | High | Pass 40 converted the overlay. The assembly bridge stays App.cpp-internal and must not be included by normal modules. |

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
Pass 42: Retire FightPresentationShared transitional helper
```

Reason:

- The training overlay cluster is now converted to normal render modules.
- Pass 41 completed the remaining transitional helper audit in `docs/TRANSITIONAL_HELPER_BOUNDARY_AUDIT.md`.
- The audit found `FightPresentationShared.h` is the smallest safe cleanup because the only live helper is AppState-coupled status-line assembly and the old round-pip renderer is unused.
- Larger helpers such as `UiRenderHelpers.h`, `FrontendFlow.h`, `TrainingDebugViewAssembly.h`, and `AppVerificationBridge.h` still need presentation/action/runtime seams before conversion.

Expected files for Pass 42:

- `engine/src/App.cpp`
- `engine/src/FightPresentationShared.h`
- docs updated after evidence

Expected `App.cpp` line reduction:

- None expected; the value is retiring a transitional helper and deleting unused helper code.

Verification focus:

- `dev_check`
- file-size guard known `App.cpp` hard debt
- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- manual smoke not required for a docs-only audit

Do not convert:

- `TrainingDebugViewAssembly.h`, `FrontendFlow.h`, `UiRenderHelpers.h`, or `AppVerificationBridge.h` until their required seams are defined.
- command/CNS runtime, hit/damage, input behavior, loading, resources, or sidecar policy.
