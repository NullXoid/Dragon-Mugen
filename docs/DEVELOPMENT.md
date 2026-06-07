# Development Workflow

Use this file as the short checklist before getting back into feature work.

## Daily Check

From a Visual Studio developer shell:

```powershell
python engine/tools/dev_check.py .
```

The full check configures/builds the executable, runs the console roster check, and then runs the scripted runtime verifiers: `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline`.

For a faster source-only pass:

```powershell
python engine/tools/dev_check.py . --skip-build
```

For a pre-commit-sized pass:

```powershell
python engine/tools/dev_check.py . --quick
```

## Playable Core Verification

Run these after changes that touch input, CMD/CNS routing, movement, hit detection, or fight runtime:

```powershell
build\dragon_mugen.exe --verify kfm-baseline
build\dragon_mugen.exe --verify evilken-smoke
build\dragon_mugen.exe --verify kfm-air-state
build\dragon_mugen.exe --verify cpu-baseline
```

These scenarios use internal symbolic input to verify the runtime path. They do not prove physical keyboard or controller hardware mapping.

## File Size Guard

Run the source file-size audit before and after refactor or verification changes:

```powershell
python tools/check_file_sizes.py
```

The guard uses tiers:

- `<= 350` lines: small-file target for most focused modules.
- `351-750` lines: medium file; acceptable for cohesive engine modules, but watch for mixed responsibilities.
- `751-1000` lines: large file; keep visible as extraction debt unless the cohesion is clearly justified.
- `> 1000` lines: hard failure unless explicitly allowlisted for a justified generated/vendor/data-only reason.

New files should still prefer the small target, and transitional render/type headers should stay well below it when practical. `engine/src/App.cpp` is intentionally not allowlisted because it is the current known monolith and must stay visible as debt until it is extracted one responsibility at a time.

For the current `App.cpp` recovery map, see [ARCHITECTURE_RECOVERY_ROADMAP_AUDIT.md](ARCHITECTURE_RECOVERY_ROADMAP_AUDIT.md) and the focused runtime-controller audits.

## Pass 11 Type Dependency Map

Pass 11 moved safe app/UI constants and pure data types from `engine/src/App.cpp` into `engine/src/AppTypes.h`. This was type prep only; no behavior, input, routing, loading, gameplay, controller, CMake, or sidecar policy changed.

### Moved safe pure app/UI state

Constants moved:

- Window/logical size constants: `kWindowWidth`, `kWindowHeight`, `kClassicLogicalWidth`, `kDefaultLogicalWidth`, `kExtraWideLogicalWidth`, `kLogicalHeight`, `kLogicalWidth`.
- Training option constants: `kTrainingOptionCount`, `kTrainingOptionRows`, `kTrainingP2ControlOption`, `kTrainingCommandHudOption`, `kTrainingInputHudOption`, `kTrainingPowerOption`, `kTrainingMoveTypeOption`, `kTrainingMoveListOption`, `kTrainingResetOption`.
- Menu/result constants: `kSingleFightPauseOptionCount`, `kMatchResultOptionCount`, `kMainSettingsCount`.
- Flow/layout constants: `kVersusPrepareStartFrames`, `kCharacterSelectColumns`, `kCharacterSelectRows`, `kCharacterSelectPageSize`.

Enums moved:

- `Screen`, `PendingMode`, `OpponentType`, `MatchPhase`, `RoundEndReason`.
- `DummyGuardMode`, `TrainingPowerMode`, `TrainingMoveCategory`, `GamepadPromptStyle`.

Structs moved:

- `TrainingOptions`, `MainSettings`, `LoadedContentSummary`, `ComboCounterState`, `FightSessionSlots`.

`ComboCounterState` moved because it contains only already-computed combo display counters. `FightSessionSlots` moved because it contains only character slot indices and opponent type, with no loaded resources or runtime ownership.

### Planned types not moved

None of the planned safe constants/enums/structs were skipped after inspection. Existing extracted input types remain owned by `engine/src/Input.h` and were not duplicated.

### Left runtime-coupled state

- `FighterState`
- fight/session runtime
- CMD/CNS runtime
- hit/damage state
- projectile/helper/effect state

### Left resource-coupled state

- `TextureSprite`
- `SystemScreenAssets`
- `AnimationClip`
- `RuntimeEffect`
- `RuntimeProjectile`
- `AudioState`
- SDL texture/audio/resource ownership
- loaded sprite/stage/character caches and asset/cache structures

### Left behavior-coupled state

- `AppState`
- `HitDefinition`
- state controller and command/CNS types
- parser/runtime helper types
- routing-heavy state
- loading-heavy state
- anything depending on anonymous-namespace behavior helpers

### Why AppState and FighterState remain

`AppState` remains in `App.cpp` because it aggregates UI state, fight runtime state, resource-owned state, loaded content, routing state, verification state, and fight/session ownership.

`FighterState` remains in `App.cpp` because it is runtime-heavy and tightly coupled to gameplay, animation, hit logic, CMD/CNS, and fight state.

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
4. Read `docs/REPOSITORY_POLICY.md` before engine/content changes. Engine/content changes must not reduce M.U.G.E.N-style customization, and the Dragon sidecar format remains `.dragon.def` unless explicitly approved.
5. If adding Dragon-only behavior, update `docs/DRAGON_EXTENSIONS.md` in the same change.
6. If a planned item is accepted but not implemented immediately, materialize it in the matching roadmap/audit document. Content plans also need a matching M.U.G.E.N-style folder or data file so they cannot disappear as untracked chat notes.
7. If adding original benchmark characters, update `docs/BENCHMARK_CHARACTERS.md`. Do not list README-only reserved folders in `game/data/select.def`.

## Feature Work Contract

Feature work follows [FEATURE_COMPLETION_POLICY.md](FEATURE_COMPLETION_POLICY.md). Create or update a spec under `docs/FEATURE_SPECS/` before coding. Only one feature may be marked `Status: In Progress`.

The active recovery feature is [0001_architecture_recovery.md](FEATURE_SPECS/0001_architecture_recovery.md). Until that is complete, broad gameplay work should be limited to fixes needed to preserve current behavior while extracting modules.

The postmortem for the current architecture drift is [FAILURE_POSTMORTEM_2026_06_04.md](FAILURE_POSTMORTEM_2026_06_04.md). Before touching engine/app code, update the preservation record in [FEATURE_LEDGER.md](FEATURE_LEDGER.md), [REGRESSION_CHECKLIST.md](REGRESSION_CHECKLIST.md), or the active feature spec.
