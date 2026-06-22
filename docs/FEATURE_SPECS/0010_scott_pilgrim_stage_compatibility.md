# Scott Pilgrim Stage Compatibility

Status: Complete

## Goal

Add a complete first external-stage compatibility proof using the local Scott Pilgrim Versus package, with SFF v2 PNG/palette stage sprites, IKEMEN-tolerant select parsing, first-pass animated stage backgrounds, MP3/OGG/WAV stage music decoding, and a mounted Story board for `Tram_Rooftop`.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/FEATURE_LEDGER.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/DRAGON_EXTENSIONS.md`
- `engine/include/dragon/Sff.h`
- `engine/src/Sff.cpp`
- `engine/include/dragon/MugenData.h`
- `engine/src/MugenData.cpp`
- `engine/src/StageMusicDecoder.h`
- `engine/src/AudioRuntime.h`
- `engine/src/VerificationScenarioScottCompatibility.cpp`
- `game/data/external_content.local.def`

## Scope

In scope:

- SFF v2 archive detection, palette-record parsing, and PNG-backed stage sprite decoding while preserving existing SFF v1 PCX behavior.
- SFF archive/sprite version and encoding metadata for diagnostics and verifiers.
- IKEMEN-aware `select.def` parsing that ignores `slot = { ... }` blocks and row metadata instead of creating fake roster entries.
- Tolerant stage path resolution for duplicate `stages/` prefixes and local external stage paths.
- SDL_mixer-backed stage BGM decoding for WAV, MP3, and OGG into Dragon's existing float sample mixer and loop channel.
- A local-only external content registry mounted from `game/data/external_content.local.def`.
- First-pass stage `type = anim` background support through `Begin Action` frames.
- Wide-viewport coverage for large fixed, non-tiled backdrop layers so 320/384-localcoord modern stages do not expose black side gutters in Dragon's wide presentation.
- First proof board: `Tram_Rooftop.def` from `C:\Users\kasom\Desktop\Scott Pilgrim Versus Vanilla\Scott Pilgrim Versus`, loaded through Story Mode with `Run Scott Run.mp3`.
- Focused verifiers for SFF v2 PNG decode, IKEMEN slot parsing, codec decode, external stage mount, and Story loading of the Scott board.

Out of scope:

- Copying Scott Pilgrim package assets into the repository.
- Full Scott character compatibility.
- Full arbitrary OpenBOR `.pak` execution or OpenBOR scripting.
- Completing OpenBOR Stage Compatibility v2 performance/import/conversion gates.
- Character-file edits or character-specific runtime fixes.

## Feature Slice

This slice proves a real external M.U.G.E.N/IKEMEN-style stage package can be mounted without vendoring third-party assets, decoded through the normal stage renderer, played with non-WAV music, and routed through Story Mode. It is an external stage compatibility proof, not a broad OpenBOR or Scott character compatibility claim.

## Ownership

- `Sff.h` and `Sff.cpp` own SFF archive version detection, v1/v2 sprite and palette metadata, PCX decode, and PNG-backed v2 sprite decode.
- `MugenData.h` and `MugenData.cpp` own IKEMEN-tolerant roster/stage parsing, duplicate-prefix stage path resolution, and local external stage mount metadata.
- `StageMusicDecoder.h` owns SDL_mixer codec decoding into Dragon's `DecodedSoundSample` format.
- `AudioRuntime.h` owns SDL_mixer lifecycle integration and stage music loop playback.
- `game/data/external_content.local.def` owns local-only external package mounts and is git-ignored.
- `VerificationScenarioScottCompatibility.cpp` owns the focused compatibility verifiers.
- `App.cpp` keeps only thin audio state and integration.

## Implementation Checklist

- [x] Add SDL_mixer dependency discovery, FetchContent fallback, linking, and Windows DLL copy.
- [x] Add SFF v2 header parsing and PNG sprite record decode while preserving SFF v1 PCX behavior.
- [x] Add SFF v2 palette-record parsing so indexed PNG sprites use their authored palettes.
- [x] Expose SFF archive version and sprite encoding metadata.
- [x] Add IKEMEN `slot = { ... }` skip logic for character and stage select parsing.
- [x] Add tolerant duplicate `stages/` prefix resolution.
- [x] Add external content registry parsing for local-only package mounts.
- [x] Register the Scott `Tram_Rooftop` board without copying third-party assets into `game/`.
- [x] Decode WAV/MP3/OGG stage BGM into the existing float sample mixer.
- [x] Add first-pass stage animated background support for `type = anim` / `actionno` blocks.
- [x] Add wide-viewport fixed-backdrop coverage so Scott rooftop sky art fills the screen edges.
- [x] Prove Story can load `Tram_Rooftop` with the external stage SFF v2 background and `Run Scott Run.mp3`.
- [x] Add focused verifiers and update roadmap, ledger, regression checklist, and extension docs.

## Verification

Focused checks:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify sff-v2-png-decode
build\dragon_mugen.exe --verify ikemen-select-slot-parsing
build\dragon_mugen.exe --verify stage-music-codec-decode
build\dragon_mugen.exe --verify external-stage-mount
build\dragon_mugen.exe --verify story-scott-tram-rooftop
```

Regression checks:

```powershell
build\dragon_mugen.exe --verify story-stage-board-expansion
build\dragon_mugen.exe --verify story-stage-select-map
build\dragon_mugen.exe --verify story-openbor-stage-default
build\dragon_mugen.exe --verify story-wave-spawn-scroll
build\dragon_mugen.exe --verify vs-loading-progress-bar
build\dragon_mugen.exe --verify arena-tmnt-openbor-stage
build\dragon_mugen.exe --verify cpu-baseline
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
git diff --check
python tools\check_file_sizes.py
```

Manual smoke:

- Keep `game/data/external_content.local.def` pointed at the local Scott Pilgrim Versus package.
- Start Story Mode, select Leonardo or another selectable fighter, and choose `Tram_Rooftop`.
- Confirm the VS/loading screen shows real progress, the stage renders, camera bounds/zoffset are usable, and `Run Scott Run.mp3` plays.
- Confirm match clear/fail result routing still returns through the Story result screen.

## Done Means

- The external Scott `Tram_Rooftop` stage is mounted through a git-ignored local registry and appears as a normal selectable Story board when the package exists.
- SFF v2 PNG stage sprites and palette records decode through the shared SFF loader without regressing SFF v1 PCX content.
- The first proof stage can render its animated rooftop/background elements instead of loading to a black-only stage or exposing black side gutters.
- IKEMEN select metadata no longer creates fake characters or stages.
- WAV, MP3, and OGG stage music can decode through the stage music channel.
- Focused verifiers prove the compatibility path, and docs record the supported subset and limits.
