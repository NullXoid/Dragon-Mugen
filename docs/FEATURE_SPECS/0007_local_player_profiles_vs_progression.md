# Local Player Profiles And VS Progression

Status: Complete

## Goal

Make Dragon progression user-owned for local play by adding P1/P2 profile slots, a non-persistent Guest slot, profile creation/selection from Options, and VS match XP awards for both non-Guest human profiles.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/FEATURE_SPECS/0005_dragon_progression_leveling_items.md`
- `docs/FEATURE_SPECS/0006_dragon_profile_progression_display.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/FEATURE_LEDGER.md`
- `game/data/dragon.def`
- `game/save/progression.def`
- `engine/include/dragon/DragonProgression.h`
- `engine/src/DragonProgression.cpp`
- `engine/src/DragonProgressionProfile.cpp`

## Scope

In scope:

- P1 and P2 progression profile slots stored in `game/save/progression.def`.
- P2 defaulting to Guest so a second local player is suggested but not forced to create a profile.
- Real profile uniqueness: P1 and P2 cannot use the same persistent profile at the same time.
- Options rows for `P1 Profile` and `P2 Profile` with left/right cycling and Enter-created profiles.
- Character Select and Fight HUD LV/XP labels sourced from the selected player profile slot.
- VS Mode match-result XP awards for both local non-Guest profiles using each player's selected character.
- Single Player and Arena retaining P1-only progression awards.
- Guest profile behavior: no saved XP, no saved inventory/items, and no Guest profile section written to disk.
- Scripted verifier coverage for Guest rules, profile uniqueness, per-profile character XP, and save round-trip.

Out of scope:

- A full typed username/profile editor.
- Login, online accounts, cloud saves, or multi-machine profile sync.
- Shop, campaign, tournament, equipment-management UI, or item-use screens.
- Applying progression stat bonuses to live M.U.G.E.N combat constants by default.
- Character-specific progression branches.

## Feature Slice

This slice completes the local-player identity layer that progression needs before shop/campaign work: one real user per persistent profile, Guest for casual P2 play, per-profile character XP/item ownership, and match-result XP for each non-Guest local VS player.

## Ownership

- `DragonProgression` owns profile IDs, Guest policy, save/load round-trip, per-profile XP/items/equipment APIs, and award calculation.
- `DragonProgressionProfile.cpp` owns profile selection, creation, active/P1/P2 slot helpers, uniqueness, and display names.
- `App.cpp` only wires Options actions, prepared labels, and match-result award calls into existing screen flow.
- `CharacterSelectOverlay`, `FightHudOverlay`, and `FightResultOverlay` render prepared text only.
- Verification lives in `VerificationScenarioDragonProgression.cpp`.

## Implementation Checklist

- [x] Add P1/P2 profile slots to progression save data.
- [x] Add reserved Guest profile ID/name and non-persistent Guest behavior.
- [x] Add create/select/cycle helpers with duplicate-profile protection.
- [x] Extend save/load format with `p1.profile` and `p2.profile`.
- [x] Keep legacy flat-save migration into P1's profile.
- [x] Add Options rows for P1/P2 profile selection and Enter-to-create.
- [x] Show P1/P2 profile and LV/XP labels in Character Select.
- [x] Show P1/P2 LV/XP labels in the Fight HUD when P2 is a local real profile.
- [x] Award VS XP to both local non-Guest players using each selected character.
- [x] Keep Single Player and Arena P1-only progression awards.
- [x] Add `dragon-progression-player-profiles` verifier.
- [x] Update roadmap, regression checklist, and feature ledger.

## Verification

Focused checks:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify dragon-progression-player-profiles
build\dragon_mugen.exe --verify dragon-progression-level-items
build\dragon_mugen.exe --verify classic-fight-outcomes
build\dragon_mugen.exe --verify vs-p2-runtime
python engine\tools\check_feature_specs.py .
git diff --check
```

Broader checks:

```powershell
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
python tools\check_file_sizes.py
```

Manual smoke:

- Open Options and confirm P1 Profile/P2 Profile rows are visible.
- Confirm P2 starts as Guest and can cycle to a real profile after one is created.
- Confirm Enter on a profile row creates/selects a new profile.
- Confirm P1 and P2 cannot select the same real profile.
- Play a VS match with P2 Guest and confirm only P1 persists XP.
- Play a VS match with P2 on a real profile and confirm both profiles receive per-character XP.

## Done Means

- XP/LV and items are owned by a profile plus character/item, not globally by character alone.
- P2 can play casually as Guest without creating persistent data.
- Two local users can use separate profiles and earn XP independently in VS.
- Single Player and Arena progression behavior remains unchanged as P1-only.
- The feature is documented in the unified roadmap/checklist/ledger and covered by a focused verifier.
