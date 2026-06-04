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

## Manual Play Flow

Check these when touching menu, input, loading, fight flow, or runtime behavior:

- Mode select opens.
- Training enters character select.
- Single Player enters character select.
- VS Mode enters character select.
- Character select moves with Up/Down/Left/Right only when a character exists in the destination cell.
- Character select does not load full character runtime data.
- Stage select opens after character confirmation.
- Stage confirmation opens the VS screen first.
- Fight view loads selected character and selected stage after VS.
- Training dummy behavior still works.
- Training P2 control still switches the opponent to local P2 behavior.
- Single Fight round timer, KO/time-over, pips, match result, and rematch/menu inputs still work.
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
