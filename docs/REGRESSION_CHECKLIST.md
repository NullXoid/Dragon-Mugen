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
- Character select moves with Up/Down/Left/Right only when a character exists in the destination cell.
- Character select does not load full character runtime data.
- Character and stage select labels do not duplicate, overlap, or show the wrong mode name.
- Training character select shows one clear dummy opponent card.
- Stage select opens after character confirmation.
- Stage select previews the selected stage behind the menu, shows the stage name at the bottom, and refreshes when changing stages with Left/Right.
- Stage confirmation opens the VS screen first.
- Fight view loads selected character and selected stage after VS.
- Fight view fully repaints the window during hitpause, camera shake, and result overlays; no stale desktop/debug text should appear around the game viewport.
- Arena and classic sweep/trip hits leave hitpause by entering the trip/fall/lying states instead of staying in grounded hitstun.
- Arena shows one health bar per active fighter, not a shared CPU health average.
- Arena knockdowns land on the floor and do not trigger viewport hitshake.
- Arena trip and heavy knockback hits resolve to floor impact before air recovery can take over.
- Arena timer ticks down, hit-frozen fighters recover or resolve to KO, and knockdowns do not pull the camera upward.
- Arena hitpause is brief, Rush counters reset after disappearing, debug hit boxes stay Training-only, and disabled timers show `INF`.
- Evil Ken crouch roundhouse trip follows the first low arc, hits the floor, then performs two small vertical-only floor bounces before knockdown without rising into air recovery.
- Training dummy behavior still works.
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
