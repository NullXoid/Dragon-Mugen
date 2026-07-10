# Story Mode OpenBOR Side Scroller

Status: Complete

## Goal

Add a first playable Story Mode path that uses Dragon's existing M.U.G.E.N runtime and OpenBOR-style scrolling-stage metadata to run a side-scrolling enemy-wave fight.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/FEATURE_LEDGER.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/DRAGON_EXTENSIONS.md`
- `engine/src/StoryModeTypes.h`
- `engine/src/StoryModeDifficulty.h`
- `engine/src/StoryModeState.h`
- `engine/src/StoryModeSession.h`
- `engine/src/StoryModeRuntime.h`
- `engine/src/StoryStageSelectOverlay.h`
- `engine/src/StoryStageSelectOverlay.cpp`
- `engine/src/VerificationScenarioStoryMode.cpp`
- `game/data/select.def`
- `game/stages/tmnt_openbor_street.def`

## Scope

In scope:

- Main-menu Story Mode route into character select, stage select, VS/loading, fight, and match-result screens.
- Story-only comic-map stage select presentation with connected episode cards and a selected mission panel.
- Story-owned difficulty selection on the map: `EASY`, `MEDIUM`, and `HARD`.
- Configurable Story board routes from `game/data/story_boards.def`, with parent boards shown on Stage Select and internal side-scroller, mid-boss, shop, and arena-boss route nodes hidden from direct selection.
- Generic enemy-role setup (`grunts`, `mini_bosses`, and `bosses`) so boards reference editable roles instead of hardcoding KFM/Ken/Ryu-style fixtures in runtime code.
- Stage `[Music] bgmusic/bgvolume` parsing and WAV-backed stage BGM playback on a dedicated loop channel.
- Story default stage selection that prefers a converted OpenBOR-style scrolling stage, especially the TMNT OpenBOR Street fixture.
- One local player versus three reusable enemy runtime slots.
- Difficulty-derived wave plans: `EASY` runs one boss wave, `MEDIUM` runs grunt, mid-boss, and boss waves, and `HARD` runs five waves with mid-boss checkpoints before the final boss.
- A route shop-door stop that can pause Story after a cleared board, enter the existing Shop Hub, and resume the next route fight when the player exits the shop.
- OpenBOR-style forward scrolling camera gates that advance with wave progress.
- Story depth/projection using the existing Arena depth systems so enemies can separate on the Z axis.
- Enemy targeting that sends all living enemies after P1 and keeps enemies from attacking each other.
- Enemy life, attack, and defence are scaled by Story difficulty only; P1 profile/character progression remains player-owned and does not become enemy level data.
- Player defeat, stage clear, result labels, and Dragon progression XP award on Story clear.
- Scripted verifiers for route, Story stage-select map routing, difficulty scaling, stage default, wave spawn/scrolling, enemy targeting, stage clear, player defeat, progression award, and Evil Ryu Story super recovery.

Out of scope:

- Full OpenBOR `.pak` execution or level-script runtime.
- Branching campaign, cutscenes, multiple shops, tournament structure, hazards, platforms, pickups, equipment application to combat, or persistent story-map progress.
- Team play, online play, or more than one local player in Story.
- Character-file edits or character-specific fixes for Leonardo, KFM, Evil Ryu, Evil Ken, or any other fighter.
- Applying Dragon level/item bonuses to live combat constants.

## Feature Slice

This is a playable Story Mode foundation, not broad OpenBOR compatibility. It proves that Dragon can route into a side-scrolling mode, load a player plus enemy runtimes, advance enemy waves, scroll a converted OpenBOR-style stage, resolve clear/fail results, and award profile-owned progression without changing M.U.G.E.N character files.

## Ownership

- `StoryModeTypes.h` owns Story state data.
- `StoryBoardPlan.h` owns editable Story board config parsing, parent/segment route expansion, enemy-role setup, wave overrides, rewards, and route cue metadata.
- `StoryModeDifficulty.h` owns Story difficulty labels and tuning values.
- `StoryModeState.h` owns Story mode selection helpers, route node selection, default enemy/stage choices, targeting helpers, and status text.
- `StoryModeSession.h` owns Story round reset, wave spawning, and scroller-gate setup.
- `StoryModeRuntime.h` owns Story fight-loop behavior: P1 input, enemy CPU targeting, hit routing, camera scroll, wave clear, route shop-door availability, stage clear, and player defeat.
- `StoryStageSelectOverlay.h/.cpp` owns Story-specific stage-select presentation. Shared stage selection mutation and routing stay in `FrontendFlow.h`.
- `AppMainLoopAssembly.h` and `ShopDemoRuntime.h` own the Story route shop handoff/resume path without moving inventory or shop-economy rules into Story code.
- `MugenData.cpp` parses stage music metadata, and `AudioRuntime.h` owns the first WAV-only stage BGM loop channel.
- `RuntimeLoading.h` reuses the per-fighter Arena runtime vector for Story's player and enemy runtime bundles.
- `FrontendMenu.cpp`, `FrontendFlow.h`, and overlay preparation code provide thin routing/presentation integration only.
- `VerificationScenarioStoryMode.cpp` owns Story verifier coverage.

## Implementation Checklist

- [x] Add Story Mode to pending-mode state, main menu, frontend routing, character select, stage select, VS/loading, fight, and result paths.
- [x] Add Story-owned state and helper modules instead of placing the whole mode in `App.cpp`.
- [x] Add a Story-only map-style stage-select overlay without changing Training, Single Player, VS, or Arena stage-select behavior.
- [x] Add Story difficulty selection and keep enemy scaling separate from player progression/level display.
- [x] Expand Story boards to a parent route with hidden internal side-scroller, mid-boss, shop, and arena-boss nodes.
- [x] Add editable enemy-role setup for `grunts`, `mini_bosses`, and `bosses` in `game/data/story_boards.def`.
- [x] Add a first stage-music hook through normal `[Music] bgmusic` metadata and WAV loop playback.
- [x] Prefer converted OpenBOR-style stages for Story defaults.
- [x] Load full per-fighter runtimes for P1 and enemy slots.
- [x] Spawn difficulty-owned wave plans: one boss wave on `EASY`, three waves with a mid-boss on `MEDIUM`, and five waves with two mid-boss checkpoints on `HARD`.
- [x] Add a Story route shop-door stop that enters Shop Hub and resumes the next route fight on exit.
- [x] Keep inactive future-wave enemy slots out of rendering and combat.
- [x] Scroll OpenBOR-style stages forward with wave gates and clamp the player to the current playable segment.
- [x] Route enemies toward P1 and prevent enemy-vs-enemy targeting.
- [x] Route player hits to living enemies and enemy/helper/projectile hits to P1.
- [x] Resolve Story clear/fail states into match result presentation.
- [x] Award Dragon progression XP on Story clear.
- [x] Verify Evil Ryu Story supers recover after superpause/helper hit runtime without leaving P1 stuck.
- [x] Add focused Story verifiers and update roadmap/ledger/checklist records.

## Verification

Focused checks:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify story-mode-menu-route
build\dragon_mugen.exe --verify story-stage-select-map
build\dragon_mugen.exe --verify story-difficulty-enemy-scaling
build\dragon_mugen.exe --verify story-board-route-plan
build\dragon_mugen.exe --verify story-shop-door-trigger
build\dragon_mugen.exe --verify story-shop-route-resume
build\dragon_mugen.exe --verify story-openbor-stage-default
build\dragon_mugen.exe --verify story-stage-board-expansion
build\dragon_mugen.exe --verify story-wave-spawn-scroll
build\dragon_mugen.exe --verify story-enemy-targeting
build\dragon_mugen.exe --verify story-stage-clear
build\dragon_mugen.exe --verify story-player-defeat
build\dragon_mugen.exe --verify story-progression-award
build\dragon_mugen.exe --verify story-evilryu-super-recovery
```

Regression checks:

```powershell
build\dragon_mugen.exe --verify arena-tmnt-openbor-stage
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify dragon-progression-player-profiles
build\dragon_mugen.exe --verify cpu-baseline
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
git diff --check
python tools\check_file_sizes.py
```

Manual smoke:

- Open Story Mode from the main menu.
- Select Leonardo or another roster character.
- Confirm the Story Stage Select shows the parent board as the selectable unit while internal route nodes stay hidden from direct selection.
- Confirm Up/Down changes Story difficulty and enemy HUD labels show difficulty, not player level.
- Confirm the default stage is `TMNT OpenBOR Street`.
- Confirm configured board roles drive the visible enemy mix: regular enemies, mini/midbosses, and bosses come from `[Enemy Setup]`.
- Confirm `Soundcheck Alley` starts its background music after loading.
- Start the match and verify `EASY` reaches a single boss wave, `MEDIUM` routes through a mid-boss then a boss, and `HARD` routes through five waves with mid-boss checkpoints before the boss.
- Walk right and confirm the camera scrolls forward but does not skip past the active wave gate.
- Clear a board that leads to a shop node, confirm the route shop-door cue appears, enter it, exit the Shop Hub, and confirm Story resumes at the next playable route node.
- Confirm enemies chase P1 instead of fighting each other.
- Use Evil Ryu in Story and confirm supers consume visible meter, pause briefly as authored, hit, and return control without leaving Ryu stuck.
- Clear all waves and confirm `STAGE CLEAR`, match result options, and XP feedback.
- Lose the match and confirm `MISSION FAILED`.

Manual observations:

- 2026-06-22: User live-tested `Soundcheck Alley` and confirmed the configured stage background music plays.
- 2026-06-22: `Soundcheck Alley` loads and resolves failure/result flow with visible scrolling-board art while preserving the dedicated background-music test path.

## Done Means

- Story Mode is selectable and reaches a working side-scrolling fight.
- Story Stage Select presents parent boards and keeps internal route segments data-driven and hidden from direct selection.
- Story board expansion, enemy role setup, shop-door route resume, and the first WAV-backed stage BGM hook are covered by focused verification.
- The mode uses the loaded character/stage data and Dragon metadata; it does not require character-file edits.
- OpenBOR-style scrolling is present as a controlled converted-stage subset, not a claim of full OpenBOR compatibility.
- Wave spawning, enemy targeting, difficulty wave plans, stage clear, player defeat, route shop stop/resume, and XP award are covered by focused verifiers.
- Story difficulty and Story Evil Ryu super recovery are covered by focused verifiers.
- The feature is recorded in the unified roadmap, ledger, and regression checklist.
