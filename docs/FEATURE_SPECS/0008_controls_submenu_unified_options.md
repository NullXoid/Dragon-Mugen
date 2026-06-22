# Controls Submenu And Unified Options

Status: Complete

## Goal

Replace the flat Options screen with category navigation and add a full Controls submenu that maps physical keyboard/controller inputs to canonical engine actions per local Dragon profile.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/FEATURE_COMPLETION_POLICY.md`
- `docs/FEATURE_LEDGER.md`
- `docs/REGRESSION_CHECKLIST.md`
- `engine/src/ControlMapping.h`
- `engine/src/ControlsStore.h`
- `engine/src/ControlsOptionsMenu.h`
- `engine/src/ControlsOptionsFlow.h`
- `engine/src/Input.h`
- `game/save/controls.def`

## Scope

In scope:

- Root Options categories: Gameplay, Video, Controls, Back.
- Gameplay rows for match timer, P1/P2 profile flow, and fall fallbacks.
- Video rows for canvas size, UI scale, and FPS cap.
- Controls submenu rows for Player 1-4 Controls, Keyboard Setup, Controller Setup, Input Test, Restore Defaults, and Back.
- Per-player control screens with profile, device assignment, preset, action set, missing required actions, conflicts, action bindings, restore defaults, and guided setup.
- Action-based mapping for fighting, Arena/Flying Dragon, Training shortcuts, and beat-em-up/OpenBOR-style actions.
- Keyboard scancode, SDL gamepad button, axis, trigger, and touchpad bindings where SDL exposes them.
- Per-profile save/load under the Dragon save root with schema version `1`.
- Guest/default behavior that allows play without corrupting saved custom mappings.
- Presets: Arcade Fighter, Beat 'em Up Modern, OpenBOR Classic, Keyboard Classic.
- Device-family glyph labels for keyboard, Xbox-style, PlayStation-style, and generic pads.
- Pause kept as a system action separate from `ST/Taunt` by default.

Out of scope:

- Online profiles, cloud sync, Steam Input, GameInput, or a controller-art editor.
- Applying progression stats to M.U.G.E.N combat constants.
- Character-file edits or character-specific control hardcodes.
- Replacing the existing M.U.G.E.N command buffer; controls still feed the same final `FighterInputState`.

## Feature Slice

This slice completes persistent local input ownership: the player can navigate Options by category, choose profile/device/preset per player, remap required actions, detect conflicts, test live inputs, and have the fight runtime consume those bindings through the existing fighter input path.

## Ownership

- `ControlMapping` owns action IDs, action sets, presets, default bindings, glyph labels, missing-action checks, and conflict detection.
- `ControlsStore` owns `game/save/controls.def` load/save and schema-versioned persistence.
- `ControlsOptionsMenu` owns Options/Controls row construction and view state for root/category/player/setup/test/defaults screens.
- `ControlsOptionsFlow.h` owns App-local Options screen transitions, binding capture, and per-row mutation glue after the required AppState helpers are available.
- `Input` owns conversion from physical mapped actions to `FighterInputState`.
- `App.cpp` owns only thin integration: profile sync, save/load calls, event forwarding, and passing mapped input to existing runtime consumers.
- Verification lives in `VerificationScenario.cpp` until verifier-family splitting resumes under architecture recovery.

## Implementation Checklist

- [x] Add action, action-set, physical binding, profile binding, preset, and controls-settings types.
- [x] Add default keyboard and SDL gamepad mappings for four players.
- [x] Add preset application and per-action binding labels.
- [x] Add conflict detection that flags important gameplay conflicts while ignoring contextual training shortcut sharing.
- [x] Add controls save/load under `game/save/controls.def`.
- [x] Replace flat Options rendering data with category-aware rows and titles.
- [x] Add Player 1-4 Controls, Keyboard Setup, Controller Setup, Input Test, Restore Defaults, and Back rows.
- [x] Add guided/manual binding capture for keyboard keys, gamepad buttons, axes, triggers, and touchpad.
- [x] Route fight input through mapped control profiles while preserving `FighterInputState`.
- [x] Keep Pause and `ST/Taunt` separate by default.
- [x] Add verifiers for options navigation, player navigation, setup, conflicts, presets, persistence, input test, glyphs, and pause/taunt separation.
- [x] Update the unified roadmap and feature ledger.

## Verification

Focused checks:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify options-category-navigation
build\dragon_mugen.exe --verify controls-player-1-4-navigation
build\dragon_mugen.exe --verify controls-guided-setup
build\dragon_mugen.exe --verify controls-manual-edit-conflicts
build\dragon_mugen.exe --verify controls-presets
build\dragon_mugen.exe --verify controls-profile-persistence
build\dragon_mugen.exe --verify controls-input-test-live
build\dragon_mugen.exe --verify controls-glyph-device-detection
build\dragon_mugen.exe --verify controls-pause-taunt-separation
```

Broader regression checks:

```powershell
build\dragon_mugen.exe --verify training-show-controller-shortcut
build\dragon_mugen.exe --verify training-command-held-button-prompt
build\dragon_mugen.exe --verify arena-z-keyboard-controls
build\dragon_mugen.exe --verify arena-z-gamepad-controls
build\dragon_mugen.exe --verify cpu-baseline
build\dragon_mugen.exe --verify evilken-specials-supers
build\dragon_mugen.exe --verify evilryu-specials-supers
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
git diff --check
python tools\check_file_sizes.py
```

## Done Means

- Options is category-based instead of one flat list.
- Players 1-4 have visible profile, device, preset, action-set, missing-action, conflict, and binding controls.
- Physical input maps to canonical actions and then to the existing fighter input state.
- Controls persist per Dragon profile and survive app restart for non-Guest profiles.
- Start/Options remains Pause by default, PlayStation touchpad can map to `ST/Taunt`, and those actions do not conflict unless the user binds them that way.
- The feature is documented in the unified roadmap/ledger and covered by focused verifiers.
