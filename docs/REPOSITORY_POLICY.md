# Repository Policy

This repository is for local Dragon MUGEN Core engine development.

## Content Policy

- The active runtime root is `game/`.
- `game/data/select.def` controls which characters and stages are selectable.
- `content/research_mugen_chars/` is ignored by Git. It is a local download/archive area, not project source.
- Public releases must replace official M.U.G.E.N and third-party character assets with original or properly licensed content.
- Local compatibility characters may exist under `game/chars/` for testing, but they should be reviewed before pushing to any public remote.

## Architecture Policy

- Run `python engine/tools/guard_architecture.py .` before commits that touch engine structure.
- The CMake build runs `dragon_architecture_guard` automatically before `dragon_core`.
- If the guard fails, move ownership back into the correct module instead of weakening the guard.

## Commit Policy

- Commit source, docs, tools, and intentional runtime test content.
- Do not commit build output, personal saves, downloaded archives, or generated binaries.
- Update `docs/DRAGON_EXTENSIONS.md` when adding Dragon-only behavior that M.U.G.E.N does not define.
- Update `docs/STRICT_ROADMAP.md` before changing platform, renderer, folder layout, or milestone order.
