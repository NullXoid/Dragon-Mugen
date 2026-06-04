# Compatibility Audit

This document tracks local compatibility checks against real M.U.G.E.N content.

## Audit Scope

This audit is a load-level/static compatibility record unless a row or section explicitly says it was live-tested. Passing this audit means files resolved and/or parsed enough to load through the current Dragon runtime subset. It does not prove full gameplay behavior, visual parity, animation timing parity, sound timing parity, controller correctness, or complete M.U.G.E.N runtime compatibility.

Use these evidence levels when describing compatibility work:

- `Parsed` - The file/controller/trigger is recognized and represented by the engine.
- `Executed` - Runtime code attempts to run the parsed behavior.
- `Live-tested` - The behavior was exercised in the running app.
- `Visually/timing accurate` - The observed behavior matches expected M.U.G.E.N timing and presentation closely enough for the current milestone.
- `Compatible with KFM` - Tested with Kung Fu Man content.
- `Compatible with Evil Ryu/Evil Ken` - Tested with those M.U.G.E.N 1.0 stress-test characters.

Static parser/load evidence should be recorded as `[VERIFIED-STATIC]` in `docs/FEATURE_LEDGER.md`. Do not mark runtime behavior as `[VERIFIED-LIVE]` until it is exercised in the running app and recorded in `docs/LIVE_VERIFICATION_MATRIX.md`.

## Reference Versions

Kung Fu Man is from the 2001 DOS M.U.G.E.N package and remains the baseline for the first playable sandbox.

Evil Ryu and Evil Ken are local stress-test characters. Their DEF files both declare:

```ini
mugenversion = 1.0
localcoord   = 320, 240
```

Their PotS update notes also mention MUGEN 1.0-specific cleanup, including removal of old AI activation methods obsolete in MUGEN 1.0. Both current DEF files include an Ikemen-only `movelist = movelist.dat` line, but their core combat files are M.U.G.E.N-style DEF/CMD/CNS/AIR/SFF/SND/ACT content.

## Documentation To Use

Use local 2001 docs for the KFM baseline:

```text
C:/Users/kasom/projects/mugen-official-archive/mugen-2001.04.14-dos/docs/
```

Use Elecbyte's M.U.G.E.N 1.0 docs for Evil Ryu/Evil Ken compatibility:

- https://www.elecbyte.com/mugendocs/upgrading.html
- https://www.elecbyte.com/mugendocs/tutorial1.html
- https://www.elecbyte.com/mugendocs/sctrls.html
- https://www.elecbyte.com/mugendocs/trigger.html

Important 1.0 notes:

- `mugenversion = 1.0` enables 1.0 behavior.
- `localcoord = 320, 240` defines the character coordinate space.
- M.U.G.E.N 1.0 state-controller and trigger docs include the controller/trigger families Evil Ryu and Evil Ken use heavily, such as `Helper`, `Explod`, `AILevel`, variable controllers, afterimages, palette effects, and projectile/helper-related triggers.

## Audit Command

```powershell
python engine/tools/audit_mugen_compat.py game
```

The audit reads `game/data/select.def`, resolves each selected character's DEF `[Files]`, and checks file presence, AIR/SFF references, CNS/CMD controller shape, trigger usage, and SND references against the current Dragon runtime subset.

## Current Results

Last checked with:

```powershell
python engine/tools/audit_mugen_compat.py game
```

| Character | Declared MUGEN | AIR Actions | SFF Images | CNS States | CNS Controllers | HitDefs | CMD Commands | SND Samples | Load-Level Status |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Kung Fu Man | `04,14,2001` | 105 | 253 | 98 | 492 | 34 | 31 | 12 | Pass |
| Evil Ryu | `1.0` | 341 | 1119 | 382 | 4181 | 189 | 136 | 79 | Pass with warnings |
| Evil Ken | `1.0` | 469 | 1225 | 429 | 4384 | 179 | 138 | 87 | Pass with warnings |

Evil Ryu and Evil Ken both load through Dragon's selected-character path. That proves roster placement, DEF `[Files]` resolution, SFF v1/PCX/ACT loading, AIR parsing, CNS/CMD text parsing, and SND archive parsing. It does not prove full gameplay compatibility yet.

## Live Verification Results

The Playable Core Proof verifier adds live runtime evidence through internal symbolic input. This does not prove physical keyboard or controller mapping.

Commands:

```powershell
build\dragon_mugen.exe --verify kfm-baseline
build\dragon_mugen.exe --verify evilken-smoke
```

Results:

| Scenario | Character | Mode | Status | Evidence |
| --- | --- | --- | --- | --- |
| `kfm-baseline` | Kung Fu Man | Training | Pass with optional partial | `SUMMARY pass=11 partial=1 fail=0 blocked=0`; proved controllable idle, held movement, crouch, jump, standing normal `y` to state `210`, crouching normal `y` to state `410`, hit contact, P2 life `1000 -> 943`, hitpause, active hit sound, and clean exit. |
| `evilken-smoke` | Evil Ken | Single Player | Pass | `SUMMARY pass=7 partial=0 fail=0 blocked=0`; proved load/idle, movement, jump, normal `x` to state `206`, timer stability, combo/hit evidence, and clean exit. |

The KFM verifier found and fixed two compatibility blockers during this pass:

- New state entries must preserve `stateTime = 0` for the entered state's first tick so authored `Time = 0` controllers can fire.
- KFM walking must use M.U.G.E.N common walk state `20` and parsed `const(velocity.walk.fwd.x/back.x)` values instead of a hardcoded state `0` velocity.

## Covered Compatibility Areas

These items are covered enough for the current prototype and should remain regression targets:

- `game/data/select.def` is the active roster authority.
- Selected character DEF `[Files]` entries are resolved for CMD, CNS/ST, `stcommon`, AIR, SFF, SND, and ACT palettes.
- KFM, Evil Ryu, and Evil Ken can be parsed and loaded without hardcoded KFM paths.
- SFF v1 PCX sprites, ACT palettes, AIR animation frames/collision boxes, and SND archive metadata load for all active test characters.
- Basic `State -1` command routing works for simple `ChangeState` blocks.
- CMD `[Command]` definitions are buffered from live input for buttons, directions, simple directional sequences, simultaneous `+` steps, hold `/` tokens, release `~` tokens, and broad `$` direction tokens. Character `[Defaults] command.time` and `command.buffer.time` are parsed and used by the matcher. Button release atoms such as `~x`/`~a` now require a real release transition; direction release atoms retain compatibility with common M.U.G.E.N motion-prefix commands such as `~D, DF, F, x`. Dragon also applies a documented player-input leniency on terminal attack-button steps: a cardinal final direction such as `F + x` can match the corresponding diagonal such as `DF + x`, so authored QCF specials remain playable when the punch/kick is pressed before the stick fully leaves the diagonal. `FF` and `BB` dashes are routed through the original command definitions and State -1 entries.
- CMD command gates now support `triggerall` and `trigger1` `command = "..."`, `command != "..."`, positive command OR groups such as `command = "QCF_x" || command = "QCF_y"`, basic `statetype = ...`, basic `statetype != ...`, `ctrl`, `movecontact`, `movehit`, `moveguarded`, literal integer `stateno`, `time`, `power`, `roundstate`, and `AILevel` comparisons, inclusive integer range gates such as `stateno != [100, 101]` used by dash/run commands, and first-pass `powermax`, `hitcount`, `numtarget`, `numhelper`, `numproj`, and `numprojid(...)` gates. State -1 `ChangeState value` can be an expression, including common M.U.G.E.N button-strength branches such as `ifelse(command = "DP_z", 1700, ifelse(command = "DP_y", 1600, 1500))`. Expression gates now include first-pass `p2dist`, `p2bodydist`, `p2stateno`, `p2statetype`, `p2movetype`, `frontedgedist`, `backedgedist`, `enemynear, ...`, and `helper(id), ...` support.
- CNS movement/state controllers now include parsed `ChangeState`, `ChangeAnim`/`ChangeAnim2`, `Helper`, `DestroySelf`, `BindToParent`, `ParentVarAdd`, `PowerAdd`, `LifeAdd`, `HitAdd`, `Trans`, `AfterImage`, `AfterImageTime`, `VelSet`, `VelAdd`, `VelMul`, `PosSet`, `StateTypeSet`, `Width`, `PlayerPush`, `HitBy`, `NotHitBy`, `HitOverride`, `TargetState`, `TargetBind`, `TargetDrop`, `TargetFacing`, `TargetLifeAdd`, `TargetPowerAdd`, `TargetVelAdd`, `TargetVelSet`, `Turn`, and `Null` for literal values plus selected character `Const(...)` values. `ChangeState`/`SelfState` and animation-end ChangeState paths preserve expression-backed `value` fields and evaluate them when the controller fires. `PowerAdd` uses `[Data] power` as `powermax`, clamps meter, and exposes `power`/`powermax` to triggers; `HitAdd` and successful `HitDef` contacts update the fighter's authored `hitcount`. `Trans` supports normal/add/addalpha blending and alpha source values for actors. `AfterImage`/`AfterImageTime` create practical additive actor trails from authored `time`, `length`, `timegap`, `framegap`, and `trans` values; detailed palette remapping fields are not yet exact. `Helper` creates owned helper actors with authored `id`, `stateno`, `pos`, `postype`, `sprpriority`, and facing basics; helpers run CNS state controllers, draw as actors, can hit the owner's opponent through normal `HitDef`, can bind to the parent, can add to parent variables, and can remove themselves through `DestroySelf`. `ChangeAnim`/`ChangeAnim2` can use full trigger groups when available, including expression triggers such as `alive` and `selfanimexist(...)`, so authored custom animation switches such as dizzy states do not need runtime hardcoding. `HitBy`/`NotHitBy` apply timed filters against parsed `HitDef attr` data, covering recovery and temporary invulnerability states without hardcoding per-character rules. `HitOverride` can route matching incoming hits into defender-owned states without granting an attacker target; target controllers use the stored normal-hit target keyed by authored `HitDef id`. `TargetLifeAdd`, `TargetPowerAdd`, `TargetVelAdd`, and `TargetVelSet` evaluate current-subset expressions when their controller fires, but still operate on Dragon's current single stored target link. `PlaySnd` supports channel/loop/persistent fields, and `StopSnd` can stop channel-owned sounds. Sound controllers run in M.U.G.E.N special-state order (`State -3`, `State -2`, then current state) while preserving authored `PlaySnd`/`StopSnd` order inside each state. Supported triggers include `1`, `!time`, `!animtime`, `time`/`animtime` comparisons and ranges, `vel`/`pos` comparisons, `statetype`, `AnimElem`, `AnimElemTime`, and `movecontact`. Default persistence runs every active tick; `persistent = 0` is one-shot per state entry.
- `Projectile` has first-pass runtime support for old-style CNS projectile blocks. It spawns owned AIR actors, moves them by authored velocity, draws them in the world, applies parsed hit/guard damage through the same first-pass hit reaction path as `HitDef`, and exposes `numproj`, `numprojid(...)`, `projhit####`, `projcontact####`, `projcontacttime(...)`, and `projguardedtime(...)` for follow-up effects. It now parses practical projectile lifecycle/movement/presentation fields: `projremove`, `projremovetime`, `projcancelanim`, `projpriority`/`priority`, `projmisstime`, `pausemovetime`, `supermovetime`, `projedgebound`, `projscreenbound`, `projstagebound`, `projheightbound`, `postype`, `offset`, `velocity`, `remvelocity`, `accel`, `velmul`, `projscale`, and `projshadow`. Runtime projectiles resolve variable-backed spawn/movement/scale expressions at creation time, respect owner pause/superpause move time, remove at parsed edge/stage/height bounds, apply removal velocity/animations, let finite hit/remove/cancel animations finish by AIR length, scale projectile collision boxes to match `projscale`, and resolve documented simple projectile priority: equal priority cancels both, higher priority cancels lower and decrements. Still missing: exact shadow shape/color parity and deeper edge cases around looped remove animations.
- `MakeDust`, `BindToRoot`, and `VarRangeSet` are implemented from the official M.U.G.E.N `sctrls.txt` definitions. `MakeDust` supports `pos`, `pos2`, and `spacing` and uses `game/data/fightfx.air` action `120` for floor dust. `BindToRoot` supports documented helper-only `time`, `facing`, and `pos` binding. `VarRangeSet` supports int `value` or float `fvalue` with documented `first`/`last` ranges.
- Common walk state `20` is used for held horizontal movement when available, and `const(velocity.walk.fwd.x)` / `const(velocity.walk.back.x)` resolve from the character `[Velocity]` section. This fixes KFM walking through the authored common movement path instead of hardcoded state `0` velocity.
- `AttackDist`, `AttackMulSet`, and `DefenceMulSet` are implemented from the official M.U.G.E.N `sctrls.txt` definitions. `AttackDist` changes the current player's state-local guard distance used by active `HitDef` guard checks, falling back to `HitDef guard.dist` or `[Size] attack.dist`. `AttackMulSet` scales damage the player gives, and `DefenceMulSet` applies the documented reciprocal defense multiplier to incoming hit and guard damage, including projectile damage.
- `AngleSet`, `AngleAdd`, `AngleMul`, `AngleDraw`, and `Offset` are implemented from the official M.U.G.E.N `sctrls.txt` drawing definitions. The stored draw angle is reset on state entry, `AngleDraw` applies rotation for the active tick around the player axis, and `Offset` shifts display for the active tick without moving the collision/body axis. Exact rotation parity may still differ where SDL's texture transform differs from M.U.G.E.N's renderer.
- `ForceFeedback`, `GameMakeAnim`, `DisplayToClipboard`, and `AppendToClipboard` are implemented from the official M.U.G.E.N `sctrls.txt` definitions. `ForceFeedback` maps documented waveforms to SDL gamepad rumble for the assigned player pad. `GameMakeAnim` spawns fightfx runtime effects and supports expression-backed `value`, `pos`, `under`, and simple random spread. Clipboard controllers store formatted debug text on the player and show it in Dragon's debug readout; formatting supports the documented `%d`, `%f`, `%%`, `\n`, and `\t` basics with up to five evaluated params.
- `VictoryQuote` and `RemapPal` are implemented from Elecbyte's M.U.G.E.N 1.0/1.1 state-controller docs. `VictoryQuote` stores the selected quote index, reads `[Quotes] victoryN` strings from the loaded CNS files, and displays the winner's selected quote on Dragon's match result screen. `RemapPal` evaluates and stores active palette mappings, including expressions such as `palno`; visible palette swapping is still limited because the current sprite texture cache is built from one loaded ACT palette instead of all SFF palette groups.
- Round initialization now follows the official M.U.G.E.N `updates.txt` and `common1.cns` path: fighters enter state `5900` at the start of each round when it exists, otherwise fall back to state `0`. The round-start phase updates CNS controllers so common/custom `5900` can clear variables, route to `190`/intro states, or return to `0` before fight control begins. `RoundState` now reports pre-fight/fight/over values for authored triggers, and `TeamMode` currently reports single mode.
- Stage `[Bound] screenleft`/`screenright` are parsed and applied as default camera-visible player constraints using loaded character `[Size]` edge widths. Basic `ScreenBound` controllers are parsed for literal `value` and `movecamera`, so `value = 0` can opt a state out of the default screen clamp for one tick.
- Ground player push uses loaded `[Size]` widths by default and honors simple one-tick `Width`/`PlayerPush` controllers so normal states do not walk through each other while dodge/pass-through states can opt out.
- The air/ground command blocker that caused jump attacks to clip into standing attacks is covered by the `statetype =/!=` gate support.
- The engine-owned state-zero jump path now preserves takeoff direction and applies the common CNS state `50` peak-action rule (`Anim + 3` when that AIR action exists), covering Evil Ryu/Evil Ken diagonal flip actions without altering their character files.
- Single Fight now reads core `[Round]` fields from `game/data/fight.def`: `match.wins`, round-start display timing, control delay, KO/DKO/Time-Over display timing, `over.waittime`, double-KO hit grace, `over.wintime`, win-message timing, and round-over timing. It also reads `[Combo]` hit-counter placement/timing/text settings. The round finish stays in-world until parsed `over.time`, applies available selected-character win/lose/draw poses, then advances to the Dragon-only final match result menu documented as an extension.
- HitDef fall fields now have first-pass runtime behavior for `fall`, `air.fall`, `fall.animtype`, recovery, `fall.damage`, `fall.xvelocity`, `fall.yvelocity`, and `yaccel`. `HitDef` and old-style `Projectile` contact resolve expression-backed `damage`, `guard.damage`, `pausetime`, spark numbers/offsets, hit/guard sound pairs, `envshake.*`, `fall.envshake.*`, `palfx.*`, ground/air hit times, ground/air velocities, guard velocity, and fall fields at hit time using the attacker expression context and the defender as `p2`/`enemynear`; this covers authored forms such as `fall.recover = (var(10) <= 0)` without flattening them during parse. Hit/guard sound parsing accepts M.U.G.E.N `S` selected-character and `F` common/fight sound prefixes. The official common-state fall controllers `HitFallDamage`, `HitFallVel`, and `HitFallSet` are parsed and executed, so fall damage and bounce velocities can be authored through M.U.G.E.N CNS instead of engine-owned hardcoding. Fall-causing hits route through loaded common fall/land actions instead of returning directly from hit shake to standing recovery. `GetHitVar(...)` now exposes the stored defender hit snapshot for common-state expressions such as `animtype`, `groundtype`, `airtype`, `slidetime`, `hittime`, `ctrltime`, `xvel`, `yvel`, `yaccel`, `fall`, `fall.recover`, `fall.recovertime`, `fall.damage`, `fall.xvel`, `fall.yvel`, and `hitcount`; `target, GetHitVar(...)` and modulo expressions are supported for the current single stored target link.
- Audio `PlaySnd`/`StopSnd` controllers now fail closed when their trigger lines cannot be fully parsed. This avoids playing a sound from only the supported part of a richer author condition.

Covered means the engine has useful prototype behavior for these cases. It does not mean every equivalent M.U.G.E.N expression form is supported.

## Current Warnings

- Evil Ryu has 10 AIR sprite references not present in its SFF.
- Evil Ken has 8 AIR sprite references not present in its SFF.
- Evil Ryu has 1 parsed sound reference not found in selected character/common/fight SND archives.
- Evil Ken has 3 parsed sound references not found in selected character/common/fight SND archives.
- These are warnings, not load blockers. The current runtime skips missing optional frames/sounds.

## Main Compatibility Gaps

Evil Ryu and Evil Ken now load without unsupported state-controller families in the current audit, but several M.U.G.E.N runtime semantics still need more exact behavior.

Highest-impact controller/runtime gaps:

- multi-target target controller behavior beyond the current single stored target link
- exact throw/cinematic target binding parity

Highest-impact trigger/expression gaps:

- Runtime AI behavior beyond literal `!AILevel`/`AILevel = 0` gates
- deeper `var(...)`/`fvar(...)` usage, redirection, and assignment timing parity
- full range-expression support, non-literal `roundstate`/`power` forms, and additional random-dependent branches
- exact `numtarget`/target lifetime parity and multi-target behavior
- deeper `helper(...)`/`enemynear` redirection forms beyond the first-pass expression evaluator
- exact `p2bodydist` body-width math, edge-distance parity, and full `ScreenBound movecamera` camera-follow behavior
- richer `AnimElemTime(...)`/`AnimElem` expression forms beyond the current direct comparisons
- expression-backed controller and HitDef values beyond the controllers already covered, especially variable-backed `x`/`y`, `anim`, `stateno`, and exact SFF palette remap behavior
- exact projectile shadow shape/color parity and deeper edge cases around looped projectile remove animations

The command audit shows why the current characters can load but are not fully playable:

- Evil Ryu and Evil Ken have many `State -1` `ChangeState` blocks that now pass the buffered-command entry shape, including expression-valued target states, but many still depend on variables, helpers, projectiles, target state, and richer expression forms that are outside the current runtime subset.
- The current candidate count should be checked with `python engine/tools/audit_mugen_compat.py game` after each parser/runtime expansion instead of treated as a fixed target.

The recent CMD work covers the most obvious air/ground attack mix-up, basic trigger1 command routing, command buffering, simple OR command groups, literal and expression target states, numeric gates, and first-pass helper/projectile count checks. It does not cover full trigger group semantics, redirection, advanced variables, advanced helper/projectile checks, or full AI behavior.

## Next Compatibility Steps

1. Keep KFM as the regression baseline.
2. Expand the trigger/expression evaluator for common `State -1` patterns in KFM, Evil Ryu, and Evil Ken, especially variables, ranges, random, enemy distance, and redirection.
3. Implement variable and expression-backed controller values, plus remaining movement-adjacent controllers such as `SprPriority`, `PosFreeze`, and edge-distance-aware `ScreenBound` with full `movecamera` behavior.
4. Improve remaining visual-effect support, especially exact palette math.
5. Expand helper/projectile runtime support after the expression/controller layer is stable.
6. Rerun `audit_mugen_compat.py` after each parser/runtime expansion.

## Pause/SuperPause Coverage

`Pause` and `SuperPause` now have first-pass runtime support. The engine reads `time`, `movetime`, and simple numeric `sound` values, freezes both fighters globally, and lets only the owning fighter continue while its `movetime` remains active. Round reset, training reset, and match reset clear the global pause state.

Still missing compared with full M.U.G.E.N behavior:

- visual darkening/animation layers used by some `SuperPause` effects
- `poweradd` and richer pause parameters
- expression-backed `sound` parameters beyond the current simple numeric form
- exact helper/projectile-specific pause interaction

## Explod Coverage

`Explod`, `ModifyExplod`, and `RemoveExplod` now share the same runtime effect model. Active explods store the M.U.G.E.N `id`, owner fighter, animation, `bindtime`, `removetime`, `sprpriority`, and simple numeric `scale`. `RemoveExplod` removes matching effects by owner/id, and `ModifyExplod` can update matching priority and scale.

Still missing compared with full M.U.G.E.N behavior:

- expression-backed `pos`, `scale`, and other controller values beyond the current numeric parser fallback
- full `postype` coverage for bound effects after spawn, especially `p2`, `front`, and `back`
- transparency/blend options such as `trans`
- helper-owned explods, once helpers exist

## EnvShake Coverage

`EnvShake` and `FallEnvShake` now have first-pass runtime support. The engine parses standalone `EnvShake` controllers and HitDef `envshake.time`, `envshake.freq`, `envshake.ampl`, and `envshake.phase`; HitDef and Projectile contact resolve those fields as expressions at hit time. HitDef `fall.envshake.*` values are carried into the defender hit runtime; `FallEnvShake` common-state controllers and the internal fall-impact path both fire that stored shake once. The shake is applied as a temporary world viewport offset, leaving HUD/debug text stable.

Still missing compared with full M.U.G.E.N behavior:

- exact M.U.G.E.N shake waveform matching
- helper-owned environment shake once helpers exist
- any stage-specific masking or per-layer shake behavior

## Palette/Screen Color Coverage

`PalFX`, `BGPalFX`, and `EnvColor` now have first-pass runtime support. Character `PalFX` parses numeric `time`, `add`, `mul`, `sinadd`, `color`, and `invertall`; HitDef and Projectile `palfx.*` values preserve expressions and apply resolved values to the defender on contact. `BGPalFX` runs as a timed background overlay, and `EnvColor` runs as a timed full-screen flash before HUD rendering.

Still missing compared with full M.U.G.E.N behavior:

- exact palette math for `color` and `invertall`
- exact per-palette SFF remapping and palette group switching
- precise BGPalFX per-layer palette modification instead of first-pass overlay tinting
- helper-owned palette effects, once helpers exist
