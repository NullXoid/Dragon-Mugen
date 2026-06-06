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

## Readiness Labels

- `READY`: can become a normal `.h/.cpp` module with existing public headers and no App.cpp-local dependencies.
- `NEEDS SMALL SEAM`: needs one small public type, view model, helper, or context, with no runtime/resource ownership change.
- `NEEDS PUBLIC RENDER SEAM`: depends on App.cpp-local render primitives, `TextureSprite`, `SystemScreenAssets`, or sprite helpers.
- `DEFER`: depends on `FighterState`, AppState internals, hit/damage, round flow, runtime loading, command/CNS runtime, or other high-risk systems.

## Conversion Inventory

| Header | Lines | Responsibility | Readiness | Main Blocker | Minimum Seam Needed | Recommended Priority | Notes |
|---|---:|---|---|---|---|---|---|
| `PauseMenuOverlay.h` | 50 | Single-fight pause menu rendering | `NEEDS SMALL SEAM` | Takes App.cpp-local `AppState`; uses `ScopedUiScale`, render primitives, and `pendingModeTitle` | Public `PauseMenuView`, public UI primitives, and a small UI scale context that does not require `AppState` | 1 | Best first overlay candidate after the UI seam. No texture/resource ownership and no gameplay behavior. |
| `StageSelectOverlay.h` | 61 | Stage Select rendering | `NEEDS SMALL SEAM` | Takes App.cpp-local `AppState`; uses selection fields plus App.cpp-local label helpers and render primitives | Public `StageSelectView` assembled by App.cpp plus public UI primitives/text helpers | 2 | Good second candidate. Avoid moving stage selection, routing, preferred-stage mutation, or loading. |
| `MainMenuOverlay.h` | 71 | Main/title menu rendering | `NEEDS PUBLIC RENDER SEAM` | Takes `AppState`; uses `drawTitleBackground`, `systemScreens.titleLogo`, and sprite helpers | Public menu view plus public title-background/sprite drawing seam or a title-background view | 3 | Visually simple but resource-backed title background/logo makes it harder than pause/stage. |
| `OptionsMenuOverlay.h` | 92 | Options menu rendering | `NEEDS PUBLIC RENDER SEAM` | Takes `AppState`; uses `drawTitleBackground`, `ScopedUiScale`, gamepad/status helpers, and App.cpp-local setting text helpers | Public options view, public settings status helpers, public UI primitives, and title-background render seam | 4 | Do not move gamepad open/close, assignment ownership, persistence, or SDL resource behavior. |
| `VsScreenOverlay.h` | 66 | VS/loading screen rendering | `NEEDS PUBLIC RENDER SEAM` | Takes `AppState`; reads portraits, face sprites, loading flags, selected metadata, and fixed-opponent slot helper | Public VS/loading view plus sprite/portrait render seam | 5 | Keep fight preparation, loading, selected runtime data, and routing in App.cpp. |
| `CharacterSelectOverlay.h` | 102 | Character Select rendering | `NEEDS PUBLIC RENDER SEAM` | Takes `AppState`; reads character icons/faces, `SystemScreenAssets`, selection state, and sprite helpers | Public character-select view plus sprite/icon/cursor render seam | 6 | Resource-heavy compared with pause/stage. Keep select.def authority, cursor mutation, and loading outside. |
| `FightHudOverlay.h` | 190 | Fight HUD rendering | `DEFER` | Reads `FighterState` life/power, fight round settings, match state, combo display, messages, gamepads, and helper labels | Public fight HUD view/read-only fight snapshot after fight runtime boundaries improve | 7 | Render-only today, but data source is runtime-heavy. Do not move hit/damage, combo mutation, timer, or round flow. |
| `FightResultOverlay.h` | 119 | Round/result overlay rendering | `DEFER` | Depends on match phase/ticks, round wins, result text helpers, fighters, victory quote helper, and result menu state | Public match/result presentation view after `MatchState`/`RoundState` audit | 8 | Keep round-flow decisions, result routing, rematch actions, and match-completion logic in App.cpp. |
| `FightPresentationShared.h` | 60 | Shared fight presentation helpers | `DEFER` | Reads match phase, round wins, gamepads, and fight round settings | Public fight presentation view or match-state seam | 9 | Shared by HUD/result overlays; convert with those modules, not before. |
| `TrainingOptionsOverlay.h` | 155 | F2 Training Options and move-list rendering | `DEFER` | Reads training options plus command entry metadata and App.cpp-local command display helpers | Public training-options view and command display view; keep command/CNS runtime out | 10 | Pure option labels are already in `TrainingOptionsBehavior`; move-list display still depends on command data/helpers. |
| `TrainingCommandOverlay.h` | 80 | Command/input HUD rendering | `DEFER` | Reads `FighterState` input history, command definitions, active command entries, and App.cpp-local command helpers | Public command HUD snapshot after input/command display seam | 11 | Keep CMD/CNS routing and command matching behavior in App.cpp. |
| `TrainingDebugOverlay.h` | 178 | Hitbox/debug overlay rendering | `DEFER` | Reads `FighterState`, camera, collision boxes, current animation frames, hit state, and debug clipboard | Public debug snapshot after fighter/render debug boundary | 12 | High runtime coupling despite being render-only. Do not convert first. |
| `UiRenderPrimitives.h` | 110 | Color/rect/text/panel/scale helpers | `NEEDS SMALL SEAM` | Mixed AppState-free primitives with `uiScale`/`ScopedUiScale` that depend on App.cpp-local `AppState`, `logicalWidthF`, and render constants | Split AppState-free primitives into a normal module; introduce `UiScaleContext` for scaled UI | 0 | This is the enabling seam for pause/stage/menu conversion. |
| `UiRenderHelpers.h` | 94 | Title/select background and fixed opponent slot rendering | `NEEDS PUBLIC RENDER SEAM` | Depends on `AppState`, `SystemScreenAssets`, `TextureSprite`, and App.cpp-local sprite helpers | Public sprite/background render seam or view structs with already-selected sprite references | 13 | Keep resource ownership and texture lifetime in App.cpp or a later resource module. |
| `FrontendFlow.h` | 411 | Existing key-flow dispatcher | `DEFER` | Behavior-coupled; calls App.cpp-local side-effect helpers for routing, loading, fight startup, reset, pause/result, and training behavior | Action-return boundary before normal module conversion | 14 | Not an overlay. Convert only after side effects are represented as actions and App.cpp remains executor. |

## Closest Conversion Candidates

1. `PauseMenuOverlay.h`
   - Smallest render-only overlay with no texture ownership and no resource-backed background.
   - Still blocked by App.cpp-local `AppState` and UI scale/render helpers.

2. `StageSelectOverlay.h`
   - Small and mostly selection metadata plus text/panel primitives.
   - Needs a public `StageSelectView` so the normal module does not depend on App.cpp-local `AppState`.

3. `MainMenuOverlay.h`
   - Small and high-value, but resource-backed title/logo rendering means it needs a public render/sprite seam before conversion.

## Recommended Next Implementation Pass

Choose option 2: create a small public UI render/view seam first.

Recommended next pass:

```text
Pass 23: Public UI Render Primitive Seam
```

Expected files to create or modify:

- create or convert a normal public/internal engine module for AppState-free UI primitives, such as `engine/src/UiRenderPrimitives.h` plus `engine/src/UiRenderPrimitives.cpp`
- update CMake for the new `.cpp`
- keep resource-backed helpers such as title/select backgrounds and sprite drawing transitional for now
- optionally introduce a small `UiScaleContext` so scaled overlays do not need `AppState`
- do not convert any overlay in the same pass unless the primitive seam is trivial and fully verified

Estimated risk: low to medium.

Expected `App.cpp` line reduction: likely zero or small. The value is hidden-coupling reduction, not physical line removal.

Expected hidden-coupling reduction:

- removes the lowest-level SDL UI primitive dependency from App.cpp include order
- gives `PauseMenuOverlay.h` and `StageSelectOverlay.h` a path toward normal `.cpp` modules
- avoids pulling `TextureSprite`, `SystemScreenAssets`, `FighterState`, CMD/CNS, hit/damage, loading, or round flow into public module boundaries

Verification focus:

- build
- `dev_check`
- file-size guard expected App.cpp debt only
- `kfm-baseline`
- `evilken-smoke`
- `kfm-air-state`
- `cpu-baseline`
- GUI smoke for Main -> Training -> Stage Select -> fight, F1/F2/R, Esc backout if implementation touches visible rendering

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
