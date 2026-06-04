# Dragon MUGEN

Early prototype for a small M.U.G.E.N-like fighting runtime.

The first target is intentionally narrow:

- Load official 2001-era Kung Fu Man data.
- Load the official KFM dojo stage.
- Read `game/data/select.def` for the active character/stage roster.
- Run a single fight/training sandbox before adding menus, teams, story, shop, or equipment.
- Keep the content model readable like early M.U.G.E.N.
- Keep Training and Single Fight as distinct runtime experiences: Training exposes debug/tools, while Single Fight uses match controls and a pause menu.

Reference content is copied locally from `mugen-official-archive/mugen-2001.04.14-dos.zip` into the M.U.G.E.N-style runtime root:

```text
game/
  chars/
  data/
  font/
  plugins/
  save/
  sound/
  stages/
```

That content is for local research/prototyping only. The eventual engine should use original assets for anything public.

`content/reference_mugen_2001/` is retained only as a raw reference copy. Runtime work should use `game/`.

## Current State

This scaffold includes:

- CMake project layout.
- A C++ M.U.G.E.N-style section/key parser foundation.
- A command-line executable that inspects a character DEF file.
- A Python content inspection tool that validates KFM, the dojo stage, AIR/CMD/CNS shape, and SFF headers.
- A local compatibility roster that can load KFM plus larger test characters through `game/data/select.def`.
- First real M.U.G.E.N data modules: `engine/src/MugenData.cpp` owns roster loading, stage loading, selected character DEF `[Files]` resolution, and selected character constants; `engine/src/FightData.cpp` owns the current `game/data/fight.def` round/combo/powerbar settings subset. The app layer consumes that data; it should not rebuild the `data/`, `chars/`, and `stages/` path rules or `fight.def` settings.

The native build uses the Visual Studio developer environment. Plain PowerShell may not have `cl.exe` on PATH.

## Roadmap

The implementation contract is [docs/STRICT_ROADMAP.md](docs/STRICT_ROADMAP.md). It locks the engine direction to C++20+, CMake, SDL3, bgfx, M.U.G.E.N-style runtime folders, and the first playable KFM training sandbox.

Update the roadmap before making changes that alter platform, renderer, folder structure, milestone order, or content-loading assumptions.

Dragon-only features are tracked in [docs/DRAGON_EXTENSIONS.md](docs/DRAGON_EXTENSIONS.md). Anything the engine adds that M.U.G.E.N does not already define must be documented there with its file location, section/key format, defaults, and compatibility impact.

Third-party character downloads used for compatibility research are archived under `content/research_mugen_chars/`. They may also be copied into `game/chars/` for local runtime compatibility tests, but they only become selectable when listed in `game/data/select.def`, and they should not be shipped as public runtime content without proper rights.

## Verify Content

```powershell
python engine/tools/guard_architecture.py .
python engine/tools/inspect_mugen_content.py game
python engine/tools/audit_mugen_compat.py game
```

`guard_architecture.py` also runs automatically as the `dragon_architecture_guard` CMake target before `dragon_core` builds. It fails if roster/stage path rules, `select.def`, `fight.def`, or their ownership structs/loaders move back into the app layer, if required M.U.G.E.N runtime folders disappear, or if `App.cpp` exceeds its current split-before-growth budget.

The compatibility audit is documented in [docs/COMPATIBILITY_AUDIT.md](docs/COMPATIBILITY_AUDIT.md). It checks the active `game/data/select.def` roster, including local Evil Ryu/Evil Ken stress-test entries, against the current runtime subset.

## Intended First Runtime Milestones

1. Parse DEF, AIR, CMD, CNS, ACT, SFF, and SND enough to inspect KFM.
2. Decode SFF v1 sprites and ACT palettes.
3. Render the stage and one KFM idle animation.
4. Add fixed-step input, position, facing, velocity, and training dummy behavior.
5. Implement the smallest useful CNS subset: Statedef, ChangeState, ChangeAnim, VelSet, PosSet, HitDef placeholders.
6. Add a training mode loop with hitbox/hurtbox debug overlays.
