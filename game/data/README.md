# Data

This folder contains global runtime configuration and shared game definitions.

## Important Files

- `mugen.cfg` - engine options such as video, sound, input, and rules.
- `select.def` - character and stage selection list.
- `system.def` - title/menu/select screen layout.
- `fight.def` - lifebars, power bars, round text, and fight UI.
- `common1.cns` - shared character states used by characters.
- `fightfx.air` / `fightfx.sff` - common fight effects, including hit and guard sparks referenced by character `HitDef` values such as `sparkno`, `guard.sparkno`, and `sparkxy`.
- `common.snd` - shared common fight sounds such as fallback hit and guard sounds.
- `fight.snd` - fight announcer, round, KO, power, and lifebar/UI sounds.
- `system.snd` - title/menu/select-system sounds.
- `dragon.def` - planned Dragon-only project defaults. This is not a M.U.G.E.N file and must be documented in [../../docs/DRAGON_EXTENSIONS.md](../../docs/DRAGON_EXTENSIONS.md) before the engine depends on any key in it.

## Editing Notes

For the first prototype, the engine reads only a small subset of these files. Keep the original structure intact so compatibility work can proceed in order.

`common1.cns` is especially important because characters can reference it through `stcommon = common1.cns`.

Do not place shared hit or guard sparks inside a character folder. Character CNS files reference them, but the common fight effect animations and sprites belong here in `game/data`.

Shared hit and guard sounds also belong here. Character-specific voices or move sounds belong in `game/chars/<character>/<character>.snd`.

## Settings Rules

Use M.U.G.E.N files for M.U.G.E.N concepts:

- `mugen.cfg` for compatible global engine options.
- `system.def` for title, menu, select, and system presentation.
- `fight.def` for lifebars and fight UI.
- `select.def` for character and stage selection.

Use Dragon files only for Dragon-only concepts:

- Project defaults that M.U.G.E.N does not define go in planned `dragon.def`.
- Local player settings go in `game/save/settings.def`, not in this folder.
- Every Dragon-only section/key must be registered in [../../docs/DRAGON_EXTENSIONS.md](../../docs/DRAGON_EXTENSIONS.md).

## Select Rules

`select.def` is the roster authority. A character folder under `game/chars/` is only storage until it is listed under `[Characters]`.

Supported current entry forms:

```ini
kfm, stages/kfm.def
EvilRyu, stages/kfm.def
SomeFolder/alternate.def, stages/kfm.def
```

Current parser behavior:

- Reads `[Characters]` in order and keeps that order on the character select screen.
- Resolves `FolderName` to `game/chars/FolderName/FolderName.def`.
- Resolves `FolderName/file.def` to `game/chars/FolderName/file.def`.
- Reads each roster character DEF `[Files]` `sprite` and palette data only far enough to load select portraits (`9000,0` icon and `9000,1` face).
- Confirming character select currently writes the P1 session slot only. Opponent slots are mode-owned (`DUMMY` for Training, `CPU` for Single Player, `LocalP2` for VS/local) until a separate opponent/P2 selection flow is implemented.
- Shows the VS screen before loading full fight-session assets.
- Reads the selected character DEF `[Files]` section for CMD, CNS, AIR, SFF, SND, palettes, and `stcommon` only after the VS screen has been presented.
- Loads the selected stage background only after the VS screen has been presented.
- Reads character stages from the second comma value when it is not `random` and `includestage` is not `0`.
- Reads `[ExtraStages]` as additional stage-select entries.

Folder scanning is only a fallback when `select.def` is missing or has no valid character/stage entries.
