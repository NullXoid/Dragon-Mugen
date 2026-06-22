# A.Ben

A.Ben is an original Dragon MUGEN benchmark character foundation.

## Current Status

This folder now contains a normal M.U.G.E.N-style foundation fileset:

- `A.Ben.def`
- `A.Ben.cmd`
- `A.Ben.cns`
- `A.Ben.air`
- `A.Ben.sff`
- `A.Ben.dragon.def`

The character is intentionally **not** added to `game/data/select.def` in this pass. Add it to the active roster only after local runtime verification confirms that the foundation loads and fights in Training, VS, and Single Player.

## Role

Readable low-complexity AI and movement benchmark: dash, guard, spacing, normals, one special, one rising special, and one super.

## Visual Direction

Black hoodie, blue glow accents, B emblem, wrist tech, and runner/speed silhouette.

## v1 Combat Target

Fast but simple speed fighter. Six normals, ST/taunt, Boost Shot, Rising Boost, and Full Charge Rush.

## Later Expansion Target

Simple conservative AI gates for spacing, guard decisions, dash safety, and round-safe opponent behavior.

## Verification Gate

Before adding this character to `game/data/select.def`, run:

```powershell
python engine/tools/dev_check.py . --skip-build
python engine/tools/dev_check.py .
python engine/tools/audit_mugen_compat.py game
build\dragon_mugen.exe --verify kfm-baseline
build\dragon_mugen.exe --verify evilken-smoke
```

Then manually confirm the character can enter Training, VS, and Single Player without engine hardcoding.
