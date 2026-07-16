# Changelog

All notable Dragon MUGEN changes are recorded here. Engine commits update the `Unreleased` section together with a preservation source.

## Unreleased

### Added

- Added a pinned V1/V2 Shop comparison path. V2 uses the supplied clean focus-room plate, deterministic alpha-sheet crops, a higher-resolution I.Chie welcome pose, and separately projected cabinet/wall/hologram layers; developer E2E now captures both versions at every output profile.
- Added an experimental Dragon Shop pinhole camera that projects the existing X/depth gameplay plane through camera depth, focal length, yaw, horizon, and camera height. Shop actors use perspective distance scaling and ground projection, while the counter projects its left/right edges independently as a textured trapezoid; collision and UI remain unchanged.
- Added strict Dragon-target compiler warning modes, opt-in warnings-as-errors, and documentation/change-notification enforcement for the Architecture Recovery cleanup slice.
- Added `video-resolution-presentation-e2e`, which renders Video Options through the production SDL target for all five output profiles, reads back the physical frame, checks native target dimensions/nonblank output, verifies menu row order/values/selection metadata, and compares the rendered menu panel while masking only the profile-specific Resolution value cell. Full `dev_check.py` now runs this scenario.
- Expanded resolution presentation verification across every top-level engine screen, all Options pages, Story Stage Select, Fight/Training/Arena/Story HUD variants, pause/result overlays, and the Shop walk-up plus V1/V2 greeting and open-panel states. Each Classic/Wide/Extra/SD/HD run now follows the Video Options input path and writes developer-only BMP proof frames under `build/verification-proof` for visual review; no verifier or proof output is included in normal runtime builds.
- Shop overlay proofs now snap the presentation to its completed cinematic camera state instead of capturing a panel-open flag over the stale walk-up camera.

### Changed

- Shop Hub now keeps its wide panorama for normal walking, then smoothly blends to the supplied concept-room composition with a focused camera, waist-high service counter, stronger I.Chie scale, staged foreground player, compact greeting banner, and gold-trimmed shop panel. No shop inventory, save, collision, or control behavior changed.
- Architecture reporting now includes physical `App.cpp` lines, direct implementation-shard count, shard lines, and aggregate direct-source lines.
- The cleanup slice reduced aggregate `App.cpp` plus direct-shard source from 17024 to 16821 lines while keeping the coordinator and shard count visible.
- Character Select, Arena Setup, Fight Result, Stage Select, and Story Stage Select now use one stable 640x360 virtual composition directly. Classic, Wide, Extra, SD 854x480, and HD 1280x720 remain selectable output profiles without changing UI composition.
- Verifier scenarios and the `--verify` entrypoint now compile only when `DRAGON_ENABLE_VERIFY=ON`; normal runtime builds no longer ship with verifier scenario code attached.

### Fixed

- Verifier fixture lookup now fails setup when a requested character is absent instead of silently selecting roster slot zero, covered by `missing-character-fixture-fails-setup`.
- Shop SD/HD viewport verification now asserts shared production geometry instead of searching for removed helper text.
- Resolution changes no longer rasterize the fixed 1280x720 Dragon presentation into a smaller intermediate texture. Classic previously reduced the UI to a letterboxed 320x180 image and enlarged it, corrupting text and borders; all profiles now retain the native presentation target and one final display fit.

### Deprecated

- Nothing currently deprecated.

### Removed

- Removed 33 proven internal zero-call functions, their orphaned helpers, unused Shop cover variants, and obsolete per-controller fired-state collections. Generic controller persistence and public APIs remain.

### Compatibility

- Public APIs, supported M.U.G.E.N parsers/controllers/content readers, Dragon extension boundaries, progression-save migration, and compatibility scenarios remain supported.

### Internal

- Consolidated presentation on the stable 640x360 virtual canvas, removed warning-exposed orphan helpers, and hardened cleanup validation. Dragon targets are warning-clean under strict GNU and MSVC validation builds.

### Known Issues

- Compatibility fixtures installed only on some developer machines remain optional; a requested missing fixture is now reported as a setup failure instead of being substituted.
