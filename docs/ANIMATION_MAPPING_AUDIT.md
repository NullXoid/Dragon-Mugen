# Animation Mapping Audit

This file tracks M.U.G.E.N common AIR action numbers that exist in the current test characters and whether Dragon MUGEN actively routes runtime behavior into them.

The runtime loads all AIR actions it can decode. `mapped` means the engine currently has gameplay logic that can choose the action. `unmapped` means the action exists in the character AIR file but the engine has no direct common-state behavior for it yet.

## Current Common-State Fixes

- Fall landing now follows `5100/5101 -> 5110 -> 5120 -> 0` when the character has action `5120`.
- This prevents grounded knockdowns from freezing in liedown and popping straight back to standing idle.
- The wake-up path is implemented as common M.U.G.E.N behavior because KFM, Evil Ryu, and Evil Ken provide AIR actions for these states but do not all define equivalent character CNS statedefs.
- Guard ready now routes through `120 -> 121 -> 122` or `130 -> 131 -> 132` where the selected character provides those actions. Missing guard idle/end frames fall back to the nearest available guard/idle state.
- Crouching defenders now use `5020-5022` and `5025-5027` where available. Characters missing crouch get-hit frames fall back to low/standing get-hit actions.
- Airborne defenders now use `5030` for air hit, `5040`/`5210`/`5140` for air recovery, and `5200`/`5210` fall recovery where available.
- Knockdowns now follow the M.U.G.E.N fall sequence more closely: first ground impact `5100`, bounce-up action `5160` when fall bounce velocity is nonzero, second ground impact `5170`, then liedown/get-up `5110 -> 5120 -> 0`. `fall.yvelocity` defaults to `-4.5` and `fall.recovertime` defaults to `4` when omitted, matching the official docs. Characters missing bounce frames fall back to `5110` or idle.
- Dizzy actions `5300`/`5301` are treated as hittable common states. The runtime clears lingering `HitBy`/`NotHitBy` filters while those actions are active and releases to idle after the common dizzy window if character CNS does not self-state first.

## Remaining Common-State Gaps

- Air guard continuity `140 -> 141 -> 142` is still not a full guard system because player air guard input and guard-distance checks are not implemented yet.
- Command-driven air/fall recovery is still simplified. The runtime currently auto-recovers when `fall.recover` and `fall.recovertime` allow it; it does not yet require the M.U.G.E.N recovery command window.
- Character-defined recovery CNS states that rely on unsupported triggers/controllers, such as `gethitvar(...)` or `palfx`, still need those systems before they can behave exactly like M.U.G.E.N. `HitBy`/`NotHitBy` now have first-pass timed attr filtering, and `Turn` is supported as a runtime facing controller.
- Downed-hit behavior now chooses `5080`/`5090` when available, but complete OTG rules still need full hitflag and target-state support.
