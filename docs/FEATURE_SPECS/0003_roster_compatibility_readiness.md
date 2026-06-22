# Roster Compatibility Readiness

Status: Complete

## Goal

Make new-character intake practical by adding a generic selectable-roster smoke verifier. The verifier must prove that every current `select.def` character can load through the normal runtime path, reach a stable fight, expose usable runtime data, move, and make simple contact without adding character-specific engine branches.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/FEATURE_LEDGER.md`
- `game/data/select.def`
- selected character DEF/CMD/CNS/AIR/SFF/SND/ACT files under `game/chars/`
- `engine/include/dragon/MugenData.h`
- `engine/src/AppVerificationBridge.h`
- `engine/src/VerificationScenarioRosterCompatibility.cpp`

## Scope

In scope:

- Enumerate current selectable characters from `game/data/select.def`.
- Exercise the same Training setup path a new selectable character uses.
- Verify fight/idle readiness, runtime state and command data, playable constants/scale, idle stability, walking, jumping, simple button attack response, and simple hit contact.
- Treat missing loaded HitDefs, jump response, simple attack response, or simple contact as capability warnings when setup and baseline runtime remain healthy.
- Keep the verifier generic and data-driven.

Out of scope:

- Importing or downloading new characters.
- Manual palette, control, or move-list editing.
- Full command-list or super-catalog certification for every character.
- Hardcoded fixes for Lili, KFM, Evil Ken, Evil Ryu, or any future downloaded character.

## Feature Slice

The completed feature slice is a full selectable-roster compatibility gate. It adds a reusable `RuntimeProbe::selectableCharacters()` interface, a focused roster-compatibility verifier module, repeated setup cleanup in the verification bridge, dispatch/build registration, and roadmap/checklist/ledger documentation.

This slice is not a substitute for move-specific compatibility work. It is the intake gate that tells us whether a character is ready for deeper command, damage, helper, projectile, throw, and super audits.

## Ownership

- Selectable character metadata is owned by `dragon::loadCharacters(...)` and `game/data/select.def`.
- Runtime setup and teardown for scripted checks is owned by `AppVerificationBridge.h`.
- Roster readiness behavior is owned by `engine/src/VerificationScenarioRosterCompatibility.cpp`.
- Preservation claims live in `docs/FEATURE_LEDGER.md`.
- Required reruns live in `docs/REGRESSION_CHECKLIST.md`.

## Implementation Checklist

- [x] Expose selectable roster entries through `RuntimeProbe`.
- [x] Add repeated verification-session resource cleanup before each `setup()` call.
- [x] Add `roster-compatibility-smoke` as a focused verifier module.
- [x] Verify load, fight phase, idle readiness, runtime bundle, idle stability, walk, jump, simple attack, and simple contact for every current selectable character.
- [x] Register the verifier in CMake and `--verify` dispatch.
- [x] Update roadmap, ledger, and regression checklist.

## Verification

Run:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify roster-compatibility-smoke
build\dragon_mugen.exe --verify classic-fight-combat
build\dragon_mugen.exe --verify compatibility-profile-resolver
build\dragon_mugen.exe --verify character-auto-fit-scale
build\dragon_mugen.exe --verify lili-smoke
build\dragon_mugen.exe --verify cpu-baseline
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
python engine\tools\check_feature_specs.py .
git diff --check
python tools\check_file_sizes.py
```

The current pass reports `roster-compatibility-smoke` as `pass=56 partial=0 fail=0 blocked=0` for KFM, Evil Ryu, Evil Ken, CFJ Lili, Lili QYC Custom, and Lili QYC Normal.

## Done Means

- Every current selectable roster entry can be loaded and smoke-tested through the same runtime path used by Training.
- A future new character has a clear first gate: add it to `game/data/select.def`, run `roster-compatibility-smoke`, then perform deeper character-specific compatibility checks only after the generic smoke passes.
- The verifier remains generic and does not hardcode character-specific runtime behavior.
- The feature is tracked in the unified roadmap, preservation ledger, and regression checklist.
