# Changelog

All notable Dragon MUGEN changes are recorded here. Engine commits update the `Unreleased` section together with a preservation source.

## Unreleased

### Added

- Added strict Dragon-target compiler warning modes and documentation/change-notification enforcement for the Architecture Recovery cleanup slice.

### Changed

- Architecture reporting now includes physical `App.cpp` lines, direct implementation-shard count, shard lines, and aggregate direct-source lines.

### Fixed

- Verifier fixture lookup now fails setup when a requested character is absent instead of silently selecting roster slot zero.

### Deprecated

- Nothing currently deprecated.

### Removed

- Removed proven internal zero-call helpers, obsolete per-controller state, and unreachable legacy presentation layouts.

### Compatibility

- Public APIs, supported M.U.G.E.N parsers/controllers/content readers, Dragon extension boundaries, progression-save migration, and compatibility scenarios remain supported.

### Internal

- Consolidated presentation on the stable 640x360 virtual canvas and hardened cleanup validation. No intended player-visible behavior change.

### Known Issues

- Compatibility fixtures installed only on some developer machines remain optional; a requested missing fixture is now reported as a setup failure instead of being substituted.
