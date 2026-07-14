# Architecture Recovery

Status: In Progress

## Goal

Stop the current pattern where gameplay features are added in small app-layer slices and then need repeated repair. Recover the project shape by freezing `App.cpp` growth, documenting feature completion rules, and extracting owned modules before new broad gameplay work.

Architecture recovery is the active feature for the conservative cleanup slice described below. This slice removes proven internal dead code and unreachable presentation branches, strengthens warning and documentation gates, and preserves public APIs, M.U.G.E.N compatibility, content, gameplay, and save data.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/STRICT_ROADMAP.md`
- `docs/MODULE_SPLIT_PLAN.md`
- `docs/FEATURE_COMPLETION_POLICY.md`
- `docs/MEMORY_MODEL.md`
- M.U.G.E.N content layout under `game/`

## Scope

In scope:

- Add repo-level feature completion rules.
- Add feature spec validation.
- Freeze `App.cpp` growth.
- Extract existing behavior into owned modules without changing gameplay behavior.
- Keep KFM, Evil Ryu, and Evil Ken loading through `game/data/select.def`.
- Keep M.U.G.E.N-compatible backend files authoritative.

Out of scope:

- New moves, AI, shop, equipment, tournament, storyboard, networking, or editor work.
- Character-specific fixes that bypass CMD/CNS/AIR/SFF/SND data.

## Feature Slice

Architecture recovery must preserve playability and compatibility behavior already listed in `docs/FEATURE_LEDGER.md`. The active architecture slice is one complete subsystem extraction or one complete preservation/verification hardening pass with ownership, docs, and checks closed together.

Incremental commits may land inside that extraction or hardening pass, but they must not become loose app-layer slices or line-count-only movement. Engine/app code commits must update `docs/FEATURE_LEDGER.md`, `docs/REGRESSION_CHECKLIST.md`, or this spec in the same commit.

## Ownership

- Feature rules: `docs/FEATURE_COMPLETION_POLICY.md`
- Module recovery order: `docs/MODULE_SPLIT_PLAN.md`
- Validation: `engine/tools/check_feature_specs.py`
- Architecture hard stops: `engine/tools/guard_architecture.py`
- Active-change preservation guard: `engine/tools/guard_active_change.py`
- Runtime extraction targets: future modules listed in `docs/MODULE_SPLIT_PLAN.md`

## Implementation Checklist

- [x] Add feature completion policy.
- [x] Add feature spec folder and active architecture recovery spec.
- [x] Add feature spec validator.
- [x] Wire feature spec validation into `dev_check.py`.
- [x] Replace the `App.cpp` size gate with preservation documentation guards.
- [x] Add an active-change guard that blocks undocumented engine/app code commits.
- [x] Remove the audited zero-call internal functions and their orphaned helpers while keeping public APIs.
- [x] Remove obsolete per-controller fired-state collections while preserving generic runtime persistence.
- [x] Collapse fixed 640x360 presentation layouts and remove unreachable Classic/non-HD branches.
- [x] Make Dragon targets warning-clean under `/W4` or `-Wall -Wextra -Wpedantic` with opt-in warnings-as-errors.
- [x] Require both preservation documentation and an `Unreleased` changelog update for engine commits.
- [x] Make requested verifier fixtures fail setup when absent instead of substituting roster slot zero.
- [x] Record physical `App.cpp` lines, directly included implementation-shard count, shard lines, and aggregate lines.
- [x] Keep the Dragon presentation render target at native 1280x720 for every output profile so Classic/Wide/Extra cannot downsample UI text or introduce nested letterboxing.
- [x] Add an end-to-end SDL presentation verifier for all five output profiles and run it from the full development gate.
- [ ] Extract screen/mode flow from `App.cpp`.
- [ ] Extract fight session and round flow from `App.cpp`.
- [ ] Extract command buffering and CMD matching from `App.cpp`.
- [ ] Extract fighter runtime and CNS controller execution from `App.cpp`.
- [ ] Extract hit/guard/projectile/effect runtime from `App.cpp`.
- [ ] Extract training-only tools from `App.cpp`.
- [ ] Update the feature ledger and regression checklist after each meaningful extraction.

## Verification

Run:

```powershell
python engine/tools/dev_check.py . --skip-build
```

For larger extraction changes, also run from a Visual Studio developer shell:

```powershell
python engine/tools/dev_check.py .
```

The cleanup warning gate was validated with warnings-as-errors on a clean-first Ninja/GNU build and a fresh Visual Studio 2022/MSVC Debug build. Warning fixes must remain source-level and must not weaken the Dragon-only warning flags. Classic, Wide, Extra, SD 854x480, and HD 1280x720 remain separate selectable output profiles; the stable virtual layout preserves one composition across them. The shared presentation target remains 1280x720 so lower profiles cannot destroy Dragon UI detail before final display scaling. `video-resolution-presentation-e2e` initializes the real SDL Video Options screen, renders and reads back all five physical frames, verifies the production target and nonblank presentation, and compares a stable UI region across profiles; shared geometry assertions continue to verify SD/HD world and UI scaling.

Manual smoke path:

- Mode select opens.
- Training, Single Player, and VS reach character select.
- Character select loads only portraits.
- Stage select reaches VS screen.
- VS path loads selected fighter/stage runtime.
- Fight view still runs KFM, Evil Ryu, and Evil Ken through `select.def`.

## Done Means

- The cleanup slice removes at least 450 lines with no intended player-visible behavior change and no newly introduced verifier regression.
- Architecture reporting includes physical `App.cpp` lines and its directly included implementation shards; a small coordinator file alone does not count as modularization.
- Dragon targets compile without warnings under the strict validation configuration.
- Public APIs and supported M.U.G.E.N/Dragon compatibility paths remain available.
- New gameplay/system work no longer needs to add subsystem logic to `App.cpp`.
- The compatibility audit passes for the active roster.
- No feature work starts without a feature spec, source references, ownership, and verification criteria.
