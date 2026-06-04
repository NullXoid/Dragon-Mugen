# Kung Fu Man

This folder contains Kung Fu Man from the official 2001 DOS M.U.G.E.N package. It is used as the first engine compatibility target.

## Files

- `kfm.def` - character manifest.
- `kfm.air` - animations and collision boxes.
- `kfm.cmd` - command definitions.
- `kfm.cns` - constants, state logic, `HitDef` damage, guard flags, and guard movement data.
- `kfm.sff` - character sprites.
- `kfm.snd` - character-owned sounds. The first sound pass checks this archive before shared `game/data` sound archives when a KFM `HitDef` requests a sound.
- `kfm.act` through `kfm6.act` - palettes.
- `intro.def` / `intro.sff` - character intro storyboard.
- `ending.def` / `ending.sff` - character ending storyboard.
- `readme.txt` - original M.U.G.E.N character readme.

## First Engine Target

The initial runtime should load this character, play idle/crouch animation, handle basic movement, execute standing normal states `200`, `210`, `230`, and `240`, execute crouching normal states `400`, `410`, `430`, and `440`, execute KFM special states through the original `kfm.cmd` and `kfm.cns`, and keep the parsed `HitDef` hit/guard data compatible with the original files.

The current special-move baseline expects KFM's own CNS `PlaySnd`, animation-end and command-triggered `ChangeState`, `SelfState`, `CtrlSet`, `VarSet`, `VarAdd`, `VarRandom`, multi-trigger `PosAdd`, `VelSet`, `VelAdd`, `VelMul`, `PosSet`, `PosFreeze`, `HitVelSet`, `SprPriority`, `Explod`, `AssertSpecial`, `Width`, `PlayerPush`, and `movecontact`/`AnimElemTime` `ChangeAnim` patterns to work without editing the reference character files.

Training mode's command-training move-list page reads the display names and target states from this character's original `kfm.cmd` `State -1` entries. Runtime input also reads the original `[Command]` definitions, so simple motions and button combinations should be added in `kfm.cmd` using M.U.G.E.N command syntax instead of Dragon-only bindings. The move list currently shows moves whose referenced CNS state and AIR animation are loaded by the runtime.

Avoid changing these reference files unless the goal is specifically to test compatibility behavior.
