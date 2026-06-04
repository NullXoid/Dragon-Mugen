# Characters

Each character lives in its own folder:

```text
chars/
  kfm/
    kfm.def
    kfm.air
    kfm.cmd
    kfm.cns
    kfm.sff
    kfm.snd
    kfm.act
  EvilRyu/
    EvilRyu.def
  EvilKen/
    EvilKen.def
```

## Important Files

- `.def` - character manifest. Lists the files that make up the character.
- `.air` - animation definitions and collision boxes.
- `.cmd` - command/input definitions and move triggers.
- `.cns` - character constants, states, movement, attacks, guard flags, and behavior.
- `.sff` - sprite archive.
- `.snd` - character sound archive. Character-owned voices and move sounds belong here, while shared hit/guard/fight UI sounds belong under `game/data`.
- `.act` - palette files.
- `.dragon.def` - planned optional Dragon-only character sidecar. This is not M.U.G.E.N and must only contain extension metadata such as command-training labels, categories, RPG defaults, or editor hints.

## Select List

Characters can live in this folder without becoming selectable. The active roster is controlled by `game/data/select.def`, matching M.U.G.E.N's structure.

To add a character to the menu, add a line under `[Characters]` in `game/data/select.def`:

```ini
CharacterFolder, stages/kfm.def
```

Use the folder name when the main DEF is `chars/<folder>/<folder>.def`. Use a relative DEF path when the DEF name is different:

```ini
SomeFolder/alternate.def, stages/kfm.def
```

## Current Characters

`kfm/` is Kung Fu Man from the 2001 DOS M.U.G.E.N release. It is the first compatibility target for this engine.

`EvilRyu/` and `EvilKen/` are local compatibility stress-test characters. They are listed in `game/data/select.def` so the current runtime can prove it resolves selected character DEF `[Files]` entries instead of hardcoding KFM. They should not be shipped in a public build unless the project has the rights to do so.

`DragonBench/`, `A.Ben/`, and `I.Chie/` are reserved original benchmark character folders. They are intentionally not selectable yet because they do not have real M.U.G.E.N DEF/CMD/CNS/AIR/SFF files. Their plan is tracked in `docs/BENCHMARK_CHARACTERS.md`.

For a new character, copy the folder shape and keep the main `.def` file name aligned with the folder name.

## Extension Rule

Do not use a Dragon sidecar as the source of truth for moves.

Moves exist because the character `.cmd`, `.cns`, `.air`, `.sff`, and `.snd` define them. A future `<character>.dragon.def` may add presentation data for command training, editor metadata, or progression defaults, but it must not replace the M.U.G.E.N character files.
