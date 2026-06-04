# Architecture Recovery

Status: In Progress

## Goal

Stop the current pattern where gameplay features are added in small app-layer slices and then need repeated repair. Recover the project shape by freezing `App.cpp` growth, documenting feature completion rules, and extracting owned modules before new broad gameplay work.

## Source References

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

## Minimum Batch

Architecture recovery must preserve playability and compatibility behavior already listed in `docs/FEATURE_LEDGER.md`. The smallest acceptable batch is one complete subsystem extraction or one complete preservation/verification improvement. Engine/app code commits must update `docs/FEATURE_LEDGER.md`, `docs/REGRESSION_CHECKLIST.md`, or this spec in the same commit.

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

Manual smoke path:

- Mode select opens.
- Training, Single Player, and VS reach character select.
- Character select loads only portraits.
- Stage select reaches VS screen.
- VS path loads selected fighter/stage runtime.
- Fight view still runs KFM, Evil Ryu, and Evil Ken through `select.def`.

## Done Means

- `App.cpp` is below 9,000 lines.
- New gameplay/system work no longer needs to add subsystem logic to `App.cpp`.
- The compatibility audit passes for the active roster.
- No feature work starts without a feature spec, source references, ownership, and verification criteria.
