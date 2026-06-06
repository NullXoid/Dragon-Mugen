# Overlay Module Conversion Audit

This docs-only audit identifies which transitional App.cpp-internal overlay headers can become normal modules next.

Baseline:

- HEAD: `e379513 Extract fight display state boundary from App.cpp`
- `engine/src/App.cpp`: `14490` file-size-guard lines
- Current verifier baseline:
  - `kfm-baseline`: `pass=11 partial=1 fail=0 blocked=0`
  - `evilken-smoke`: `pass=7 partial=0 fail=0 blocked=0`
  - `kfm-air-state`: `pass=11 partial=0 fail=0 blocked=0`
  - `cpu-baseline`: `pass=7 partial=0 fail=0 blocked=0`

This pass does not change source code. It only maps module-conversion blockers and the smallest seams needed before Phase D implementation.

Pass 23 update:

- `UiRenderContext.h` now provides a resource-free render context for primitive UI drawing.
- `UiRenderPrimitives.h/.cpp` is now a normal module for AppState-free color, rectangle, debug text, panel, fit-text, scaled-text, and scoped-scale helpers.
- `UiRenderHelpers.h`, sprite helpers, title/select backgrounds, `TextureSprite`, `SystemScreenAssets`, and overlay headers remain transitional.
- `PauseMenuOverlay.h` remains the recommended first overlay conversion target.

Pass 24 update:

- `PauseMenuOverlay.h` is now a normal declaration header with `PauseMenuView`.
- `PauseMenuOverlay.cpp` now owns render-only pause menu labels and drawing.
- `App.cpp` builds `PauseMenuView` from `FrontendState` and still owns pause state, option mutation, route actions, fight runtime, input handling, and screen transitions.
- `PauseMenuOverlay` no longer depends on App.cpp-local `AppState`, `FighterState`, or include order.

Pass 25 update:

- `MainMenuOverlay.h` is now a normal declaration header with `MainMenuView`.
- `MainMenuOverlay.cpp` now owns AppState-free main-menu foreground labels, highlight, description, footer, and title text.
- `App.cpp` still owns `drawTitleBackground`, title logo sprite drawing, `SystemScreenAssets`, sprite helpers, and `SDL_RenderPresent`.
- `MainMenuOverlay` no longer depends on App.cpp-local `AppState`, `FighterState`, or include order.

Pass 26 update:

- `StageSelectOverlay.h` is now a normal declaration header with `StageSelectView` and `StageSelectRowView`.
- `StageSelectOverlay.cpp` now owns AppState-free Stage Select foreground rendering using prepared display data, `UiRenderContext`, and public UI primitives.
- `App.cpp` still owns selected-stage state, stage row assembly, selected-stage metadata lookup, fighter/opponent label preparation, routing, Enter/Esc behavior, loading, and `SDL_RenderPresent`.
- `StageSelectOverlay` no longer depends on App.cpp-local `AppState`, `FighterState`, `SelectionState`, `StageSlot`, `CharacterSlot`, or include order.

Pass 27 update:

- `OptionsMenuOverlay.h` is now a normal declaration header with `OptionsMenuView` and `OptionsMenuRowView`.
- `OptionsMenuOverlay.cpp` now owns AppState-free Options foreground rendering using prepared row/value data, `UiRenderContext`, and public UI primitives.
- `App.cpp` still owns settings state, row/value assembly, gamepad status text, setting mutation, routing, input, persistence, title-background drawing, and `SDL_RenderPresent`.
- `OptionsMenuOverlay` no longer depends on App.cpp-local `AppState`, `FighterState`, `MainSettings`, gamepad device objects, or include order.

Pass 28 update:

- `VsScreenOverlay.h` is now a normal declaration header with `VsScreenView`.
- `VsScreenOverlay.cpp` now owns AppState-free VS/loading presentation using prepared labels, a non-owning portrait texture view, `UiRenderContext`, and public UI primitives.
- `App.cpp` still owns portrait/resource selection, selected fighter/stage metadata assembly, load-status mapping, fight preparation, loading, routing, and `SDL_RenderPresent`.
- `VsScreenOverlay` no longer depends on App.cpp-local `AppState`, `FighterState`, `SelectionState`, `TextureSprite`, `SystemScreenAssets`, or include order.

Pass 29 update:

- `UiSpriteView.h/.cpp` now provides a public non-owning sprite view and top-left sprite drawing helper for overlay modules.
- `VsScreenOverlay` now uses `UiSpriteView` instead of a private portrait view.
- `TextureSprite`, texture creation/destruction, sprite caches, sprite loading, and resource ownership remain in `App.cpp`.
- `CharacterSelectOverlay.h` remains transitional, but the sprite-view seam needed for its conversion now exists.

## Readiness Labels

- `READY`: can become a normal `.h/.cpp` module with existing public headers and no App.cpp-local dependencies.
- `NEEDS SMALL SEAM`: needs one small public type, view model, helper, or context, with no runtime/resource ownership change.
- `NEEDS PUBLIC RENDER SEAM`: depends on App.cpp-local render primitives, `TextureSprite`, `SystemScreenAssets`, or sprite helpers.
- `DEFER`: depends on `FighterState`, AppState internals, hit/damage, round flow, runtime loading, command/CNS runtime, or other high-risk systems.

## Conversion Inventory

| Header | Lines | Responsibility | Readiness | Main Blocker | Minimum Seam Needed | Recommended Priority | Notes |
|---|---:|---|---|---|---|---|---|
| `PauseMenuOverlay.h` / `PauseMenuOverlay.cpp` | 16 / 63 | Single-fight pause menu rendering | `READY` | None for current render-only API; `App.cpp` still supplies a ready mode label and selected option | No remaining seam for pause rendering; future unification may replace private render labels with shared pause action metadata | 0 | Pass 24 converted this overlay to a normal module. No texture/resource ownership and no gameplay behavior. |
| `StageSelectOverlay.h` / `StageSelectOverlay.cpp` | 27 / 67 | Stage Select foreground rendering | `READY` | None for current prepared-view API; App.cpp still owns selection metadata lookup, row assembly, routing, loading, and `SDL_RenderPresent` | No remaining seam for Stage Select foreground rendering; resource/select-background seams remain separate | 0 | Pass 26 converted this overlay to a normal module. No `AppState`, `FighterState`, `SelectionState`, `StageSlot`, `CharacterSlot`, resource ownership, or gameplay behavior. |
| `MainMenuOverlay.h` / `MainMenuOverlay.cpp` | 14 / 78 | Main menu foreground rendering | `READY` | None for current foreground API; App.cpp still owns resource-backed title background and logo drawing | No remaining seam for foreground menu rendering; title background/logo resource seam remains separate | 0 | Pass 25 converted the AppState-free foreground menu module. Full title screen resource drawing intentionally stays in App.cpp. |
| `OptionsMenuOverlay.h` / `OptionsMenuOverlay.cpp` | 24 / 59 | Options menu foreground rendering | `READY` | None for current prepared-view API; App.cpp still owns settings row assembly, gamepad status text, behavior, title background, and `SDL_RenderPresent` | No remaining seam for Options foreground rendering; title-background and gamepad ownership seams remain separate | 0 | Pass 27 converted this overlay to a normal module. No `AppState`, `FighterState`, `MainSettings`, gamepad device objects, resource ownership, or controller behavior. |
| `VsScreenOverlay.h` / `VsScreenOverlay.cpp` | 28 / 101 | VS/loading screen rendering | `READY` | None for current prepared-view API; App.cpp still owns portrait/resource selection, load-status mapping, selected metadata assembly, routing, loading, and `SDL_RenderPresent` | No remaining seam for VS/loading presentation; broader sprite/resource ownership seams remain separate | 0 | Pass 28 converted this overlay to a normal module. Pass 29 migrated its portrait field to public non-owning `UiSpriteView`. |
| `CharacterSelectOverlay.h` | 102 | Character Select rendering | `NEEDS SMALL SEAM` | Takes `AppState`; reads selection state and resource-owned character/system sprites, but the public non-owning sprite view now exists | Prepared `CharacterSelectView` with rows, labels, selected index/page data, and `UiSpriteView` handles for face/icon/cell/cursor art | 5 | Pass 30 candidate. Keep select.def authority, cursor mutation, routing, loading, and resource ownership outside. |
| `FightHudOverlay.h` | 190 | Fight HUD rendering | `DEFER` | Reads `FighterState` life/power, fight round settings, match state, combo display, messages, gamepads, and helper labels | Public fight HUD view/read-only fight snapshot after fight runtime boundaries improve | 7 | Render-only today, but data source is runtime-heavy. Do not move hit/damage, combo mutation, timer, or round flow. |
| `FightResultOverlay.h` | 119 | Round/result overlay rendering | `DEFER` | Depends on match phase/ticks, round wins, result text helpers, fighters, victory quote helper, and result menu state | Public match/result presentation view after `MatchState`/`RoundState` audit | 8 | Keep round-flow decisions, result routing, rematch actions, and match-completion logic in App.cpp. |
| `FightPresentationShared.h` | 60 | Shared fight presentation helpers | `DEFER` | Reads match phase, round wins, gamepads, and fight round settings | Public fight presentation view or match-state seam | 9 | Shared by HUD/result overlays; convert with those modules, not before. |
| `TrainingOptionsOverlay.h` | 155 | F2 Training Options and move-list rendering | `DEFER` | Reads training options plus command entry metadata and App.cpp-local command display helpers | Public training-options view and command display view; keep command/CNS runtime out | 10 | Pure option labels are already in `TrainingOptionsBehavior`; move-list display still depends on command data/helpers. |
| `TrainingCommandOverlay.h` | 80 | Command/input HUD rendering | `DEFER` | Reads `FighterState` input history, command definitions, active command entries, and App.cpp-local command helpers | Public command HUD snapshot after input/command display seam | 11 | Keep CMD/CNS routing and command matching behavior in App.cpp. |
| `TrainingDebugOverlay.h` | 178 | Hitbox/debug overlay rendering | `DEFER` | Reads `FighterState`, camera, collision boxes, current animation frames, hit state, and debug clipboard | Public debug snapshot after fighter/render debug boundary | 12 | High runtime coupling despite being render-only. Do not convert first. |
| `UiRenderContext.h` / `UiRenderPrimitives.h` / `UiRenderPrimitives.cpp` | 10 / 31 / 87 | Color/rect/text/panel/scale helpers and resource-free UI render context | `READY` | No App.cpp-local dependency for primitive drawing; `App.cpp` still has a thin `uiRenderContext(...)` bridge because `AppState` is local | First overlay conversion needs a small view struct, starting with `PauseMenuView`; no primitive seam remains | 0 | Pass 23 completed the primitive seam. `debugText` is dependency-clean because it uses SDL debug text only. |
| `UiSpriteView.h` / `UiSpriteView.cpp` | 25 / 32 | Non-owning sprite view and top-left sprite drawing for overlays | `READY` | No ownership or App.cpp-local dependency; `App.cpp` still adapts local `TextureSprite` to `UiSpriteView` | Character Select conversion still needs a prepared view struct and App.cpp-owned sprite selection | 0 | Pass 29 completed the public sprite-view seam. It does not create, load, cache, or destroy textures. |
| `UiRenderHelpers.h` | 94 | Title/select background and fixed opponent slot rendering | `NEEDS PUBLIC RENDER SEAM` | Depends on `AppState`, `SystemScreenAssets`, `TextureSprite`, and App.cpp-local sprite helpers | Public sprite/background render seam or view structs with already-selected sprite references | 13 | Keep resource ownership and texture lifetime in App.cpp or a later resource module. |
| `FrontendFlow.h` | 411 | Existing key-flow dispatcher | `DEFER` | Behavior-coupled; calls App.cpp-local side-effect helpers for routing, loading, fight startup, reset, pause/result, and training behavior | Action-return boundary before normal module conversion | 14 | Not an overlay. Convert only after side effects are represented as actions and App.cpp remains executor. |

## Closest Conversion Candidates

1. `PauseMenuOverlay.h` / `PauseMenuOverlay.cpp`
   - Converted in Pass 24.
   - Proves the view-struct pattern for low-risk overlays.

2. `MainMenuOverlay.h` / `MainMenuOverlay.cpp`
   - Converted in Pass 25.
   - Proves the foreground-only module pattern when background/logo resources stay in App.cpp.

3. `StageSelectOverlay.h`
   - Converted in Pass 26.
   - Proves the prepared-view pattern for selection metadata screens.

4. `OptionsMenuOverlay.h`
   - Converted in Pass 27.
   - Proves the prepared-row pattern for settings-style menus while keeping gamepad/status ownership in App.cpp.

5. `VsScreenOverlay.h` / `VsScreenOverlay.cpp`
   - Converted in Pass 28.
   - Proves the prepared-view pattern for a resource-adjacent loading screen while keeping resource selection and ownership in App.cpp.
   - Migrated to `UiSpriteView` in Pass 29.

6. `CharacterSelectOverlay.h`
   - Next likely candidate area, but it is more resource-coupled than the completed menu overlays.
   - Now has the required sprite-view seam.
   - Needs a prepared view conversion that keeps selection semantics and resource ownership in App.cpp.

## Recommended Next Implementation Pass

Pass 23 completed option 2 for low-level UI primitives. Pass 24 converted the first overlay. Pass 25 converted the main-menu foreground while leaving resource-backed title background/logo drawing in App.cpp. Pass 26 converted Stage Select foreground rendering using a prepared view. Pass 27 converted Options foreground rendering using prepared rows. Pass 28 converted VS/loading presentation using prepared labels and a non-owning portrait texture view. Pass 29 created the public non-owning sprite-view seam and migrated VS/loading to it.

Recommended next pass:

```text
Pass 30: Convert Character Select overlay to normal module
```

Expected files to create or modify:

- create a normal `CharacterSelectOverlay.cpp`
- convert `CharacterSelectOverlay.h` to a normal declaration header with a prepared view struct
- update App.cpp to assemble prepared labels, selected/page data, and `UiSpriteView` handles for face/icon/cell/cursor art
- update CMake for the new `.cpp`
- keep character icons/faces, `SystemScreenAssets`, selection mutation, routing, loading, and resource ownership in App.cpp

Estimated risk: low to medium.

Expected `App.cpp` line reduction: likely zero or a small increase, because prepared view assembly is more explicit than hidden App.cpp-internal include coupling.

Expected hidden-coupling reduction:

- converts another frontend overlay from App.cpp-internal to normal module shape
- uses `UiSpriteView` without moving `TextureSprite`, `SystemScreenAssets`, loading ownership, `FighterState`, CMD/CNS, hit/damage, or round flow

Verification focus:

- build
- `dev_check`
- file-size guard expected App.cpp debt only
- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- GUI smoke for Character Select rendering if practical, plus Stage Select -> VS/loading -> fight

## Explicit Non-Goals

Do not start with:

- `FightHudOverlay.h`
- `FightResultOverlay.h`
- training debug/command overlays
- `FrontendFlow.h`
- actor/world rendering
- CMD/CNS
- hit/damage/guard
- round flow
- `FighterState`
- character/stage runtime loading
- audio
- resource ownership
- Dragon Mode
- store/crafting
- `.dragon/*.json`
