# Failure Postmortem: Architecture Drift

Date: June 4, 2026

## What Failed

The engine work drifted away from the user's stated direction. The project was supposed to follow M.U.G.E.N's content structure and documentation-driven behavior. Instead, too much behavior was implemented as small app-layer slices in `engine/src/App.cpp`.

That made the project feel broken and untrustworthy. The response afterward also minimized the problem by offering simple choices instead of putting hard prevention in place.

## Direct Cause

- Features were implemented as symptoms instead of complete modules.
- M.U.G.E.N docs and file ownership were referenced, but not enforced strongly enough before coding.
- `App.cpp` grew into a mixed application/runtime file.
- Architecture recovery started with a symbolic extraction that removed fewer than 100 lines.
- The emotional value of the project was not treated with enough care.

## Correction Already Applied

- The failed `ScreenFlow` extraction was reverted before commit.
- The repo returned to the clean committed state at `ec2f265`.
- `engine/tools/guard_active_change.py` now blocks future commits that touch `App.cpp` unless the file is reduced to a meaningful extraction target.
- Feature specs now require a minimum batch definition.
- The policy now uses dependency order, difficulty, and completion value as planning language.

## Prevention Rules

- Do not call a feature complete unless its spec's `Done Means` section is satisfied.
- Do not commit small `App.cpp` edits during architecture recovery.
- Do not add gameplay behavior until the current recovery feature either completes or is explicitly replaced by a new accepted feature spec.
- Do not use Dragon sidecars or app code to replace M.U.G.E.N files as the source of truth.
- Do not answer user frustration with generic choices. State what failed, what was reverted, what guard changed, and what remains.

## Current Recovery Gate

Any commit that stages `engine/src/App.cpp` must bring it to 15,500 lines or fewer. This forces the next App-layer change to be a real extraction batch instead of another small cleanup.
