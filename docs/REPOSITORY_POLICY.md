# Repository Policy

This repository is for local Dragon MUGEN engine development.

## Content Policy

- The active runtime root is `game/`.
- `game/data/select.def` controls which characters and stages are selectable.
- `content/research_mugen_chars/` is ignored by Git. It is a local download/archive area, not project source.
- Public releases must replace official M.U.G.E.N and third-party character assets with original or properly licensed content.
- Local compatibility characters may exist under `game/chars/` for testing, but they should be reviewed before pushing to any public remote.

## M.U.G.E.N Customization Preservation Rules

Dragon MUGEN must preserve M.U.G.E.N-style editability and customization. M.U.G.E.N files are creator-owned source of truth for M.U.G.E.N-style content, and Dragon-specific metadata is optional extension data.

Hard rules:

- Do not permanently rewrite imported character files.
- Do not permanently rewrite imported stage files.
- Do not hardcode roster, stages, or character-specific behavior.
- `select.def` remains the roster/stage authority unless a future explicitly documented alternate roster mode is approved.
- Character folders alone do not make characters selectable.
- Classic Mode must not require Dragon metadata.
- Classic Mode should be able to run supported M.U.G.E.N-style content without `.dragon.def`.
- Dragon-specific metadata must use the current repo convention: M.U.G.E.N-style `.dragon.def`.
- Do not introduce `.dragon/*.json` runtime sidecars.
- Do not migrate sidecar data to JSON.
- Do not replace editable M.U.G.E.N content with closed compiled-only data.
- Do not mix Dragon Mode data directly into `.cmd`, `.cns`, `.air`, `.sff`, `.snd`, `.act`, stage `.def`, or stage `.sff`.
- If `.dragon.def` is missing, Classic Mode should still load supported M.U.G.E.N content normally.
- If `.dragon.def` is missing, Dragon Mode should use safe defaults or mark Dragon enhancements unavailable.
- Generated caches, if any, must be clearly marked generated/cache and safe to delete/rebuild.
- Generated caches must not replace editable source files.

Original editable source files include `.def`, `.air`, `.cmd`, `.cns`, `.st`, `.sff`, `.snd`, `.act`, stage `.def`, stage `.sff`, `select.def`, `fight.def`, `system.def`, and motif files where supported.

JSON is not the runtime character/stage sidecar format. Developer or test artifacts may use JSON later only when they are clearly not runtime sidecars and not part of the character/stage customization format.

## Architecture Policy

- Run `python engine/tools/guard_architecture.py .` before commits that touch engine structure.
- Run `python engine/tools/dev_check.py . --skip-build` before resuming feature work after a repo/setup change.
- The CMake build runs `dragon_architecture_guard` automatically before `dragon_core`.
- If the guard fails, move ownership back into the correct module instead of weakening the guard.

## Commit Policy

- Commit source, docs, tools, and intentional runtime test content.
- Do not commit build output, personal saves, downloaded archives, or generated binaries.
- Update `docs/DRAGON_EXTENSIONS.md` when adding Dragon-only behavior that M.U.G.E.N does not define.
- Update `docs/STRICT_ROADMAP.md` before changing platform, renderer, folder layout, or milestone order.
