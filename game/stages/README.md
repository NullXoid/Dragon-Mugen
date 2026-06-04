# Stages

Stage files live here.

## Typical Files

```text
stages/
  kfm.def
  kfm.sff
```

## Important Files

- `.def` - stage definition. Contains camera bounds, player start positions, stage info, music settings, shadows, and background elements.
- `.sff` - stage sprite archive.
- `.dragon.def` - planned optional Dragon-only stage sidecar. This is not M.U.G.E.N and must only contain extension metadata such as previews or editor/training hints.

## Editing Background Images

Background images stay in the M.U.G.E.N stage backend.

To change a stage background:

1. Add or replace sprites in the stage `.sff`.
2. Edit the stage `.def` `[BGDef]` and `[BG ...]` sections.
3. Keep the stage `.def` and `.sff` together in `game/stages/`.

Common background keys:

- `[BGDef] spr` chooses the stage SFF file.
- `spriteno` chooses the sprite group/image inside the SFF.
- `start` positions the background element.
- `delta` controls camera-relative movement.
- `tile` repeats the image.
- `mask` enables transparency.
- `layerno` decides whether the element draws behind or in front of fighters.

Do not put playable stage background definitions in a Dragon extension file. Dragon sidecars may add extras later, but the stage remains a M.U.G.E.N `.def` plus `.sff`.

## Current Stages

- `kfm.def` / `kfm.sff` - Mountainside Temple, the first stage compatibility target.
- `stage0.def` / `stage0.sff` - additional sample stage from the 2001 package.

For the first renderer milestone, `kfm.def` should be parsed and its 7 background elements accounted for.
