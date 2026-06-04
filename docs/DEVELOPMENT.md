# Development Workflow

Use this file as the short checklist before getting back into feature work.

## Daily Check

From a Visual Studio developer shell:

```powershell
python engine/tools/dev_check.py .
```

For a faster source-only pass:

```powershell
python engine/tools/dev_check.py . --skip-build
```

For a pre-commit-sized pass:

```powershell
python engine/tools/dev_check.py . --quick
```

## Git Hooks

This repo uses a tracked hook directory:

```powershell
git config core.hooksPath .githooks
```

The current pre-commit hook runs the architecture guard. It is intentionally narrow so normal commits stay fast, while the full `dev_check.py` command remains available before larger changes.

## Current Private-Repo Constraint

The active `game/` tree includes official M.U.G.E.N/KFM assets and local Evil Ryu/Evil Ken compatibility characters. Keep this repository private while those assets are tracked.

Before any public remote or release:

- Replace or remove third-party character assets.
- Replace official M.U.G.E.N/KFM assets with original or properly licensed content.
- Keep `content/research_mugen_chars/` ignored.
- Re-run the full development check.

## Before New Feature Work

1. Run `git status --short --ignored`.
2. Run `python engine/tools/dev_check.py . --skip-build`.
3. Confirm `docs/STRICT_ROADMAP.md` still matches the planned work.
4. If adding Dragon-only behavior, update `docs/DRAGON_EXTENSIONS.md` in the same change.
5. If a planned item is accepted but not implemented immediately, materialize it in the matching roadmap/audit document. Content plans also need a matching M.U.G.E.N-style folder or data file so they cannot disappear as untracked chat notes.
6. If adding original benchmark characters, update `docs/BENCHMARK_CHARACTERS.md`. Do not list README-only reserved folders in `game/data/select.def`.

## Feature Work Contract

Feature work follows [FEATURE_COMPLETION_POLICY.md](FEATURE_COMPLETION_POLICY.md). Create or update a spec under `docs/FEATURE_SPECS/` before coding. Only one feature may be marked `Status: In Progress`.

The active recovery feature is [0001_architecture_recovery.md](FEATURE_SPECS/0001_architecture_recovery.md). Until that is complete, broad gameplay work should be limited to fixes needed to preserve current behavior while extracting modules.

The postmortem for the current architecture drift is [FAILURE_POSTMORTEM_2026_06_04.md](FAILURE_POSTMORTEM_2026_06_04.md). Follow its prevention rules before touching `engine/src/App.cpp`.
