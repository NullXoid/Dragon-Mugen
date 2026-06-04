# MVP: Earliest-M.U.G.E.N-Style Runtime

The first playable target is not a full M.U.G.E.N replacement. It is a deliberately small runtime shaped like the earliest useful M.U.G.E.N build.

## Target Content

- Character: `game/chars/kfm`
- Stage: `game/stages/kfm.def`
- Shared common states: `game/data/common1.cns`

The 2001 package is used because the 1999 archive is program-only and does not include Kung Fu Man. The runtime folder keeps the M.U.G.E.N layout because that layout is part of the creator experience.

## First Playable Mode

Training/single-fight sandbox:

- Load KFM twice.
- Place P1 and P2 on the KFM stage.
- Let P1 move/jump/attack.
- Make P2 idle or use a simple dummy AI.
- Draw sprites, stage, lifebars, and debug collision boxes.
- Fixed 60 Hz simulation.

## Parser Order

1. DEF parser for character/stage file discovery.
2. AIR parser for animations and collision boxes.
3. ACT parser for palettes.
4. SFF v1 decoder for sprites.
5. CMD parser for input commands.
6. CNS parser for statedefs and controller blocks.
7. SND parser for sound metadata.

## Runtime Order

1. Draw stage.
2. Draw KFM idle animation.
3. Add animation timing.
4. Add input and facing.
5. Add basic physics: position, velocity, gravity, ground.
6. Add state machine with a minimal CNS subset.
7. Add collision/hit detection.
8. Add training options for debug visuals, hit feedback, hit sound, and basic dummy behavior.

## CNS Subset for First Playable

Initial controllers:

- `ChangeState`
- `ChangeAnim`
- `VelSet`
- `VelAdd`
- `PosSet`
- `CtrlSet`
- `StateTypeSet`
- `PlaySnd`
- `StopSnd`
- `HitDef` parsed into the first collision, damage, hit-pause, and get-hit behavior

Initial triggers:

- `Time`
- `AnimTime`
- `AnimElem`
- `Command`
- `Ctrl`
- `StateNo`
- `MoveType`
- `StateType`
- `Pos`
- `Vel`
- `Life`

## Deliberately Out of Scope for First Playable

- Menus.
- Arcade ladders.
- Team modes.
- Storyboards.
- Shop/equipment/leveling.
- Network play.
- Full M.U.G.E.N compatibility.
- SFF v2.
- HD/zoom behavior.

Those come after the small runtime can make KFM stand, move, attack, and reset cleanly.
