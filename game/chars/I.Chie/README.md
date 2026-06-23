# I.Chie

I.Chie is an original Dragon MUGEN advanced benchmark character foundation.

## Current Status

This folder now contains a normal M.U.G.E.N-style text foundation:

- `I.Chie.def`
- `I.Chie.cmd`
- `I.Chie.cns`
- `I.Chie.air`
- `I.Chie.dragon.def`

The runtime sprite archive `I.Chie.sff` still needs to be produced from approved/redistributable sprite assets before the character is added to `game/data/select.def`.

The character is intentionally **not** added to `game/data/select.def` in this pass. Add it to the active roster only after local runtime verification confirms that the foundation loads and fights in Training, VS, and Single Player.

## Role

Advanced AI and compatibility stress benchmark foundation for variables, helpers, projectiles, target controllers, superpause, explods, palette effects, and richer decision logic.

## Visual Direction

Black hoodie, purple glow accents, triangle emblem, wrist tech, agile runner silhouette, and violet tech energy.

## v1 Combat Target

Agile tech fighter. Six normals, ST/taunt, Violet Pulse, Violet Rise, and Prism Break Rush.

## Later Expansion Target

Echo Step, helper/echo clone, projectile stress, target-controller routes, superpause, explods, palette effects, and richer AI.

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
