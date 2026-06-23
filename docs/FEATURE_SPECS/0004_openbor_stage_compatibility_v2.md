# OpenBOR Stage Compatibility v2

Status: In Progress

## Goal

Turn the current Arena-only OpenBOR-style scroll-stage experiment into a real compatibility slice: converted OpenBOR-style stages should load with documented metadata, show real staged loading progress on the VS/Arena load screen, and stay playable with four Arena fighters without unexplained sustained FPS drops.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/FEATURE_LEDGER.md`
- `docs/DRAGON_EXTENSIONS.md`
- `game/data/select.def`
- `game/stages/tmnt_openbor_street.def`
- `game/stages/tmnt_openbor_street.sff`
- `engine/src/VerificationScenarioArenaOpenBor.cpp`
- `engine/src/VerificationScenarioLoading.cpp`
- `engine/src/LoadingProgressState.h`
- `engine/src/RuntimeLoading.h`
- `engine/src/FightSessionRuntime.h`
- `engine/src/FramePerformance.h`
- `engine/src/WorldRender.h`
- `engine/src/VerificationScenarioPerformance.cpp`
- `engine/src/VsScreenOverlay.h`
- `engine/src/VsScreenOverlay.cpp`

## Scope

In scope:

- Arena-only OpenBOR Stage Compatibility v2 for converted stage assets.
- Real load progress UI for shared VS/Arena/Story loading, driven by staged loading phases.
- OpenBOR-style stage conversion/import rules that produce normal Dragon/M.U.G.E.N-compatible stage files plus documented `[OpenBOR]` or `[DragonOpenBOR]` metadata.
- 4-fighter Arena performance telemetry and stress verification on converted OpenBOR-style stages.
- Scroller camera bounds, forward-only scroll behavior, fighter start spacing, stage preview readiness, and result/pause routing preservation.
- Documentation of what OpenBOR behavior is supported, partial, or intentionally deferred.

Out of scope:

- Direct arbitrary `.pak` execution inside the runtime.
- Full OpenBOR scripting, enemy spawners, hazards, platforms, shops, story, or beat-em-up campaign flow.
- True 3D/rotated backgrounds.
- Character-specific fixes for Leonardo, KFM, Evil Ken, Evil Ryu, or any other downloaded character.
- Replacing M.U.G.E.N stage compatibility in Training, Single Player, or VS.

## Feature Slice

This feature is one complete Arena/OpenBOR compatibility slice, not a set of isolated polish patches. It includes an owned loading-progress boundary, an OpenBOR conversion/metadata boundary, a four-fighter performance measurement path, scripted verifiers, manual test instructions, and roadmap/checklist/ledger updates.

The loading bar must represent actual progress through known work such as character metadata, character runtime, stage metadata, stage sprites, textures, sounds, and fight-session preparation. If a loading step is still synchronous, the feature must split or report that step honestly instead of drawing a fake smooth bar.

## Ownership

- OpenBOR compatibility rules live in Arena/stage compatibility modules and `docs/DRAGON_EXTENSIONS.md`.
- Converted fixture coverage lives in `engine/src/VerificationScenarioArenaOpenBor.cpp`.
- VS/Arena load presentation lives in `VsScreenOverlay` through prepared view data, while loading/resource ownership remains outside the overlay.
- Performance telemetry belongs in the focused `FramePerformance` diagnostics module, with render-only culling kept in world-render helpers and thin integration in `App.cpp`.
- Regression requirements live in `docs/REGRESSION_CHECKLIST.md`.
- Preservation claims live in `docs/FEATURE_LEDGER.md`.

## Implementation Checklist

- [x] Add staged loading progress data that can report current phase, bounded fraction, and display percentage for shared VS/Arena/Story loading.
- [x] Render a real progress bar on the VS/loading screen using `VsScreenOverlay` view data.
- [x] Keep load progress truthful when a phase cannot be split yet by showing the active phase and bounded progress rather than fake completion.
- [ ] Define and document OpenBOR Stage Compatibility v2 metadata fields, importer/converter expectations, and unsupported OpenBOR behaviors.
- [ ] Add or update a conversion tool/path for OpenBOR-style stage panels into Dragon/M.U.G.E.N-compatible stage DEF/SFF assets.
- [ ] Expand `arena-tmnt-openbor-stage` coverage for stage metadata, scroll bounds, start spacing, pause/result routing, and non-Arena isolation.
- [x] Add 4-fighter Arena performance telemetry for frame time, FPS, fighter count, helper/projectile/effect count, and stage draw workload.
- [x] Add a 4-fighter Arena/OpenBOR stress verifier or recorded telemetry capture that can distinguish renderer/runtime slowdown from hitpause or pause states.
- [x] Profile and fix the confirmed low-FPS 4-player case without hardcoding Leonardo or the TMNT fixture.
- [x] Update roadmap, ledger, regression checklist, and compatibility docs after the feature behavior is implemented.

Performance note: `FramePerformance` now tracks rolling frame summaries, p95/worst frame time, fixed-step pressure, draw/skipped-draw counts, workload counts, and pause/hitpause/superpause separation. Video Options can show `FPS`, `PERF`, or `OFF`; `DRAGON_PERF_OVERLAY=1` forces the detailed overlay and `DRAGON_PERF_LOG=1` writes local ignored TSV captures under `artifacts/perf/`. World rendering performs expanded-viewport render-only culling for stage tiles, actors, projectiles, effects, shadows, and afterimages without skipping gameplay updates, collision, helper/projectile lifetime, or result/death rendering.

## Verification

Run:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify arena-openbor-scroll-stage
build\dragon_mugen.exe --verify arena-tmnt-openbor-stage
build\dragon_mugen.exe --verify vs-loading-progress-bar
build\dragon_mugen.exe --verify runtime-performance-metrics
build\dragon_mugen.exe --verify arena-openbor-4fighter-performance
build\dragon_mugen.exe --verify render-culling-preserves-runtime
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify arena-cpu-2
build\dragon_mugen.exe --verify arena-cpu-3
build\dragon_mugen.exe --verify roster-compatibility-smoke
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
python engine\tools\check_feature_specs.py .
git diff --check
python tools\check_file_sizes.py
```

Manual smoke:

- Start Arena with `TMNT OpenBOR Street`.
- Confirm the VS/Arena/Story load screen shows a progress bar and useful current phase text.
- Run 1 CPU, 2 CPU, and 3 CPU matches.
- In the 3 CPU match, keep all four fighters visible and active long enough to observe FPS/frame-time telemetry.
- Confirm low FPS is either fixed or recorded with enough telemetry to identify the bottleneck.
- Confirm Training, Single Player, and VS do not auto-scroll from OpenBOR metadata.

## Done Means

- OpenBOR Stage Compatibility v2 has a clear supported subset, conversion/import path, and documented unsupported behaviors.
- VS/Arena/Story loading progress reflects actual loading phases.
- Converted OpenBOR-style stages are selectable through `select.def`, previewable, and playable in Arena without breaking non-Arena modes.
- Four-fighter Arena on the converted TMNT OpenBOR Street fixture has measured performance evidence and no untriaged sustained low-FPS case.
- The implementation lives in owned loading, stage compatibility, performance, and verifier modules rather than expanding `App.cpp`.
