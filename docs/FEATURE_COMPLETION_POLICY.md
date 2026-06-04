# Feature Completion Policy

This project cannot keep growing by tiny behavior patches. A feature is not complete because one visible symptom improved. A feature is complete only when its data path, runtime ownership, UI/options, compatibility rules, documentation, and verification are all closed for the agreed scope.

## Feature Rule

Before implementation starts, create or update a feature spec under `docs/FEATURE_SPECS/`.

Every feature spec must include:

- Goal
- Source references
- Scope
- Ownership
- Implementation checklist
- Verification
- Done criteria

Only one feature may be marked `Status: In Progress` at a time. Small commits are allowed inside that feature, but they must move the same feature toward its done criteria. They are not permission to start another loose slice.

Planning language should use dependency order, difficulty, and completion value. Do not use uncertainty as the deciding factor for what to do next.

## Preservation Documentation Rule

Every feature spec must define a minimum batch. The minimum batch is the smallest change that can be honestly considered real progress on that feature.

The active guard in `engine/tools/guard_active_change.py` is not a line-count guard. It is a preservation guard. Any commit that changes engine/app code must also update one of:

- `docs/FEATURE_LEDGER.md`
- `docs/REGRESSION_CHECKLIST.md`
- the active `docs/FEATURE_SPECS/*.md` file

This forces every engine change to record what must keep working, what changed, or how it was verified.

If the minimum batch is too difficult, stop and revise the feature spec before coding. Do not replace it with an undocumented slice.

## Complete Means Complete

A completed feature must include:

- The M.U.G.E.N file/backend source of truth when the feature maps to M.U.G.E.N behavior.
- The Dragon extension documentation when the feature is not M.U.G.E.N behavior.
- Runtime ownership in an engine module, not hidden in screen-specific app code.
- UI/settings work for the feature when the player needs to control it.
- Verification steps that can be repeated.
- Cleanup of stale placeholders, dead branches, and one-off debug-only code.

If a feature cannot be completed at that scope, narrow the scope in the feature spec before coding. Do not silently land a half-feature.

## Mistake Protocol

When a change violates the project direction:

- State plainly what failed.
- Revert or isolate the bad change before continuing.
- Add or update a guard/policy if the same mistake can repeat.
- Re-run verification.
- Do not present generic options as a substitute for prevention.

## M.U.G.E.N Compatibility Rule

When implementing M.U.G.E.N behavior, consult the relevant M.U.G.E.N files and docs before coding:

- `game/data/select.def` for roster/stage selection.
- Character DEF `[Files]` for CMD/CNS/ST/AIR/SFF/SND/ACT resolution.
- `game/data/fight.def` for lifebars, powerbars, round, combo, sounds, and fight UI data.
- `game/data/common1.cns` and character CNS/ST files for common states and controller behavior.
- Character CMD files for inputs and State -1 move routing.
- Stage DEF/SFF files for background/camera/player starts.

The runtime should read the files and execute their behavior. Do not hardcode character-specific or stage-specific fixes unless the M.U.G.E.N files explicitly define that behavior.

## App Layer Rule

`engine/src/App.cpp` is large, but file size is not the active blocker. The active blocker is losing previous behavior while adding new behavior.

The app layer may orchestrate screens and sessions, but it must not own M.U.G.E.N file resolution, fight rules, CNS execution, input command parsing, collision/hit logic, audio controller behavior, or stage rendering rules unless that ownership is documented as temporary in the feature ledger.

If work stays in the app layer for playability, document what was added and what must remain working.

## No Lost Plan Items

If a plan item is accepted but not implemented immediately, it must be materialized in the repo:

- Roadmap or audit document for engine work.
- M.U.G.E.N-style folder or data file for content work.
- Dragon extension document for Dragon-only behavior.
- Feature spec when it becomes the active implementation target.

## No Lost Features

When a feature exists, it belongs in `docs/FEATURE_LEDGER.md`. When a behavior must be checked before commit, it belongs in `docs/REGRESSION_CHECKLIST.md`.

Adding a new feature is not allowed to erase or weaken older behavior unless the ledger explicitly documents the replacement.
