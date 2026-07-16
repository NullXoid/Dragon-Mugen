# Regression Checklist

This is the verification appendix, not the roadmap. Use [ENGINE_COMPLETION_ROADMAP.md](ENGINE_COMPLETION_ROADMAP.md) for the current completion plan and phase ordering.

Use this checklist before committing engine/app code. It exists because previous work was lost while adding new features.

Every engine commit must update both one preservation source (this checklist, the feature ledger, or the active feature spec) and root `CHANGELOG.md`. Internal cleanup notices must say that no player-visible behavior change is intended. Compatibility or Dragon extension documents change only when their supported data behavior changes; project handoff files change at milestones or branch-state changes rather than every small commit.

For cleanup changes, also confirm that audited public APIs remain declared, requested verifier fixtures never substitute roster slot zero, strict compiler warnings are clean, and removed future ideas are preserved in a roadmap/spec rather than dormant code.

Run `build\dragon_mugen.exe --verify missing-character-fixture-fails-setup` after changing verifier setup or roster lookup. It must report setup failure for the declared missing fixture without selecting a fallback character.

## Automated Checks

Run for source-level changes:

```powershell
python engine/tools/dev_check.py . --skip-build
```

Run from a Visual Studio developer shell for build-level changes:

```powershell
python engine/tools/dev_check.py .
```

Run a fresh configure/build from a Visual Studio developer shell after changing CMake targets or library types:

```powershell
cmake --fresh -S . -B build -G Ninja
cmake --build build --target dragon_mugen --config Debug
```

Run these for Arena DLC fight-loop changes:

```powershell
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify arena-cpu-2
build\dragon_mugen.exe --verify arena-cpu-3
build\dragon_mugen.exe --verify arena-z-keyboard-controls
build\dragon_mugen.exe --verify arena-z-gamepad-controls
build\dragon_mugen.exe --verify arena-z-hit-depth
build\dragon_mugen.exe --verify arena-z-push-depth
build\dragon_mugen.exe --verify arena-z-draw-order
build\dragon_mugen.exe --verify arena-camera-rotation-toggle
build\dragon_mugen.exe --verify arena-camera-rotation-projection
build\dragon_mugen.exe --verify arena-camera-rotation-draw-order
build\dragon_mugen.exe --verify arena-z-cpu-align
build\dragon_mugen.exe --verify arena-z-modifier-sidestep
build\dragon_mugen.exe --verify arena-per-fighter-runtime
build\dragon_mugen.exe --verify arena-openbor-scroll-stage
build\dragon_mugen.exe --verify arena-tmnt-openbor-stage
build\dragon_mugen.exe --verify story-mode-menu-route
build\dragon_mugen.exe --verify story-stage-select-map
build\dragon_mugen.exe --verify story-board-route-plan
build\dragon_mugen.exe --verify story-shop-door-trigger
build\dragon_mugen.exe --verify story-shop-route-resume
build\dragon_mugen.exe --verify story-openbor-stage-default
build\dragon_mugen.exe --verify story-stage-board-expansion
build\dragon_mugen.exe --verify story-wave-spawn-scroll
build\dragon_mugen.exe --verify story-enemy-targeting
build\dragon_mugen.exe --verify story-stage-clear
build\dragon_mugen.exe --verify story-player-defeat
build\dragon_mugen.exe --verify story-progression-award
build\dragon_mugen.exe --verify vs-loading-progress-bar
build\dragon_mugen.exe --verify sff-v2-png-decode
build\dragon_mugen.exe --verify ikemen-select-slot-parsing
build\dragon_mugen.exe --verify stage-music-codec-decode
build\dragon_mugen.exe --verify external-stage-mount
build\dragon_mugen.exe --verify story-scott-tram-rooftop
build\dragon_mugen.exe --verify runtime-performance-metrics
build\dragon_mugen.exe --verify story-wave3-performance
build\dragon_mugen.exe --verify arena-openbor-4fighter-performance
build\dragon_mugen.exe --verify render-culling-preserves-runtime
build\dragon_mugen.exe --verify arena-evilryu-air-special-contact-landing
build\dragon_mugen.exe --verify vs-p2-runtime
build\dragon_mugen.exe --verify kfm-guard-recovery
build\dragon_mugen.exe --verify kfm-specials-supers
build\dragon_mugen.exe --verify evilken-specials-supers
build\dragon_mugen.exe --verify evilken-air-special-contact-landing
build\dragon_mugen.exe --verify evilken-power-charge-helper
build\dragon_mugen.exe --verify evilken-training-demo-hit
build\dragon_mugen.exe --verify training-show-select-hold
build\dragon_mugen.exe --verify training-show-controller-shortcut
build\dragon_mugen.exe --verify training-command-side-switch-highlight
build\dragon_mugen.exe --verify training-command-facing-aware-display
build\dragon_mugen.exe --verify training-command-physical-direction-guide
build\dragon_mugen.exe --verify training-command-complete-blink
build\dragon_mugen.exe --verify training-command-filtered-complete  # Evil Ken + Evil Ryu variable-gated follow-up skip
build\dragon_mugen.exe --verify training-command-list-tabs
build\dragon_mugen.exe --verify training-command-icon-atlas
build\dragon_mugen.exe --verify training-palette-slot-separation
build\dragon_mugen.exe --verify character-auto-fit-scale
build\dragon_mugen.exe --verify evilryu-specials-supers
build\dragon_mugen.exe --verify evilryu-air-special-contact-landing
build\dragon_mugen.exe --verify evilryu-dash
build\dragon_mugen.exe --verify evilken-trip-grounding
build\dragon_mugen.exe --verify classic-fight-outcomes
build\dragon_mugen.exe --verify classic-fight-routing
build\dragon_mugen.exe --verify classic-fight-combat
build\dragon_mugen.exe --verify roster-compatibility-smoke
build\dragon_mugen.exe --verify dragon-progression-level-items
build\dragon_mugen.exe --verify dragon-progression-player-profiles
build\dragon_mugen.exe --verify shop-route-entry
build\dragon_mugen.exe --verify shop-room-actor-projection
build\dragon_mugen.exe --verify shop-room-movement-collision
build\dragon_mugen.exe --verify shop-buy-sell-persistence
build\dragon_mugen.exe --verify shop-equip-profile-scope
build\dragon_mugen.exe --verify shop-guest-no-save
build\dragon_mugen.exe --verify shop-controller-keyboard-navigation
build\dragon_mugen.exe --verify shop-panel-text-fit
build\dragon_mugen.exe --verify shop-demo-room-hook
build\dragon_mugen.exe --verify dragon-progression-enemy-reward
```

## Manual Play Flow

Check these when touching menu, input, loading, fight flow, or runtime behavior:

- Mode select opens.
- Training enters character select.
- Single Player enters character select.
- VS Mode enters character select.
- Arena Mode enters character select and then Arena Setup.
- Story Mode enters character select, prefers `TMNT OpenBOR Street` when available, starts through VS/loading, and reaches a side-scrolling enemy-wave fight.
- Story Mode Stage Select uses the Story-only connected episode-card/map presentation, cycles selectable parent boards with Left/Right, changes Story difficulty with Up/Down, keeps internal route nodes hidden from direct selection, defaults back to the configured OpenBOR board, and Enter still opens the shared VS/loading screen. The selected board, stage, wave count, difficulty, and target rows should remain readable on the stable virtual layout instead of crowding clipped cards onto the HD output.
- Story Mode enemy labels/status use `EASY`/`MED`/`HARD` difficulty, and difficulty scales enemy life/attack/defence without applying the player profile level to enemies.
- Story Mode fight HUD uses compact story-specific fighter bars/status and hides the normal round timer. Enemy waves should read as board progress, not as full M.U.G.E.N survival rounds.
- Story Mode `Soundcheck Alley` starts its configured WAV background music through normal stage `[Music] bgmusic` metadata.
- When `game/data/external_content.local.def` points at the local Scott Pilgrim Versus package, Story Mode Stage Select includes `Tram_Rooftop`, the stage loads through the shared VS/loading screen, SFF v2 PNG/palette stage art and first-pass animated BG elements render, and `Run Scott Run.mp3` starts through normal stage `[Music] bgmusic` metadata.
- Story Mode spawns difficulty-owned board waves: `EASY` ends with one boss wave, `MEDIUM` runs three waves with a midboss then boss, and `HARD` runs five waves with midboss checkpoints before the boss. The OpenBOR test board should use editable `[Enemy Setup]` roles named `grunts`, `mini_bosses`, and `bosses` so wave transitions are visually obvious without hardcoding specific character names in engine code.
- Story Mode route shop stops are data-driven: clearing a board before a shop node should show the configured shop-door cue, accept the mapped light-kick / PlayStation X action while the prompt is active, enter the Shop Hub, and resume the next playable route board after exiting the shop.
- Story Mode keeps inactive future-wave enemies invisible, scrolls forward only to the current wave gate, and has enemies chase P1 rather than each other.
- Story Mode clears to `STAGE CLEAR` after all waves, fails to `MISSION FAILED` when P1 is defeated, returns through match-result options, and awards P1 profile-owned Dragon XP/gold on clear with current balance in the result summary. Result overlays must keep text/panels inside the stable virtual layout instead of slipping back to 320x240 positioning at HD output.
- Evil Ryu Story supers briefly pause as authored, then recover to gameplay after helper/projectile hit runtime; Ryu must not remain stuck after the super.
- Story/Arena HUD shows compact per-fighter power strips under each active health bar so super availability remains visible while retaining the same generic CMD/CNS power gates and power consumption.
- Story/Arena wave stress should not spam per-hit terminal logs by default; enable `DRAGON_DEBUG_HIT_LOG=1` only when hit-event console traces are needed.
- Shop route opens the Dragon-only shop hub, lets P1 hold movement through the wider room, use Shift/left-trigger run with the tuned Phase 2 speed, stand in front of the counter by default, route around the counter through the top/back aisle, blocks direct walking through the counter body, walk to I.Chie/counter, open buy/sell/equip tabs, switch tabs with Q/E, L1/R1, or L2/R2, select an Equip target with Left/Right, confirm/cancel transactions with clear balance-aware banner/audio feedback, keep Buy/Sell/Equip icon/name/value rows and item detail/effect text inside the panel, persist named-profile gold/inventory/equipment, and keep Guest browse-only. With no generated room art installed, the fallback room should show shelf bays, neon I.Chie branding, dragon accents, and layered counter/wall art rather than plain placeholder bars. Optional generated room art under `game/data/shop/` should replace the fallback backdrop/counter layers without changing collision, depth sorting, or panel behavior.
- Story enemy defeats award configurable named-profile XP/gold once per defeated enemy, show floating `+XP +G` and coin-burst feedback, append current gold balance to Story reward/status text, and do not apply or persist those rewards to Guest.
- Arena Setup can start 1, 2, and 3 CPU free-for-all matches.
- Arena Setup can change CPU slots, stage, timer, Z Axis, and Camera Rotate without affecting Training, Single Player, or VS.
- Character select moves with Up/Down/Left/Right only when a character exists in the destination cell.
- Character select does not load full character runtime data.
- Character and stage select labels do not duplicate, overlap, or show the wrong mode name.
- Character select uses the same stable virtual-layout rule as Main Menu, Options, Story Select, loading, and results. At HD/fullscreen, the selected fighter card, opponent/mode card, roster grid, stage row, and footer controls should stay centered and readable instead of falling back to far-left/far-right `320x240` composition.
- Training character select shows one clear dummy opponent card.
- Stage select opens after character confirmation.
- Stage select previews the selected stage behind the menu, shows the stage name at the bottom, and refreshes when changing stages with Left/Right.
- Regular Stage Select uses the same stable virtual-layout rule as Main Menu, Options, Story Select, loading, and results. `stage-select-responsive-layout` must stay green so HD output does not collapse the stage menu into the old `320x240` top-strip layout.
- Main menu labels, top title text, panel header text, logo, panel geometry, menu row spacing, selection styling, panel shadow, and motif shadow remain editable presentation data. Dragon-owned values come from `game/data/dragon.def` `[Dragon.MainMenu]`, and compatible fallback labels come from M.U.G.E.N `game/data/system.def` `[Title Info]`. `main-menu-editable-presentation-data` and `main-menu-editable-layout-data` must stay green when touching menu loading or presentation.
- Stage confirmation opens the VS screen first.
- Fight view loads selected character and selected stage after VS.
- VS/Arena/Story loading shows actual load progress for character, stage, sprite/sound/runtime preparation, not only static `PLEASE WAIT` text. Loading and match-result presentation should use the same stable virtual layout rules as menus so the HD/fullscreen output improves clarity without shrinking the content into the top strip.
- Loading and match-result overlays should keep centered Dragon-style panels on the stable virtual layout. HD result screens should dim, not fully erase, the fight context behind the compact result and menu panels, and `vs-loading-progress-bar` should still support screenshot proof through `DRAGON_LOADING_SCREENSHOT`.
- VS/loading overlay geometry should be authored once on the stable `1280x720` presentation grid and remain centered/proportional at Classic, Wide, Extra, SD, and HD output presets rather than using per-resolution panel sizes.
- Fight view fully repaints the window during hitpause, camera shake, and result overlays; no stale desktop/debug text should appear around the game viewport.
- Video Options exposes Performance HUD `FPS`/`PERF`/`OFF`. `FPS` keeps the compact top-right counter visible by default; `PERF` shows frame-time/workload telemetry so live performance drops can be distinguished from gameplay hitpause, superpause, or state timing.
- Video resolution presets are output presets, not alternate game-layout canvases. `video-resolution-stable-virtual-layout` must stay green so Classic, Wide, Extra, SD, and HD keep one stable virtual presentation grid across menus and options.
- `video-resolution-presentation-e2e` must exercise the Video Options input path for Classic, Wide, Extra, SD, and HD across Mode Select, every Options page, Character Select, Arena Setup, regular and Story Stage Select, Versus, Fight/Training/Arena/Story HUD variants, pause/result overlays, Training Options, and both Shop states. Review its developer-only proof frames under `build/verification-proof/video-resolution`; all non-profile-specific screen compositions must match across presets.
- Dragon launches fullscreen with `1280x720 HD 720P` selected by default, keeps a `1280x720` windowed fallback, toggles fullscreen with `F11` or `Alt+Enter`, and minimizes with `Alt+M`. `video-hd-fullscreen-window-policy` must stay green.
- Fight view `F3` toggles Freeze Watch. Normal play should show only a small status badge; expanded fighter/helper details should appear only for sustained runtime or pose stalls.
- Fight view Start opens a lightweight pause/resume overlay. While this pause is open, Select/Back opens the full mode options menu.
- In Training, the lightweight pause overlay exposes command Show and Next/Previous controls without requiring the large options menu.
- Training options exposes an explicit `EXIT TO MAIN` / `MAIN MENU` path so backing out is not ESC-only.
- Fight view `F4` toggles screenshot freeze with only a small temporary notice so screenshots can capture frozen gameplay without the full options menu covering the screen.
- Arena and classic sweep/trip hits leave hitpause by entering the trip/fall/lying states instead of staying in grounded hitstun.
- Arena shows one health bar per active fighter, not a shared CPU health average.
- Arena knockdowns land on the floor and do not trigger viewport hitshake.
- Arena trip and heavy knockback hits resolve to floor impact before air recovery can take over.
- Arena timer ticks down, hit-frozen fighters recover or resolve to KO, and knockdowns do not pull the camera upward.
- Arena hitpause is brief, Rush counters reset after disappearing, debug hit boxes stay Training-only, and disabled timers show `INF`.
- Arena Z-axis modifier moves depth with Shift/left trigger using authored walk animation; A.Ben must continue advancing walk frames while moving up/down in depth rather than freezing on a single frame. Normal Up/Down still jump/crouch and still feed quarter-circle commands when the modifier is not held.
- Arena double-tapping the Z-axis modifier performs a short sidestep using authored walk animation; Up/Down on the second tap chooses the sidestep depth direction.
- Arena depth affects hit gating, player push, CPU alignment, projected sprite position, and draw order only when Z Axis is enabled.
- Arena Camera Rotate defaults off, only activates when Z Axis is also on, eases yaw from P1/living-fighter depth, and changes actor/effect projection and depth draw order without rotating backgrounds or combat math.
- Arena can select `OpenBOR Scroll Test` and `TMNT OpenBOR Street`; in Arena they scroll forward only, clamp at the configured end, and do not make Training, Single Player, or VS stages auto-scroll.
- Arena OpenBOR-style stages must be retested with four active fighters onscreen. `arena-openbor-4fighter-performance` and `render-culling-preserves-runtime` must stay green, and any sustained live low FPS must be fixed or captured with frame-time, actor/effect/projectile, and stage draw workload telemetry before OpenBOR Stage Compatibility v2 is marked complete.
- Story wave 3 stress must keep `story-wave3-performance` green; hitpause/superpause dips should be reported separately from non-pause gameplay frame time.
- Arena gamepad Start opens pause/start behavior only and is not mapped as a fighter button or depth input.
- Evil Ken crouch roundhouse trip follows the first low arc, hits the floor, then performs two small vertical-only floor bounces before knockdown without rising into air recovery.
- KFM, Evil Ken, and Evil Ryu supers are blocked below their authored CMD power gate and still consume power through CNS `poweradd` after valid entry.
- Character life, HUD max life, healing clamps, and damage scaling use each loaded fighter's CNS `[Data]` constants generically; KFM should start at `1000`, Evil Ken at `900`, and Evil Ryu at `950` without character-specific engine branches.
- Training dummy behavior still works.
- Training command HUD Show Me still starts from keyboard `H`, P1 controller L3/R3/touchpad, or a 2-second Select/Back hold; short Select/Back tap still advances to the next move.
- Training command HUD prefers optional Ikemen `movelist.dat` presentation text for move inputs, so human command cards can show diagonals such as `DB` even when CMD recognition uses a lenient shorthand.
- When a character DEF declares an Ikemen-style `movelist` file, Training command HUD/full command list treats that file as the authoritative displayed move list; unlisted internal command states should stay hidden.
- A.Ben Training command list stays sourced from `game/chars/A.Ben/movelist.dat` / character data and must not show unlisted internal/default moves such as `Boost Shot Medium`.
- Training command HUD/input history shows action-strength labels (`LP/MP/SP` and `LK/MK/SK`) instead of keyboard letters.
- Training command HUD/full command list switches to assigned P1 controller prompts when a controller is detected: Xbox-style `X/Y/LB` and `A/B/RB`, or PlayStation-style `SQ/TRI/L1` and `X/O/R1`.
- Training command HUD/full command list render facing-aware physical arrows when fighters switch sides, while live/recent input and D-pad guides show the actual physical direction pressed.
- Training full command list keeps selected bottom entries visible in both Main Techniques and All Techniques even when section headers are inserted into the visible rows.
- Training full command list starts on standard standing punches/kicks and orders sections as standing normals, crouching normals, air normals, specials, supers, throws, then counters.
- Training command HUD uses a compact top command strip plus a smaller live-input panel, not a large lower command card that covers the fight.
- Training command HUD, controller guide, and lightweight pause/help overlay render inside the Dragon safe area at HD/output presets; at `1280x720`, they remain readable without shrinking into old `320x240` coordinates or covering the fight.
- Training command HUD keeps live input/expected rows as lightweight floating clusters without a hard panel box, renders empty input history as plain `- - -` instead of a placeholder button, keeps the controller guide right-aligned without a redundant player label, uses the expected input highlight as the progress cue, reserves the top-right command strip status for real result feedback such as completion instead of a generic ready state, and renders the top objective as a strongly faded-end band without corner ticks.
- Training command HUD shows a full-command completion flash/checkmark when the selected input sequence is completed.
- Training P2 control still switches the opponent to local P2 behavior.
- Single Fight round timer, KO/time-over, pips, match result, and rematch/menu inputs still work.
- Dragon progression awards XP once on match result, writes local profile-owned save data under `game/save/progression.def`, displays P1/P2 profile and selected fighter LV/XP on Character Select, displays compact P1 LV/XP and local real-profile P2 LV/XP in the fight HUD, keeps P2 Guest non-persistent by default, prevents P1/P2 from sharing the same real profile, migrates old flat character saves into P1, awards VS XP to both non-Guest local profiles, and does not change Training or M.U.G.E.N-authored combat constants by default.
- Classic KFM, Evil Ken, and Evil Ryu guard contact/presentation/recovery, trip/fall recovery, and actual-hit KO routing stay covered by `classic-fight-combat`.
- Arena defeated fighters are ignored for targeting/win checks, and last-fighter-standing reaches the winner and end screens.
- Keyboard and controller inputs both feed the command buffer.
- KFM, Evil Ryu, and Evil Ken still appear from `game/data/select.def`.
- New character testing starts by adding the character to `game/data/select.def`, running `roster-compatibility-smoke`, and only then moving into command, damage, helper, projectile, throw, and super-specific audits.

## Compatibility Checks

- Do not hardcode character-specific behavior.
- Verify the relevant M.U.G.E.N source file for the behavior being changed:
  - `select.def` for roster/stage selection.
  - character DEF `[Files]` for runtime files.
  - CMD for commands and State -1.
  - CNS/ST/common1 for state behavior and controllers.
  - AIR for animation/collision routing.
  - SFF/ACT for sprites/palettes.
  - SND/fight common sounds for audio.
  - stage DEF/SFF for backgrounds, starts, bounds, zoffset, and camera.
- For external package tests, keep third-party assets outside `game/` and mount them through the git-ignored `game/data/external_content.local.def`.
- For IKEMEN packages, verify `slot = { ... }` metadata is ignored as metadata and not loaded as a fake character or stage.
- For SFF v2 stages, verify PNG-backed indexed sprites use their SFF palette records and decode without regressing SFF v1 PCX stages.
- Update `docs/COMPATIBILITY_AUDIT.md` if the runtime subset changes.
- Update `docs/FEATURE_LEDGER.md` if a feature behavior is added, removed, or intentionally replaced.
