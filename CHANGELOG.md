# Changelog

All notable Dragon MUGEN changes are recorded here. Engine commits update the `Unreleased` section together with a preservation source.

## Unreleased

### Added

- Added strict Dragon-target compiler warning modes, opt-in warnings-as-errors, and documentation/change-notification enforcement for the Architecture Recovery cleanup slice.

### Changed

- Architecture reporting now includes physical `App.cpp` lines, direct implementation-shard count, shard lines, and aggregate direct-source lines.
- The cleanup slice reduced aggregate `App.cpp` plus direct-shard source from 17024 to 16821 lines while keeping the coordinator and shard count visible.
- Character Select, Arena Setup, Fight Result, Stage Select, and Story Stage Select now use one stable 640x360 virtual composition directly. Classic, Wide, Extra, SD 854x480, and HD 1280x720 remain selectable output profiles without changing UI composition.

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
