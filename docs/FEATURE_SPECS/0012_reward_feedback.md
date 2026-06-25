# Reward Feedback

Status: Complete

## Goal

Make Dragon progression rewards visible where they are earned. Players should see gold and XP feedback during Story enemy defeats and on match-result screens, instead of discovering their balance only after entering the shop.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/FEATURE_LEDGER.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/FEATURE_SPECS/0011_arena_shop_hub.md`
- `engine/include/dragon/DragonProgression.h`
- `engine/src/DragonProgression.cpp`
- `engine/src/StoryModeRuntime.h`
- `engine/src/StoryModeState.h`
- `engine/src/StoryRewardFeedbackRuntime.h`
- `engine/src/AppRoundProgressionRuntime.h`
- `engine/src/VerificationScenarioDragonProgression.cpp`
- `engine/src/VerificationScenarioStoryMode.cpp`
- `game/data/dragon.def`

## Scope

In scope:

- Configurable win/loss/Arena win gold rewards.
- Configurable Story enemy defeat XP/gold rewards.
- Profile-owned gold persistence through `DragonProgression`.
- Match-result reward text that includes gold gained and current balance.
- Story enemy defeat feedback with floating `+XP +G` text.
- Story enemy defeat coin-burst visuals.
- Story status/result text that exposes the current named-profile gold balance.
- Guest behavior that remains non-persistent.
- Focused verifiers for progression, result text, and Story reward feedback.

Out of scope:

- Item drops.
- Collectible coin pickup physics.
- Loot tables.
- Character-file reward edits.
- Character-specific reward branches.
- Combat balance changes.

## Feature Slice

This lands as a complete economy-feedback slice. A named profile can earn gold from match wins and Story enemy defeats, see that reward immediately in Story as world feedback, see gold gained/current balance in result summaries, and spend that same balance in the shop. Guest can still play without persisting reward changes.

## Ownership

- `DragonProgression` owns XP, level, gold, inventory, equipment, profile persistence, and reward config parsing.
- `StoryModeRuntime` owns when Story enemy rewards are awarded.
- `StoryRewardFeedbackRuntime` owns transient floating reward and coin-burst visuals.
- `AppRoundProgressionRuntime` owns match-result award presentation.
- `StoryModeState` owns Story status/result reward text.
- Verifier changes belong in focused progression and Story verifier files, not broad UI or combat modules.
- `App.cpp` should remain a thin integration point only.

## Implementation Checklist

- [x] Add Dragon config values for win/loss/Arena gold rewards.
- [x] Persist awarded gold through named-profile `DragonProgression`.
- [x] Keep Guest rewards non-persistent.
- [x] Add result text that includes gained gold and current balance.
- [x] Add Story enemy defeat XP/gold feedback text.
- [x] Add Story enemy defeat coin-burst feedback.
- [x] Add Story status/result current balance text.
- [x] Ensure reward visuals do not affect hitboxes, collision, targeting, helpers, projectiles, or actor lifecycle.
- [x] Add verifier snapshot fields for reward feedback.
- [x] Add focused verifiers and update roadmap, ledger, and regression checklist records.

## Verification

Focused checks:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify dragon-progression-enemy-reward
build\dragon_mugen.exe --verify story-reward-feedback
build\dragon_mugen.exe --verify story-progression-award
```

Regression checks:

```powershell
build\dragon_mugen.exe --verify story-stage-clear
build\dragon_mugen.exe --verify story-wave-spawn-scroll
build\dragon_mugen.exe --verify story-scott-tram-rooftop
build\dragon_mugen.exe --verify arena-tmnt-openbor-stage
build\dragon_mugen.exe --verify shop-buy-sell-persistence
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify arena-cpu-2
build\dragon_mugen.exe --verify arena-cpu-3
build\dragon_mugen.exe --verify cpu-baseline
python engine\tools\dev_check.py . --skip-build
git diff --check
python tools\check_file_sizes.py
```

## Done Means

- Story enemy defeats visibly show earned XP/gold during gameplay.
- Story enemy defeats create non-gameplay coin-burst feedback.
- Story clear/fail/result paths show reward balance information for named profiles.
- Single Player, VS, Arena, and Story result summaries show gold gained and current balance when progression awards apply.
- Shop balance reflects the same profile-owned gold earned from combat.
- Guest remains safe to test without saved economy changes.
- Verifiers prove reward config, gold persistence, visible Story reward feedback, Story result awards, and related routing regressions.
