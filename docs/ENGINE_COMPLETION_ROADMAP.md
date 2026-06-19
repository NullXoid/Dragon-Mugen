# Engine Completion Roadmap

This is the single planning source for completing Dragon MUGEN as a playable, editable, M.U.G.E.N-style fighting engine. Older roadmap and backlog files now point here. Operational records remain separate: `FEATURE_LEDGER.md` preserves what already works, and `REGRESSION_CHECKLIST.md` defines what must be rerun.

## Direction

- Engine: C++20 or newer, CMake, SDL3 platform layer, bgfx renderer direction.
- Runtime content root: `game/`, using M.U.G.E.N-style folders and names.
- Roster and stage authority: `game/data/select.def`.
- Character and stage source of truth: editable M.U.G.E.N files first; Dragon `.dragon.def` sidecars only extend behavior.
- Public-release requirement: replace official M.U.G.E.N and third-party assets with original or properly licensed content.
- Feature process: one active feature spec under `docs/FEATURE_SPECS/`; broad work must not grow `App.cpp` as a catch-all.

Non-goals before the core engine is stable:

- No networking.
- No story, tournament, shop, equipment, or RPG progression as active work.
- No editor runtime pivot.
- No hardcoded fixes for individual downloaded characters.
- No JSON character/stage runtime sidecars replacing `.dragon.def`.

## Current State

The engine already has a working local play loop:

- Training, Single Player, VS Mode, and DLC Arena Mode are present as separate paths.
- Character and stage selection route through `select.def`.
- KFM, Evil Ken, Evil Ryu, and local compatibility fixtures load through selected character DEF files.
- SFF v1, ACT, AIR, CMD, CNS, SND, stage DEF/SFF, fight data, and common fight FX are partially supported.
- Training includes command HUD, input history, command list, Show Me, pause/capture controls, controller prompts, palette separation, and command completion feedback.
- Arena includes FFA CPU counts, per-fighter runtimes, Z-axis movement, sidestep, depth-aware hit/push/draw order, camera yaw, and a simple OpenBOR-style scroll-stage pass.
- Single Player and VS have round/match presentation, pause, result, rematch, and local P2 runtime support.
- Scripted verifiers cover the main compatibility roster, Arena runtime, Training command systems, and CPU baseline.

The engine is not complete yet. The main remaining work is fight-rule correctness, deeper M.U.G.E.N compatibility, Training completion, Arena polish, persistent input/settings, presentation, and architecture recovery.

## Completion Phases

### 1. Fight Correctness

Goal: make classic Training, Single Player, and VS fight outcomes reliable before building higher modes.

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

Key work:

- Expand trigger and expression support: variables, ranges, random branches, redirection, `enemynear`, `helper(...)`, `target`, `p2bodydist`, `p2stateno`, `numtarget`, `AnimElemTime`, edge distance, and richer `roundstate`/`power` forms.
- Improve controller parity: target controllers, throws/cinematics, helper lifecycle, projectile edge cases, explod ownership, pause/superpause, palette effects, `ScreenBound movecamera`, and expression-backed controller values.
- Keep unsupported behavior documented in `COMPATIBILITY_AUDIT.md`.
- Rerun `python engine/tools/audit_mugen_compat.py game` after every compatibility expansion.

Done when:

- KFM, Evil Ken, and Evil Ryu load without unexpected unsupported-controller failures.
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

Key work:

- Rerun manual Arena setup/navigation: Character Select, CPU count 1/2/3, CPU slots, stage, timer, Z Axis, Camera Rotate, Start Match, Back, and result routing.
- Improve CPU behavior after fight correctness is stable, still using generic character runtime data.
- Expand OpenBOR-style scrolling stages only through documented Arena/DLC metadata.
- Keep Z-axis, camera yaw, depth hit gating, depth push, draw order, and defeated-fighter handling Arena-only.

Done when:

- Arena scripted verifiers and a current manual GUI pass cover setup, runtime, pause/result routing, and Z/camera behavior.
- Training, Single Player, and VS are unaffected when Arena Z/camera/scroller features are enabled in Arena.

### 5. Input And Settings

Goal: make physical controls and user preferences reliable and persistent.

Key work:

- Add keyboard and controller rebinding through a shared options UI.
- Save user settings under `game/save/settings.def`.
- Keep automatic controller prompt detection, prompt style overrides, Start/Select behavior, and command-buffer routing consistent across modes.
- Maintain physical keyboard and controller smoke tests for movement, diagonals, commands, pause, result navigation, and command training.

Done when:

- Rebinding survives app restart.
- Keyboard, Xbox-style, and PlayStation-style prompts map to the same canonical command IDs.
- Physical input smoke confirms both keyboard and controller feed the command buffer.

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

Blocked until earlier phases are stable:

- Story mode.
- Tournament/campaign shell.
- Shop, equipment, leveling, or progression.
- External editor.
- Original benchmark characters as real playable fixtures.

Done when:

- Each future mode has its own feature spec, content ownership plan, verification gates, and no dependency on hardcoded character behavior.

## Current Risks

High priority:

- Classic Single Player and VS still need dedicated KO/double-KO/draw/pause-action verification.
- Guard, fall recovery, guard effects, and KO coverage remain partial.
- Broader CMD/runtime compatibility remains partial for air-only specials, throws, alpha counters, custom combo, full super catalogs, and non-roster characters.

Medium priority:

- CPU behavior is a baseline, not full AI.
- Arena setup and GUI navigation need a current manual pass.
- Training options need a broader live pass.

Polish and verification:

- Visual feedback smoke should be rerun on the current branch.
- Physical keyboard/controller retests should remain on every release checklist.
- Oversized source and verifier files are still architecture debt.

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
python engine/tools/audit_mugen_compat.py game
```

Use `REGRESSION_CHECKLIST.md` for mode-specific scripted and manual checks. Use `FEATURE_LEDGER.md` to preserve what already works.
