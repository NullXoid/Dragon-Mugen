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

## Readiness Labels

- `NEEDS SMALL VIEW SEAM`: needs prepared view structs and App.cpp-local view assembly, with no runtime/resource ownership change.
- `NEEDS COMMAND HUD VIEW`: needs a prepared command/input HUD snapshot because current drawing reads command and fighter input runtime data.
- `NEEDS FIGHTER DEBUG VIEW`: needs precomputed screen-space debug data because current drawing reads fighter, stage, animation, camera, and collision-box data.
- `DEFER`: should not be converted until a safer seam exists because direct conversion would expose runtime-heavy types.

## Header Audit

| Header | Lines | Responsibility | Main Dependencies | Readiness | Main Blocker | Minimum View Seam Needed | Recommended Priority | Notes |
|---|---:|---|---|---|---|---|---|---|
| `TrainingOptionsOverlay.h` | 155 | F2 Training Options menu and command move-list page rendering | `AppState`, `TrainingOptions`, `displayableMoveListEntries`, command entry display helpers, selected character label, UI primitives | `NEEDS SMALL VIEW SEAM` | Move-list rendering currently reads command entry metadata and App.cpp-local command display helpers directly | `TrainingOptionsMenuView` plus `TrainingMoveListView`, assembled by `App.cpp` from training options and command display data | 1 | Closest conversion candidate. Option rows are low risk; move-list rows need prepared command labels/input/status strings so the module does not take `CommandStateEntry`. |
| `TrainingCommandOverlay.h` | 80 | Training command/input HUD rendering | `AppState`, first `FighterState`, opponent `FighterState`, input history, command definitions, active command matching, command display helpers | `NEEDS COMMAND HUD VIEW` | Current drawing computes command history and active command state from live fighter/runtime data | `TrainingCommandHudView`, assembled by `App.cpp` after command/input matching remains in place | 2 | Convert after the options overlay proves the training view pattern. Do not move command matching or CMD/CNS behavior. |
| `TrainingDebugOverlay.h` | 178 | F1 debug overlay, floor line, origins, collision boxes, fighter readout | `AppState`, `FighterState`, `StageSlot`, `AnimationClip`, `AnimationFrame`, `CollisionBox`, camera/stage offsets, debug clipboard | `NEEDS FIGHTER DEBUG VIEW` | Current drawing projects collision boxes and origins from live fighter/stage/animation data | `TrainingDebugView` with precomputed screen-space floor/origins/boxes/readout lines | 3 | Defer until a debug view seam exists. This overlay is render-only but strongly coupled to runtime state shape. |

## Needed View Seams

| View Seam | Needed By | Fields / Data | Assembled By | Risk | Notes |
|---|---|---|---|---|---|
| `TrainingOptionsMenuView` | `TrainingOptionsOverlay` | selected option, option rows, labels, statuses, footer text | `App.cpp` using `TrainingOptionsBehavior` and `TrainingState` | Low | No runtime mutation or fighter state is needed. The overlay should only draw prepared rows. |
| `TrainingMoveListView` | `TrainingOptionsOverlay` | selected character label, category label, visible move rows, selected detail text, perform/input text, page text | `App.cpp` using existing command display helpers and command entry data | Medium | Keep `CommandStateEntry` and command/CNS data in `App.cpp`; pass only prepared strings and selected/highlight flags. |
| `TrainingCommandHudView` | `TrainingCommandOverlay` | current input token, recent input tokens, command rows, active command row, panel visibility flags | `App.cpp` after existing input-history and active-command matching | Medium | Do not move command matching, input history ownership, or CMD/CNS behavior into the overlay module. |
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
Pass 36: Convert TrainingOptionsOverlay to normal module using prepared TrainingOptionsMenuView and TrainingMoveListView
```

Reason:

- `TrainingOptionsOverlay` is the least runtime-coupled remaining training overlay.
- Training option labels/status text already live in `TrainingOptionsBehavior`.
- The command move-list page can be converted safely if `App.cpp` prepares move rows and detail strings instead of passing command entries into the module.
- `TrainingCommandOverlay` should wait until a command/input HUD view seam exists.
- `TrainingDebugOverlay` should wait until a screen-space fighter debug view seam exists.

Expected files for Pass 36:

- `engine/src/TrainingOptionsOverlay.h`
- `engine/src/TrainingOptionsOverlay.cpp`
- `engine/src/App.cpp`
- `CMakeLists.txt`
- documentation updates after build/verifier evidence

Expected `App.cpp` line reduction:

- Small to moderate.
- Explicit view assembly may offset some extracted drawing lines, as in prior overlay conversions.

Verification focus:

- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- manual Training F2 options and move-list smoke if practical

Do not convert:

- `TrainingCommandOverlay` until a command/input HUD view seam exists.
- `TrainingDebugOverlay` until a fighter debug/screen-space hitbox view seam exists.
