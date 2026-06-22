# Dragon Profile Progression Display

Status: Complete

## Goal

Make Dragon progression user-owned and visible by storing XP under an active local player profile, migrating the old flat character save data, and displaying profile LV/XP in the game UI without changing M.U.G.E.N-authored combat behavior.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/FEATURE_SPECS/0005_dragon_progression_leveling_items.md`
- `game/data/dragon.def`
- `game/save/progression.def`

## Scope

- Add active profile ownership to Dragon progression saves.
- Preserve existing flat `[Character]` and `[Inventory]` saves through automatic migration into the active profile.
- Keep existing progression APIs usable by routing character XP, inventory, equipment, awards, and effective stats through the active profile.
- Show the active profile and selected fighter LV/XP on character select.
- Show compact P1 LV/XP in the fight HUD.
- Keep match-result XP feedback and save persistence.

Out of scope:

- Manual profile creation UI.
- Shop, campaign, tournament, equipment-management, or item-use screens.
- Applying progression stat bonuses to live M.U.G.E.N combat constants by default.
- Replacing character CMD/CNS/AIR/SFF/SND data with Dragon progression logic.

## Feature Slice

This slice completes the immediate player-facing progression loop: a local user profile owns the XP, old saves are preserved, the selected fighter's current level and XP are visible before and during play, and match results still award XP once.

## Ownership

- `DragonProgression` owns profile save data, migration, award recording, effective stats, and LV/XP summary formatting.
- `App.cpp` only assembles progression strings for existing views.
- `CharacterSelectOverlay` and `FightHudOverlay` only render prepared profile/LV/XP labels.
- Verification stays under `VerificationScenarioDragonProgression.cpp`.

## Implementation Checklist

- [x] Add profile-aware progression save structures.
- [x] Add default active profile resolution from local user environment.
- [x] Migrate old flat character/inventory saves into the active profile.
- [x] Keep public award, inventory, equipment, and effective-stat APIs active-profile aware.
- [x] Add reusable LV/XP summary formatting.
- [x] Display profile and selected fighter LV/XP on character select.
- [x] Display compact P1 LV/XP in the fight HUD.
- [x] Extend progression verifier coverage for profile isolation, display summary, save round-trip, and legacy migration.

## Verification

Required focused checks:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify dragon-progression-level-items
python engine\tools\dev_check.py . --skip-build
python engine\tools\check_feature_specs.py .
git diff --check
```

Broader regression:

```powershell
build\dragon_mugen.exe --verify classic-fight-outcomes
build\dragon_mugen.exe --verify arena-cpu-1
```

## Done Means

- XP is owned by the active local profile rather than a global character row.
- Existing flat saves keep their XP and inventory after loading.
- Character select and fight HUD expose LV/XP without changing combat constants.
- `dragon-progression-level-items` verifies profile isolation and migration.
