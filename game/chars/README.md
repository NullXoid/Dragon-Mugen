# Characters

Each character lives in its own folder:

```text
chars/
  A.Ben/
    A.Ben.def
    A.Ben.air
    A.Ben.cmd
    A.Ben.cns
    A.Ben.sff
    A.Ben.snd
  I.Chie/
    I.Chie.def
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

`A.Ben/` and `I.Chie/` are the repo-owned Dragon MUGEN characters.

Other character folders such as `kfm/`, `EvilRyu/`, `EvilKen/`, `CFJ_Lili/`, `Dcat_Leo/`, and `DragonClaw/` are local-only compatibility references, blueprint ideas, or third-party tests. They can exist on a developer machine, but they are ignored by Git and should not be committed to the clean repo.

The active checked-in roster should only list characters the project owns or is allowed to ship. Local-only reference characters may be listed temporarily in a private working tree for compatibility testing, but should not be pushed.

For a new character, copy the folder shape and keep the main `.def` file name aligned with the folder name.

## Extension Rule

Do not use a Dragon sidecar as the source of truth for moves.

Moves exist because the character `.cmd`, `.cns`, `.air`, `.sff`, and `.snd` define them. A future `<character>.dragon.def` may add presentation data for command training, editor metadata, or progression defaults, but it must not replace the M.U.G.E.N character files.
