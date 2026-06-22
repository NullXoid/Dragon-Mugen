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
- Six-board Story Stage Select smoke route using normal stage entries, including sewer, comic-street, and WAV-music board fixtures.
- Stage `[Music] bgmusic/bgvolume` parsing and WAV-backed stage BGM playback on a dedicated loop channel.
- Story default stage selection that prefers a converted OpenBOR-style scrolling stage, especially the TMNT OpenBOR Street fixture.
- One local player versus three reusable enemy runtime slots.
- Three enemy waves with `1`, `2`, then `3` active enemies, for six total enemy defeats.
- OpenBOR-style forward scrolling camera gates that advance with wave progress.
- Story depth/projection using the existing Arena depth systems so enemies can separate on the Z axis.
- Enemy targeting that sends all living enemies after P1 and keeps enemies from attacking each other.
- Enemy life, attack, and defence are scaled by Story difficulty only; P1 profile/character progression remains player-owned and does not become enemy level data.
- Player defeat, stage clear, result labels, and Dragon progression XP award on Story clear.
- Scripted verifiers for route, Story stage-select map routing, difficulty scaling, stage default, wave spawn/scrolling, enemy targeting, stage clear, player defeat, progression award, and Evil Ryu Story super recovery.

Out of scope:

- Full OpenBOR `.pak` execution or level-script runtime.
- Branching campaign, cutscenes, shops, tournament structure, hazards, platforms, pickups, equipment application, or persistent story-map progress.
- Team play, online play, or more than one local player in Story.
- Character-file edits or character-specific fixes for Leonardo, KFM, Evil Ryu, Evil Ken, or any other fighter.
- Applying Dragon level/item bonuses to live combat constants.

## Feature Slice

This is a playable Story Mode foundation, not broad OpenBOR compatibility. It proves that Dragon can route into a side-scrolling mode, load a player plus enemy runtimes, advance enemy waves, scroll a converted OpenBOR-style stage, resolve clear/fail results, and award profile-owned progression without changing M.U.G.E.N character files.

## Ownership

- `StoryModeTypes.h` owns Story state data.
- `StoryModeDifficulty.h` owns Story difficulty labels and tuning values.
- `StoryModeState.h` owns Story mode selection helpers, default enemy/stage choices, targeting helpers, and status text.
- `StoryModeSession.h` owns Story round reset, wave spawning, and scroller-gate setup.
- `StoryModeRuntime.h` owns Story fight-loop behavior: P1 input, enemy CPU targeting, hit routing, camera scroll, wave clear, stage clear, and player defeat.
- `StoryStageSelectOverlay.h/.cpp` owns Story-specific stage-select presentation. Shared stage selection mutation and routing stay in `FrontendFlow.h`.
- `MugenData.cpp` parses stage music metadata, and `AudioRuntime.h` owns the first WAV-only stage BGM loop channel.
- `RuntimeLoading.h` reuses the per-fighter Arena runtime vector for Story's player and enemy runtime bundles.
- `FrontendMenu.cpp`, `FrontendFlow.h`, and overlay preparation code provide thin routing/presentation integration only.
- `VerificationScenarioStoryMode.cpp` owns Story verifier coverage.

## Implementation Checklist

- [x] Add Story Mode to pending-mode state, main menu, frontend routing, character select, stage select, VS/loading, fight, and result paths.
- [x] Add Story-owned state and helper modules instead of placing the whole mode in `App.cpp`.
- [x] Add a Story-only map-style stage-select overlay without changing Training, Single Player, VS, or Arena stage-select behavior.
- [x] Add Story difficulty selection and keep enemy scaling separate from player progression/level display.
- [x] Expand the Story board route to six stage entries for map scrolling/selection coverage.
- [x] Add a first stage-music hook through normal `[Music] bgmusic` metadata and WAV loop playback.
- [x] Prefer converted OpenBOR-style stages for Story defaults.
- [x] Load full per-fighter runtimes for P1 and enemy slots.
- [x] Spawn three enemy waves with active slot counts `1`, `2`, and `3`.
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
- Confirm the Story Stage Select uses connected episode cards, highlights the selected stage, and keeps left/right/enter/escape behavior clear.
- Confirm Up/Down changes Story difficulty and enemy HUD labels show difficulty, not player level.
- Confirm the default stage is `TMNT OpenBOR Street`.
- Confirm Left/Right can reach `TMNT Sewer Patrol`, `Comic Street Rumble`, and `Soundcheck Alley`.
- Confirm `Soundcheck Alley` starts its background music after loading.
- Start the match and verify P1 fights one enemy, then two, then three.
- Walk right and confirm the camera scrolls forward but does not skip past the active wave gate.
- Confirm enemies chase P1 instead of fighting each other.
- Use Evil Ryu in Story and confirm supers consume visible meter, pause briefly as authored, hit, and return control without leaving Ryu stuck.
- Clear all waves and confirm `STAGE CLEAR`, match result options, and XP feedback.
- Lose the match and confirm `MISSION FAILED`.

Manual observations:

- 2026-06-22: User live-tested `Soundcheck Alley` and confirmed the configured stage background music plays.
- 2026-06-22: `Soundcheck Alley` loads and resolves failure/result flow with visible scrolling-board art while preserving the dedicated background-music test path.

## Done Means

- Story Mode is selectable and reaches a working side-scrolling fight.
- Story Stage Select presents the available stages as a small map/episode route and still routes through the normal VS/loading path.
- Story board expansion and the first WAV-backed stage BGM hook are covered by focused verification.
- The mode uses the loaded character/stage data and Dragon metadata; it does not require character-file edits.
- OpenBOR-style scrolling is present as a controlled converted-stage subset, not a claim of full OpenBOR compatibility.
- Wave spawning, enemy targeting, stage clear, player defeat, and XP award are covered by focused verifiers.
- Story difficulty and Story Evil Ryu super recovery are covered by focused verifiers.
- The feature is recorded in the unified roadmap, ledger, and regression checklist.
