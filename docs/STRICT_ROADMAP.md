# Strict Roadmap

This file is now the stable engine-direction contract. The current completion sequence lives in [ENGINE_COMPLETION_ROADMAP.md](ENGINE_COMPLETION_ROADMAP.md).

## Non-Negotiable Direction

- Language: C++20 or newer for the engine/runtime.
- Build: CMake.
- Platform layer: SDL3.
- Renderer direction: bgfx.
- Runtime/content root: `game/`, using M.U.G.E.N-style folders and names.
- Roster/stage authority: `game/data/select.def`.
- M.U.G.E.N files remain creator-owned source of truth.
- Dragon-only features must be documented in [DRAGON_EXTENSIONS.md](DRAGON_EXTENSIONS.md).
- Classic Mode must not require Dragon metadata.
- Character files, stage files, and roster data must not be hardcoded or permanently rewritten by engine fixes.

## Runtime Folder Contract

```text
dragon-mugen/
  engine/
  game/
    chars/
    data/
    font/
    sound/
    stages/
    plugins/
    save/
  docs/
  build/
  CMakeLists.txt
  README.md
```

Rules:

- `game/chars/<character>/` contains character `.def`, `.air`, `.cmd`, `.cns`, `.st`, `.sff`, `.snd`, `.act`, and character storyboard files.
- `game/stages/` contains stage `.def` and `.sff` files.
- `game/data/` contains `mugen.cfg`, `select.def`, `system.def`, `fight.def`, `common1.cns`, fight FX, and later ruleset data.
- `game/font/`, `game/sound/`, `game/plugins/`, and `game/save/` keep their M.U.G.E.N-style responsibilities.
- Character folders alone do not make characters selectable; `select.def` does.
- Dragon sidecars use `.dragon.def`, not JSON character/stage runtime sidecars.

## Current Planning Source

Use [ENGINE_COMPLETION_ROADMAP.md](ENGINE_COMPLETION_ROADMAP.md) for:

- current phase ordering,
- remaining engine work,
- current risks,
- done criteria,
- active verification gates.

Use [FEATURE_LEDGER.md](FEATURE_LEDGER.md) for preserved behavior history and [REGRESSION_CHECKLIST.md](REGRESSION_CHECKLIST.md) for repeatable checks.

## Explicitly Not Allowed

- No Win32-only app layer.
- No DirectX-only renderer.
- No Godot/Unity/browser runtime pivot.
- No engine-owned content layout that hides M.U.G.E.N concepts.
- No feature work that bypasses editable M.U.G.E.N files as source of truth.
- No story, shop, equipment, tournament, networking, or editor work before the engine core is stable enough to support it.
