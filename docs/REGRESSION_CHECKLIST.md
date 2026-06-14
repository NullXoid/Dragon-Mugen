# Regression Checklist

Use this checklist before committing engine/app code. It exists because previous work was lost while adding new features.

## Automated Checks

Run for source-level changes:

```powershell
python engine/tools/dev_check.py . --skip-build
```

Run from a Visual Studio developer shell for build-level changes:

```powershell
python engine/tools/dev_check.py .
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
build\dragon_mugen.exe --verify character-auto-fit-scale
build\dragon_mugen.exe --verify evilryu-specials-supers
build\dragon_mugen.exe --verify evilryu-air-special-contact-landing
build\dragon_mugen.exe --verify evilryu-dash
build\dragon_mugen.exe --verify evilken-trip-grounding
```

## Manual Play Flow

Check these when touching menu, input, loading, fight flow, or runtime behavior:

- Mode select opens.
- Training enters character select.
- Single Player enters character select.
- VS Mode enters character select.
- Arena Mode enters character select and then Arena Setup.
- Arena Setup can start 1, 2, and 3 CPU free-for-all matches.
- Arena Setup can change CPU slots, stage, timer, Z Axis, and Camera Rotate without affecting Training, Single Player, or VS.
- Character select moves with Up/Down/Left/Right only when a character exists in the destination cell.
- Character select does not load full character runtime data.
- Character and stage select labels do not duplicate, overlap, or show the wrong mode name.
- Training character select shows one clear dummy opponent card.
- Stage select opens after character confirmation.
- Stage select previews the selected stage behind the menu, shows the stage name at the bottom, and refreshes when changing stages with Left/Right.
- Stage confirmation opens the VS screen first.
- Fight view loads selected character and selected stage after VS.
- Fight view fully repaints the window during hitpause, camera shake, and result overlays; no stale desktop/debug text should appear around the game viewport.
- Fight view `F3` toggles Freeze Watch. Normal play should show only a small status badge; expanded fighter/helper details should appear only for sustained runtime or pose stalls.
- Arena and classic sweep/trip hits leave hitpause by entering the trip/fall/lying states instead of staying in grounded hitstun.
- Arena shows one health bar per active fighter, not a shared CPU health average.
- Arena knockdowns land on the floor and do not trigger viewport hitshake.
- Arena trip and heavy knockback hits resolve to floor impact before air recovery can take over.
- Arena timer ticks down, hit-frozen fighters recover or resolve to KO, and knockdowns do not pull the camera upward.
- Arena hitpause is brief, Rush counters reset after disappearing, debug hit boxes stay Training-only, and disabled timers show `INF`.
- Arena Z-axis modifier moves depth with Shift/left trigger using authored walk animation; normal Up/Down still jump/crouch and still feed quarter-circle commands when the modifier is not held.
- Arena double-tapping the Z-axis modifier performs a short sidestep using authored walk animation; Up/Down on the second tap chooses the sidestep depth direction.
- Arena depth affects hit gating, player push, CPU alignment, projected sprite position, and draw order only when Z Axis is enabled.
- Arena Camera Rotate defaults off, only activates when Z Axis is also on, eases yaw from P1/living-fighter depth, and changes actor/effect projection and depth draw order without rotating backgrounds or combat math.
- Arena can select `OpenBOR Scroll Test`; in Arena it scrolls forward only, clamps at the configured end, and does not make Training, Single Player, or VS stages auto-scroll.
- Arena gamepad Start opens pause/start behavior only and is not mapped as a fighter button or depth input.
- Evil Ken crouch roundhouse trip follows the first low arc, hits the floor, then performs two small vertical-only floor bounces before knockdown without rising into air recovery.
- KFM, Evil Ken, and Evil Ryu supers are blocked below their authored CMD power gate and still consume power through CNS `poweradd` after valid entry.
- Training dummy behavior still works.
- Training command HUD Show Me still starts from keyboard `H`, P1 controller L3/R3/touchpad, or a 2-second Select/Back hold; short Select/Back tap still advances to the next move.
- Training command HUD prefers optional Ikemen `movelist.dat` presentation text for move inputs, so human command cards can show diagonals such as `DB` even when CMD recognition uses a lenient shorthand.
- Training command HUD/input history shows action-strength labels (`LP/MP/SP` and `LK/MK/SK`) instead of keyboard letters.
- Training command HUD/full command list switches to assigned P1 controller prompts when a controller is detected: Xbox-style `X/Y/LB` and `A/B/RB`, or PlayStation-style `SQ/TRI/L1` and `X/O/R1`.
- Training P2 control still switches the opponent to local P2 behavior.
- Single Fight round timer, KO/time-over, pips, match result, and rematch/menu inputs still work.
- Arena defeated fighters are ignored for targeting/win checks, and last-fighter-standing reaches the winner and end screens.
- Keyboard and controller inputs both feed the command buffer.
- KFM, Evil Ryu, and Evil Ken still appear from `game/data/select.def`.

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
- Update `docs/COMPATIBILITY_AUDIT.md` if the runtime subset changes.
- Update `docs/FEATURE_LEDGER.md` if a feature behavior is added, removed, or intentionally replaced.
