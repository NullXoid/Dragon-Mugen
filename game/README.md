# Game Folder

This is the runtime content root. It intentionally follows the M.U.G.E.N folder model so creators know where things belong.

The engine roadmap and folder-structure contract live in [../docs/STRICT_ROADMAP.md](../docs/STRICT_ROADMAP.md).

Dragon-only extension rules live in [../docs/DRAGON_EXTENSIONS.md](../docs/DRAGON_EXTENSIONS.md). If a feature is not plain M.U.G.E.N, it must be documented there before the runtime depends on it.

## Folders

- `chars/` - playable characters and CPU opponents.
- `data/` - engine configuration, select lists, system screens, fight UI, and common states.
- `font/` - font assets used by menus, lifebars, debug text, and story screens.
- `sound/` - shared/global sounds not owned by one character.
- `stages/` - stage definitions and stage sprites.
- `plugins/` - reserved for future optional engine extensions.
- `save/` - local profiles, settings, unlocks, and progression data.

## Editing Rule

Prefer editing text definition files first. Binary assets such as `.sff`, `.snd`, and `.act` should be edited with tools once the engine/editor supports them.

The current reference content comes from the official 2001 DOS M.U.G.E.N package for research and compatibility testing.

Keep M.U.G.E.N files authoritative:

- Stage backgrounds are changed in `stages/<stage>.def` and `stages/<stage>.sff`.
- Character moves are changed in `chars/<character>/<character>.cmd`, `.cns`, `.air`, `.sff`, and `.snd`.
- System/select/fight UI belongs in `data/system.def`, `data/select.def`, and `data/fight.def`.
- Local user preferences belong in `save/`, not in character or stage folders.

Dragon-only sidecars may be added later, but they extend this layout instead of replacing it.
