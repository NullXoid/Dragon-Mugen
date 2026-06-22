# Dragon Progression Leveling And Items

Status: Complete

## Goal

Add a Flying-Dragon-inspired Dragon progression foundation that makes characters earn XP/levels and use item inventory/equipment without changing base M.U.G.E.N character files or silently modifying classic compatibility behavior.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/DRAGON_EXTENSIONS.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/FEATURE_LEDGER.md`
- `game/data/dragon.def`
- `game/save/README.md`
- `engine/include/dragon/DragonProgression.h`
- `engine/src/DragonProgression.cpp`
- `engine/src/ProgressionState.h`

## Scope

In scope:

- Dragon-only project defaults for progression in `game/data/dragon.def`.
- Local player progression save data in `game/save/progression.def`.
- Character XP, level curves, victories, defeats, item inventory, equipment slots, and effective stat-bonus calculation.
- One-time match-result XP awards for the local P1 character in match modes.
- A compact match-result progression summary line.
- Scripted verifier coverage for data loading, XP awards, level-up behavior, item grant/equip rules, stat calculation, and save round-trip.

Out of scope:

- Full campaign, story, tournament, shop, or equipment-management UI.
- Applying progression stat bonuses to live M.U.G.E.N combat constants by default.
- Character-specific branches for KFM, Evil Ken, Evil Ryu, Leonardo, or downloaded characters.
- Replacing CMD/CNS/AIR/SFF/SND behavior with Dragon progression logic.

## Feature Slice

This slice adds the reusable progression spine: data, save, XP, items, equipment, effective stats, result-screen award feedback, and verification. It deliberately does not apply progression bonuses to live combat yet; a later Dragon mode/shop slice can opt into those computed bonuses without disturbing Training, VS, Single Player compatibility tests, or M.U.G.E.N-authored constants.

## Ownership

- Progression rules and persistence are owned by `DragonProgression`.
- Runtime session state is owned by `ProgressionState`.
- Project defaults live in `game/data/dragon.def`.
- Local mutable save data lives in `game/save/progression.def`.
- Match-result display receives prepared progression text only; `FightResultOverlay` does not own progression logic.
- Verification lives in `VerificationScenarioDragonProgression.cpp`.

## Implementation Checklist

- [x] Add Dragon progression data structures and APIs.
- [x] Parse Dragon progression defaults from `game/data/dragon.def`.
- [x] Load and save local progression from `game/save/progression.def`.
- [x] Support XP awards, level curves, wins/losses, item inventory, equipment slots, and effective stat bonuses.
- [x] Award match-result XP once per completed match for the local P1 character.
- [x] Show compact progression feedback on the match-result screen.
- [x] Add `dragon-progression-level-items` verifier.
- [x] Run the focused build and regression checks.
- [x] Mark this spec complete after verification passes.

## Verification

Run:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify dragon-progression-level-items
build\dragon_mugen.exe --verify classic-fight-outcomes
build\dragon_mugen.exe --verify classic-fight-routing
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify roster-compatibility-smoke
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
python engine\tools\check_feature_specs.py .
git diff --check
python tools\check_file_sizes.py
```

Manual smoke:

- Win or lose a Single Player/VS/Arena match as P1.
- Confirm the match-result screen shows one progression summary line.
- Confirm `game/save/progression.def` is created or updated.
- Restart the app and confirm the save remains readable.
- Confirm Training behavior and command compatibility remain unchanged.

## Done Means

- Progression data is editable and Dragon-owned.
- Local progression persists without modifying M.U.G.E.N content files.
- Match completion can award XP and show the result once.
- Item inventory/equipment and effective stat-bonus calculations are available for future shop/campaign work.
- Existing fight/roster/Arena checks stay green.
