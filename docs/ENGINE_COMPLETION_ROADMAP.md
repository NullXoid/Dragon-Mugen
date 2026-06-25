# Engine Completion Roadmap

This is the single planning source for completing Dragon MUGEN as a playable, editable, M.U.G.E.N-style fighting engine. Older roadmap and backlog files now point here. Operational records remain separate: `FEATURE_LEDGER.md` preserves what already works, and `REGRESSION_CHECKLIST.md` defines what must be rerun.

## Operating Rules

These rules apply before planning or implementing any new engine feature:

- Single roadmap authority: this file is the unified roadmap. Feature specs under `docs/FEATURE_SPECS/` are active work contracts for roadmap phases, not competing plans.
- Full feature implementation: plan and implement complete feature slices with runtime behavior, UI/options where needed, compatibility rules, documentation, and verification. Avoid baby-step or symptom-only patches that do not close a named capability.
- Meaningful progress over micro-adjustments: incremental commits are allowed, but they must move the same accepted feature slice toward its done criteria. Do not replace a full feature with a tiny verifier-only or cosmetic-only slice unless that slice explicitly closes a roadmap item.
- Module-first implementation: new gameplay, UI, verifier, or runtime systems belong in focused modules/categories with thin integration. Do not grow `engine/src/App.cpp` as a catch-all. If the correct boundary does not exist, create or extend the boundary as part of the feature.
- Context-refresh reminder: after every major context handoff, compaction, or roughly every 100k tokens of project work, re-read this section before planning. The next plan or status update should restate these rules briefly so they do not fall out of context. This is a documented handoff rule, not an automated token monitor.

## Direction

- Engine: C++20 or newer, CMake, SDL3 platform layer, bgfx renderer direction.
- Runtime content root: `game/`, using M.U.G.E.N-style folders and names.
- Roster and stage authority: `game/data/select.def`.
- Character and stage source of truth: editable M.U.G.E.N files first; Dragon `.dragon.def` sidecars only extend behavior.
- Public-release requirement: replace official M.U.G.E.N and third-party assets with original or properly licensed content.
- Feature process: one active feature spec under `docs/FEATURE_SPECS/`; each spec must define a complete feature slice and ownership boundary.

Non-goals before the core engine is stable:

- No networking.
- No tournament or broad equipment-management/campaign economy mode as active work beyond the accepted Dragon progression, Story foundation, and first Arena-style shop hub. Future shop/economy expansion should still land as complete feature slices.
- No editor runtime pivot.
- No hardcoded fixes for individual downloaded characters.
- No JSON character/stage runtime sidecars replacing `.dragon.def`.

## Current State

The engine already has a working local play loop:

- Training, Single Player, VS Mode, DLC Arena Mode, and Story Mode are present as separate paths.
- Character and stage selection route through `select.def`.
- KFM, Evil Ken, Evil Ryu, and local Lili compatibility fixtures load through selected character DEF files.
- SFF v1 PCX, SFF v2 PNG/palette stage sprites, ACT, AIR, CMD, CNS, SND, stage DEF/SFF, basic animated stage backgrounds, fight data, and common fight FX are partially supported.
- Training includes command HUD, input history, command list, Show Me, pause/capture controls, controller prompts, palette separation, and command completion feedback.
- Options now uses Gameplay, Video, and Controls categories. Controls provide Player 1-4 setup, keyboard/controller setup, live input test, restore defaults, action presets, conflict/missing-action warnings, per-profile persistence, and canonical action mapping into the existing `FighterInputState`.
- Arena includes FFA CPU counts, per-fighter runtimes, Z-axis movement, sidestep, depth-aware hit/push/draw order, camera yaw, and a simple OpenBOR-style scroll-stage pass. OpenBOR Stage Compatibility v2 remains the active broader conversion/import feature spec; shared VS/Arena/Story loading progress and the first 4-fighter performance diagnostics/culling gate are implemented.
- Story Mode has a first playable OpenBOR-style side-scrolling foundation: one local player, a Story-only map-style stage select with six board entries, `EASY`/`MEDIUM`/`HARD` enemy difficulty selection, three reusable enemy runtime slots, three waves, forward scrolling gates, enemy targeting against P1, stage clear/fail result presentation, Dragon progression XP/gold awards with live reward feedback, Story-specific enemy scaling that does not reuse player level data, Evil Ryu Story super recovery coverage, and stage BGM support for WAV/MP3/OGG through SDL_mixer.
- A local-only external content registry can mount private third-party test packages outside `game/`; the first proof mounts Scott Pilgrim Versus `Tram_Rooftop` as a Story-compatible board with SFF v2 PNG/palette animated backgrounds and MP3 stage music when the local package exists.
- Single Player and VS have round/match presentation, pause, result, rematch, local P2 runtime support, and a Dragon-only progression foundation with P1/P2 local profile slots, Guest P2 support, per-profile character/item ownership, P1-only Single Player/Arena awards, and VS awards for both non-Guest local players without changing base M.U.G.E.N character files.
- A modular performance diagnostics layer tracks frame timing, fixed-step pressure, workload counts, pause/hitpause/superpause separation, PERF/FPS/OFF HUD modes, optional local perf logs under `artifacts/perf/`, and safe render-only culling for stage/actor/effect drawing.
- Scripted verifiers cover the selectable roster smoke gate, classic fight outcomes/routing/combat, Arena runtime, Story runtime, Training command systems, performance stress paths, and CPU baseline.

The engine is not complete yet. The main remaining work is fight-rule correctness, deeper M.U.G.E.N compatibility, Training completion, Arena polish, persistent input/settings, presentation, and architecture recovery.

## Completion Phases

### 1. Fight Correctness

Goal: make classic Training, Single Player, and VS fight outcomes reliable before building higher modes.

Completed feature spec: [FEATURE_SPECS/0002_fight_correctness.md](FEATURE_SPECS/0002_fight_correctness.md).

Key work:

- Verify P1 KO, P2 KO, double-KO, draw, time-over, round replay, round pips, match result, rematch, restart, and menu-return behavior.
- Verify pause actions beyond Resume in Single Player and VS.
- Finish guard, guard damage, guard spark/sound, knockdown, fall recovery, air recovery, wake-up, and defeated-state handling for KFM, Evil Ken, and Evil Ryu.
- Keep hit damage, power gates, power consumption, and fighter constants generic from loaded character data.

Done when:

- Dedicated verifiers or recorded live passes cover KO, double-KO, draw, time-over, pause actions, guard, fall recovery, and match-result routing.
- Existing KFM, Evil Ken, Evil Ryu, CPU, Training, VS, and Arena verifiers stay green.

### 2. M.U.G.E.N Compatibility Depth

Goal: expand the runtime subset so supported characters behave because their own CMD/CNS/AIR/SFF/SND files say so.

Completed feature specs:

- [FEATURE_SPECS/0003_roster_compatibility_readiness.md](FEATURE_SPECS/0003_roster_compatibility_readiness.md)
- [FEATURE_SPECS/0010_scott_pilgrim_stage_compatibility.md](FEATURE_SPECS/0010_scott_pilgrim_stage_compatibility.md)

Key work:

- Keep `roster-compatibility-smoke` green for every character listed in `game/data/select.def` before deeper new-character testing.
- Keep SFF v1 PCX and SFF v2 PNG/palette sprite decode compatible for stages and future character infrastructure.
- Keep stage BGM decoding generic for WAV, MP3, and OGG paths from normal stage `[Music] bgmusic` metadata.
- Keep IKEMEN-only `select.def` metadata tolerant without treating slot blocks as roster entries.
- Expand trigger and expression support: variables, ranges, random branches, redirection, `enemynear`, `helper(...)`, `target`, `p2bodydist`, `p2stateno`, `numtarget`, `AnimElemTime`, edge distance, and richer `roundstate`/`power` forms.
- Improve controller parity: target controllers, throws/cinematics, helper lifecycle, projectile edge cases, explod ownership, pause/superpause, palette effects, `ScreenBound movecamera`, and expression-backed controller values.
- Keep unsupported behavior documented in `COMPATIBILITY_AUDIT.md`.
- Rerun `python engine/tools/audit_mugen_compat.py game` after every compatibility expansion.

Done when:

- KFM, Evil Ken, and Evil Ryu load without unexpected unsupported-controller failures.
- Every selectable roster entry passes the generic load/fight/idle/runtime/movement smoke before move-specific audits begin.
- The active compatibility warnings are understood, documented, and either acceptable or filed as targeted work.
- No character-specific engine branches are needed for the supported roster.

### 3. Training Completion

Goal: turn the current Training tools into a reliable command-learning and debugging mode.

Key work:

- Formalize command-training flow: move ordering, objective states, success/fail tracking, reset behavior, Show Me behavior, and filtered move categories.
- Verify Training options broadly: dummy guard/freeze, P2 control, power mode, guard damage, command HUD, input HUD, move list, reset, and hitbox/debug toggles.
- Keep the live HUD actionable and lightweight; explanatory legend stays in pause/help.
- Preserve facing-aware physical command display and controller-specific prompts.

Done when:

- Command completion works across All, Normal, Special, Super, throw, air, and contextual move categories for the active roster.
- Training options have scripted or live verification.
- The command HUD and full command list remain readable at supported logical widths.

### 4. Arena Completion

Goal: finish Arena as a DLC-only 2.5D FFA mode without affecting classic modes.

Planned feature spec: [FEATURE_SPECS/0004_openbor_stage_compatibility_v2.md](FEATURE_SPECS/0004_openbor_stage_compatibility_v2.md).

Key work:

- Rerun manual Arena setup/navigation: Character Select, CPU count 1/2/3, CPU slots, stage, timer, Z Axis, Camera Rotate, Start Match, Back, and result routing.
- Improve CPU behavior after fight correctness is stable, still using generic character runtime data.
- Expand OpenBOR-style scrolling stages only through documented Arena/DLC metadata.
- Keep the shared VS/Arena/Story loading progress bar backed by staged loading phases, and extend phase detail when more loader work is split out.
- Keep the 4-fighter Arena performance gate for converted OpenBOR-style stages green, including frame-time/FPS telemetry and stress coverage for fighters, helpers, projectiles, effects, and scrolling backgrounds.
- Define OpenBOR Stage Compatibility v2 around import/conversion, scroll metadata, camera bounds, actor projection, and performance budgets before claiming broader OpenBOR support.
- Keep Z-axis, camera yaw, depth hit gating, depth push, draw order, and defeated-fighter handling Arena-only.

Done when:

- Arena scripted verifiers and a current manual GUI pass cover setup, runtime, pause/result routing, and Z/camera behavior.
- The VS/Arena/Story load screen exposes actual staged progress for character, stage, sprite, sound, and runtime preparation.
- A 4-fighter Arena match on the converted TMNT OpenBOR Street fixture has measured frame-time/FPS evidence, safe culling preservation checks, and no known sustained 4-player slowdown left untriaged.
- Training, Single Player, and VS are unaffected when Arena Z/camera/scroller features are enabled in Arena.

### 5. Input And Settings

Goal: make physical controls and user preferences reliable and persistent.

Completed feature spec: [FEATURE_SPECS/0008_controls_submenu_unified_options.md](FEATURE_SPECS/0008_controls_submenu_unified_options.md).

Key work:

- Keep keyboard and controller rebinding routed through the shared category Options UI.
- Keep controls saved per Dragon profile under the user-save root, with Guest using defaults.
- Keep automatic controller prompt detection, prompt style overrides, Start/Select behavior, touchpad `ST/Taunt`, and command-buffer routing consistent across modes.
- Maintain physical keyboard and controller smoke tests for movement, diagonals, commands, pause, result navigation, and command training.

Done when:

- Rebinding survives app restart for named profiles and Guest remains non-persistent.
- Keyboard, Xbox-style, and PlayStation-style prompts map to the same canonical action IDs.
- Physical input smoke confirms keyboard, Xbox-style, and PlayStation-style controllers feed the command buffer, pause routing, Training shortcuts, and result navigation.

### 6. Presentation And Content Readiness

Goal: replace prototype presentation with data-driven fight UI and release-safe content.

Key work:

- Move lifebar, powerbar, combo, round, KO, time-over, and fight callout presentation toward `game/data/fight.def`, `fight.sff`, fonts, and sound metadata.
- Improve font readability and UI consistency without hiding M.U.G.E.N data ownership.
- Add shared UI/music/storyboard support only after the image storyboard path is stable.
- Track all Dragon-only presentation behavior in `DRAGON_EXTENSIONS.md`.
- Prepare original or licensed replacement assets for any public release.

Done when:

- Fight UI reads the intended M.U.G.E.N data sources before falling back to Dragon defaults.
- Public-release content risks are removed or documented as private-only.

### 7. Architecture Recovery

Goal: stop feature work from concentrating in oversized app-layer files.

Key work:

- Continue extracting owned modules from `App.cpp`: screen flow, fight/round flow, command buffering, fighter runtime, CNS execution, hit/guard/projectile/effect runtime, and Training tools.
- Split oversized verifier files by scenario family.
- Reduce or justify all files over the hard file-size threshold.
- Keep engine/app code commits paired with `FEATURE_LEDGER.md`, `REGRESSION_CHECKLIST.md`, or the active feature spec.

Done when:

- `App.cpp` is below the active recovery target and no longer owns subsystem logic.
- `python tools/check_file_sizes.py` passes or only reports explicitly justified exceptions.
- Broad gameplay work can be implemented in owned modules instead of app-layer patches.

### 8. Future Modes

Goal: add higher-level game modes only after the engine core can support them.

Completed feature specs:

- [FEATURE_SPECS/0005_dragon_progression_leveling_items.md](FEATURE_SPECS/0005_dragon_progression_leveling_items.md)
- [FEATURE_SPECS/0006_dragon_profile_progression_display.md](FEATURE_SPECS/0006_dragon_profile_progression_display.md)
- [FEATURE_SPECS/0007_local_player_profiles_vs_progression.md](FEATURE_SPECS/0007_local_player_profiles_vs_progression.md)
- [FEATURE_SPECS/0009_story_mode_openbor_side_scroller.md](FEATURE_SPECS/0009_story_mode_openbor_side_scroller.md)
- [FEATURE_SPECS/0011_arena_shop_hub.md](FEATURE_SPECS/0011_arena_shop_hub.md)
- [FEATURE_SPECS/0012_reward_feedback.md](FEATURE_SPECS/0012_reward_feedback.md)

Accepted foundation work:

- Dragon progression data/save support for character XP, levels, item inventory, equipment slots, and computed stat bonuses.
- Profile-owned local player XP with legacy flat-save migration.
- P1/P2 local profile slots with Guest as the default non-persistent P2 profile and duplicate real-profile prevention.
- Match-result XP/gold feedback for the local P1 character in Single Player/Arena/Story and both local non-Guest profiles in VS, plus current gold balance in result summaries, character-select LV/XP, and fight-HUD LV/XP display.
- No default application of progression bonuses to live M.U.G.E.N combat constants until a future Dragon mode explicitly opts in.
- Story Mode foundation route with converted OpenBOR-style stage scrolling, one-player enemy waves, stage clear/fail results, and P1 progression award on clear.
- Arena-style shop hub route: a Dragon-only non-combat hub inspired by Flying Dragon, using Arena-style 2.5D player/NPC/counter presentation. The local player can hold movement through an expanded shop room, use tuned Shift/left-trigger run, stand in front of the counter, route through the top/back aisle and side lanes to the other side, and is blocked from walking through the solid counter body. The player can interact with I.Chie at the item counter, browse buy/sell/equip tabs with keyboard or shoulder/trigger controls, choose an equip target character, confirm/cancel transactions with clear feedback, read compact icon/name/value item rows plus selected-item effects, wrapped details, ownership, target, and balance-aware confirmation text in the shop panel overlay, and persist profile-owned gold, inventory, and equipment through `DragonProgression`; Guest can browse but cannot save transactions. The room renderer now has a richer built-in fallback composition with shelf bays, I.Chie neon branding, dragon accents, layered wall bands, a deeper counter face, and optional prompt-driven PNG layer hooks for a generated shop backdrop, counter back, and counter front under `game/data/shop/`.
- Story enemy defeats now grant configurable per-enemy XP/gold into the same named-profile economy used by the shop, display floating `+XP +G` feedback plus coin bursts, show live Story gold balance, and keep Guest non-persistent.

Blocked until earlier phases are stable:

- Tournament/campaign shell.
- Campaign economy expansion beyond the first Arena-style shop hub: multiple shops, crafting, branching inventories, item pickups, and story-gated shop stock.
- External editor.
- Original benchmark characters as real playable fixtures.

Done when:

- Each future mode has its own feature spec, content ownership plan, verification gates, and no dependency on hardcoded character behavior.

## Current Risks

High priority:

- Classic fight outcome, routing, and active-roster guard/fall/KO behavior now have scripted coverage; current manual GUI smoke for those same routes is still due.
- Broader CMD/runtime compatibility remains partial for air-only specials, throws, alpha counters, custom combo, full super catalogs, and non-roster characters.
- Newly added or non-selectable characters remain unproven until they are added to `game/data/select.def` and pass `roster-compatibility-smoke`.
- Four-fighter Arena and Story wave 3 now have scripted performance telemetry/culling coverage; a current manual live retest is still needed after major runtime or renderer changes before OpenBOR Stage Compatibility v2 can be called complete.
- Story Mode has scripted coverage for routing, map-style Stage Select, OpenBOR-stage defaulting, wave spawning, scrolling, targeting, clear/fail, and progression award. A current manual GUI pass is still due before calling the first Story foundation live-verified.
- Scott `Tram_Rooftop` is a local external stage proof only. Full Scott character compatibility, broader Scott stage coverage such as `Movie_Set` and `Cherry_Garden`, and direct OpenBOR `.pak` compatibility remain separate future work.

Medium priority:

- CPU behavior is a baseline, not full AI.
- Arena setup and GUI navigation need a current manual pass.
- VS/Arena/Story loading now has scripted staged-progress coverage; a current manual visual pass should still confirm the bar and phase text on real long-load paths.
- Training options need a broader live pass.

Polish and verification:

- Visual feedback smoke should be rerun on the current branch.
- Physical keyboard/controller retests should remain on every release checklist.
- Oversized source and verifier files are still architecture debt; current file-size guard failures include `App.cpp`, `VerificationScenario.cpp`, `VerificationScenarioSpecials.cpp`, `VerificationScenarioEvilKen.cpp`, `TrainingCommandPracticeAssembly.h`, and `VerificationScenarioArena.cpp`.

## Required Checks

Before engine/app commits:

```powershell
python engine/tools/dev_check.py . --skip-build
git diff --check
```

For build-level changes:

```powershell
cmake --build build --target dragon_mugen
python engine/tools/dev_check.py .
```

For architecture or verifier work:

```powershell
python tools/check_file_sizes.py
```

For compatibility work:

```powershell
build\dragon_mugen.exe --verify roster-compatibility-smoke
python engine/tools/audit_mugen_compat.py game
```

Use `REGRESSION_CHECKLIST.md` for mode-specific scripted and manual checks. Use `FEATURE_LEDGER.md` to preserve what already works.
