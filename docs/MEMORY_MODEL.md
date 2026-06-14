# Memory Model

This is the asset and runtime memory plan for Dragon MUGEN. It exists early because M.U.G.E.N-style content can become large quickly: many characters, many palettes, stage layers, sounds, storyboards, and later RPG equipment/campaign data.

## Principles

- Load definitions first, heavy binary assets second.
- Keep creator-facing folders simple; keep cache behavior inside the engine.
- Do not decode every sprite for every installed character at startup.
- Do not hold duplicate decoded copies of the same asset unless there is a measurable reason.
- Separate CPU-side decoded assets from GPU-side renderer resources.
- Make ownership explicit with RAII types and handles.

## Startup Loading

At startup:

- Read `game/data/select.def` for the active character and stage roster.
- Resolve listed character entries to lightweight DEF metadata under `game/chars`.
- Resolve listed stage entries to lightweight DEF metadata under `game/stages`.
- Read global config data from `game/data`.
- Read Dragon-only sidecars only as optional extension metadata; M.U.G.E.N files remain authoritative for compatible content.
- Read local player settings from `game/save`, not from character or stage folders.
- Do not decode all SFF/SND files yet.

Current implementation follows this: mode select, character select, and stage select are driven by `select.def` metadata. Folder scanning is only a fallback if `select.def` is missing or empty.

Current code ownership: `engine/src/MugenData.cpp` owns roster loading, stage loading, selected character DEF `[Files]` resolution, and selected character `[Size]`/`[Data]`/`[Velocity]`/`[Movement]` constants. `engine/src/FightData.cpp` owns the current `game/data/fight.def` round, combo, and powerbar settings subset. Screen flow code must ask these modules for metadata instead of rebuilding M.U.G.E.N path or fight-setting rules directly in `App.cpp`. Future extraction should keep following content ownership: system/motif UI parsing belongs with a system data module, character runtime data belongs with a character/session module, and rendering resources belong behind the renderer/cache layer.

`engine/tools/guard_architecture.py` enforces the current ownership boundary during the CMake build. It checks that required M.U.G.E.N runtime folders still exist, that `MugenData` and `FightData` stay compiled, and that app-layer files do not regain `select.def`/`fight.def`/`chars`/`stages` path rules. `engine/tools/guard_active_change.py` enforces preservation documentation for engine/app code changes.

## Mode Loading

When entering a fight mode:

1. Resolve the selected `select.def` character entry and selected stage.
2. Load roster select icons from character DEF `[Files]` `sprite` plus palette data only.
3. When entering VS/fight preparation, load the selected character DEF `[Files]` entries: `.air`, `.cmd`, `.cns`/`st`, `stcommon`, `.sff`, `.snd`, and `.act`.
4. Load stage definition: `.def`.
5. Load binary asset archives lazily:
   - character `.sff`
   - character `.snd`
   - stage `.sff`
   - palettes `.act`
6. Decode only sprites needed for the current screen/animation at first.

## Asset Cache Targets

The engine should eventually have cache layers like:

```text
TextDocumentCache
  DEF/AIR/CMD/CNS parsed documents

SpriteArchiveCache
  SFF headers and subfile metadata

SpriteFrameCache
  decoded RGBA frames or palette-indexed surfaces

TextureCache
  GPU textures created from decoded sprites

SoundArchiveCache
  SND metadata and decoded sound buffers
```

## Lifetime Rules

- Menus own only lightweight metadata.
- Stage confirmation first shows the VS screen using select-safe roster portraits and stage metadata. After the VS frame is presented, the engine prepares the selected fighter runtime, selected stage background, camera, round state, and fighter start positions. Entering fight view must use that prepared session and must not reload/reset again.
- Fight view owns a fight-session asset set.
- Leaving the fight releases session-only assets.
- Frequently reused global UI assets may remain cached.
- Project content and local settings have different lifetimes. Character/stage/data files are content; `game/save` files are local mutable state.

## Current Fight Runtime

The current runtime keeps character selection lightweight. The character select list comes from `game/data/select.def`, and the select/VS screens load only each roster entry's select portraits (`SFF` group `9000,0` icon and `9000,1` face) through the character DEF `[Files]` `sprite` and palette entries. The full selected character DEF `[Files]` runtime - CMD, CNS/ST, AIR, SFF action clips, SND, palettes, shared `stcommon`, and the selected stage background - is loaded only after the VS screen has been presented, then unloaded when the player backs out to fighter/stage/mode selection.

Current fight-session behavior:

- Character clips are loaded from the selected character AIR actions only for the active VS/fight session. KFM currently loads 105 actions; Evil Ryu loaded 335 rendered actions in the local compatibility test; Evil Ken loaded 465 rendered actions.
- Common hit and guard spark clips load from `game/data/fightfx.air` and `game/data/fightfx.sff`; they do not belong in character folders.
- Character and fight sound samples load from the selected character SND first, then `game/data/common.snd` and `game/data/fight.snd` as fallbacks. Character sounds stay with the character; shared/common fight sounds stay in `game/data`. Decoded samples are scheduled as mixed SFX voices instead of being appended as full queued buffers, so rapid attacks overlap/cap cleanly instead of building a delayed repeated-sound backlog. Stage/system BGM fields are parsed as content metadata only; the current mixer is SFX-only and does not start a music stream.
- Shared-palette character sprites use the selected ACT palette with reversed SprMaker/M.U.G.E.N palette order.
- Fighter origins use stage `[PlayerInfo]` starts and `[StageInfo] zoffset`.
- Fighter body widths are loaded from the selected character `[Size]` section. Fighter origins are clamped to the camera-visible play area by default using the selected stage `[Bound] screenleft`/`screenright`, the character edge width, and `[PlayerInfo] leftbound`/`rightbound`. This prevents normal movement, player push, hit reactions, and round-finish slides from leaving the view when the camera has reached its stage bounds.
- P1 walk/jump/crouch movement is engine-owned until common CNS movement states are parsed; P2 idles or guards until a parsed hit routes it into get-hit or guard-hit behavior. The engine-owned jump path records the AIR action chosen at takeoff (`41`, `42`, or `43`) and mirrors common CNS state `50` by switching to `Anim + 3` near the jump peak when that AIR action exists, so characters such as Evil Ryu/Evil Ken can use `45`/`46` flip arcs without character-specific code.
- Keyboard `A/S/D` map to M.U.G.E.N punch commands `x/y/z`; keyboard `Z/X/C` map to kick commands `a/b/c`. The selected character CMD `[Command]` sections are buffered from live input and then matched by CMD `State -1` entries to route into CNS states; KFM's currently tested standing buttons route to states `200`, `210`, `230`, and `240`, and Down plus those buttons can route to crouch normal states `400`, `410`, `430`, and `440`.
- Keyboard Space maps to the M.U.G.E.N `s` command for P1, keyboard Semicolon maps to `s` for the local P2 slot, and SDL gamepad Start also feeds `s`. This keeps CMD files that use start/select style commands on the same input-buffer path as attack buttons.
- SDL3 standard gamepad input is layered over the same command names. Xbox `X/Y/LB` and PlayStation `Square/Triangle/L1` map to `x/y/z`; Xbox `A/B/RB` and PlayStation `Cross/Circle/R1` map to `a/b/c`; D-pad or left stick maps to the buffered direction steps and directional hold commands.
- Single Player and VS/local have separate runtime UI/rules paths from Training. P1 uses arrows plus `A/S/D` and `Z/X/C`; VS/local also gives the `LocalP2` slot `I/J/K/L` movement plus `U/O/P` and `N/M/,`, still mapped to M.U.G.E.N `x/y/z/a/b/c` command names.
- Mode select and character select use the M.U.G.E.N-style motif assets from `game/data/system.sff` and roster select portraits only; they do not load character AIR/CNS/CMD/SND runtime data.
- Character select currently owns only the P1 roster choice. Confirming that choice writes `sessionSlots.p1Character`; the opponent side is mode-owned through `sessionSlots.opponentType` (`DUMMY` for Training, `CPU` for Single Player, `LocalP2` for VS/local). The opponent character index remains empty until a separate P2/opponent selection flow is implemented.
- Match modes run a main-menu-configurable timer, M.U.G.E.N `game/data/fight.def` `match.wins` scoring, tie-round replay, parsed round-start/KO/time-over/win timing, parsed `over.waittime`, `over.hittime`, `over.wintime`, and `over.time`, parsed `[Combo]` hit-counter timing/placement/text, first-pass `[Powerbar]` gauge positions/ranges, round reset, and a held match-result menu. VS/local runs bidirectional hit checks and local P2 controls; Single Player currently uses a non-attacking CPU placeholder until AI/opponent runtime work lands. Training keeps dummy behavior, dummy guard/invincibility/freeze options, command-training move list, hit counters, and debug overlays.
- The match flow is `RoundStart -> Fight -> RoundFinish -> RoundResult -> MatchResult`. `RoundFinish` shows the M.U.G.E.N-style callout (`K.O.`, `Double K.O.`, `Time Over`), keeps the round alive until parsed `over.time`, applies winner/loser/draw poses after parsed `over.wintime` where the selected character defines states `181`, `170`, or `175`, and allows double-KO grace through parsed `over.hittime`. `RoundResult` scores the round and shows winner/draw plus pips; `MatchResult` is a Dragon/Flying-Dragon-inspired post-match decision screen for rematch, fighter select, stage select, or mode select.
- P1/P2 auto-face each other.
- Ground player push uses loaded `[Size]` widths by default and honors simple CNS `Width`/`PlayerPush` one-tick controllers. `Width value = front,back` and literal/`Const(...)` `edge` or `player` pairs adjust screen-edge and player-push widths for the active tick. `PlayerPush value = 0` lets a state opt out of push checking for that tick.
- Stage camera data is parsed from `[Camera]`.
- Background layers apply `[BG ...] delta` against the current camera position.
- `F1` toggles hitboxes quickly, `F2` opens training options, and `R` resets the fight session.
- In match modes, `F2`/Escape opens a match pause menu instead of the Training options menu. That menu can resume, restart the match, return to fighter select, return to stage select, or return to mode select.
- Main menu `OPTIONS` currently exposes match timer length, canvas size, Dragon UI scale, controller label style, and P1/P2 gamepad assignment. Canvas size changes the SDL logical presentation and fight camera width without changing M.U.G.E.N character/stage file placement; UI scale changes Dragon floating panel and callout density. This is Dragon-only runtime settings UI until equivalent M.U.G.E.N-compatible settings are parsed from `game/data` and local preferences are persisted in `game/save/settings.def`.
- Training options control hitboxes, origins, floor line, debug readout, hit flash, hit sparks, hit log, hit sound, dummy invincibility, dummy life auto-restore, dummy freeze, dummy guard mode, guard damage, P2-controlled training, command HUD, input HUD, training power mode, move-list category, character move-list display, and position reset. `POWER = MAX` holds both fighters at the selected character's parsed `[Data] power` maximum so super routes can be tested without editing the character CNS. When P2-controlled training is on, the active opponent type becomes `LocalP2`; dummy-only options stop applying until the option is turned off.
- AIR Clsn1 and Clsn2 boxes are attached to animation frames, including `Clsn1Default`/`Clsn2Default` handling.
- Common hit and recovery mapping uses loaded AIR actions with fallbacks for incomplete framesets: guard ready/end `120-132`, crouch get-hit `5020-5027`, air get-hit/recovery `5030`, `5040`, `5140`, `5200`, `5210`, bounce/ground-impact `5080`, `5090`, `5160`, `5170`, and wake-up `5100/5101 -> 5110 -> 5120 -> 0`. Remaining common action gaps are tracked in `docs/ANIMATION_MAPPING_AUDIT.md`.
- CNS `HitDef` sections are parsed by state number and assigned runtime IDs. The current trigger subset supports `Time = n`, `AnimElem = n`, and KFM-style `p2bodydist X` near/far split checks. Runtime hit application is keyed by controller plus current animation element so multi-hit states can connect again on later elements.
- Unguarded hits increment a per-attacker combo counter. Active combo count resets when the defender leaves hit/guard recovery, while the displayed count remains for parsed `fight.def` `[Combo] displaytime`. Guarded hits do not increment the counter.
- The training move-list page is styled as a command-training screen: a highlighted technique list, selected-move detail panel, and command strip. Its content is derived from the loaded character CMD `State -1` entries, including positive command OR groups, filtered to states whose current AIR/CNS data is loaded, and category-filtered as all, normal, special, or super moves. No separate move-list data file is required for the first KFM pass.
- The in-fight training command HUD is Flying Dragon-inspired: it shows recent P1 input tokens and a compact set of executable CMD-derived moves without requiring a separate move-list data file.
- CNS `Statedef` sections provide state metadata such as `anim`, `ctrl`, `velset`, `type`, `movetype`, and `physics`.
- Simple CNS `ChangeState`/`SelfState` controllers with literal target `value` are parsed for animation-end transitions and the current trigger subset, including optional `ctrl`. This covers `AnimTime = 0`/`!AnimTime`, timed state changes such as `Time = n`, command-triggered state branches, velocity/position landing checks, edge-distance checks, and direct `AnimElem`/`AnimElemTime` comparisons.
- Simple CNS `PlaySnd` controllers with `value = group,index` are parsed against the same trigger subset as runtime movement controllers, using the same character/common/fight sound archive lookup as hit and guard sounds. The `F` common-sound prefix, `channel`, `lowpriority`, `volume`, `loop`, and `persistent` fields are represented in the mixer subset; channel playback replaces the previous voice unless `lowpriority` blocks interruption. CNS `StopSnd` controllers stop parsed channels. `PlaySnd` and `StopSnd` execute through one ordered audio-controller pass using M.U.G.E.N's special-state order (`State -3`, `State -2`, then current state), so global landing/intro/loop cleanup sounds and active-state sounds run in the authored order. Audio controllers with trigger lines now fail closed when the full trigger expression is outside the supported parser subset; they do not fall back to partial `Time`/`AnimElem` matches.
- Simple CNS `CtrlSet` controllers with `value = 0/1` use the runtime trigger subset, so `Time` and `AnimElem` control-restoration patterns work for KFM normals and specials.
- Simple CNS `PosAdd` controllers use the runtime trigger subset, including multiple `triggerN` entries on one controller, with X movement applied in the fighter's facing direction.
- Simple CNS `ChangeAnim` controllers can require `movecontact` plus `AnimElemTime(...)` comparisons and can jump to a target animation element. Custom target states now keep M.U.G.E.N-style animation ownership: `ChangeAnim` uses the target's own AIR data, while `ChangeAnim2` uses the custom-state owner's AIR data. If a `ChangeAnim2` victim animation is missing, Dragon falls back to common get-hit/fall actions instead of leaving the target on stale idle frames. This covers KFM standing strong punch state `210` and Lili's missing air-throw victim animation without character-file edits.
- CNS `VelSet`, `VelAdd`, `VelMul`, `PosSet`, `PosFreeze`, `HitVelSet`, `HitBy`, `NotHitBy`, `SprPriority`, `Explod`, `AssertSpecial`, and `StateTypeSet` controllers are parsed for plain numeric or selected `Const(...)` values and the current trigger subset. Runtime passes execute authored `State -3`, `State -2`, then current-state controllers for the supported controller families instead of limiting special-state order to audio only. Character `[Velocity] run.fwd/run.back` and `[Movement] yaccel` constants feed dash/backdash movement. Runtime controllers honor default per-tick persistence and `persistent = 0`, so KFM gravity/friction, custom hit-state movement, recovery protection, wall-freeze behavior, and simple special-move movement can come from CNS instead of app hardcoding.
- Parsed `HitBy`/`NotHitBy` controllers apply timed, attribute-based hit filtering from `value = SCA, ...` style fields. This is a M.U.G.E.N compatibility system, not a Dragon-only option; it lets authored recovery and temporary invulnerability states reject or accept matching `HitDef attr` values.
- Parsed `HitOverride`, `TargetState`, `TargetBind`, `TargetDrop`, `TargetFacing`, and `TargetLifeAdd` use the M.U.G.E.N 1.1 controller model for the first custom-target pass. `HitOverride` can route an incoming matching hit into a defender-owned state and does not grant the attacker a normal target link. Normal hits and guarded hits store a short-lived target reference keyed by the authored `HitDef id`; literal `TargetState value`, `TargetBind pos/time/id`, `TargetDrop excludeID`, `TargetFacing value/id`, and `TargetLifeAdd value/id/kill/absolute` can then affect that stored target. `TargetLifeAdd value` uses the current expression evaluator, so simple `floor(...)`, `fvar(...)`, and arithmetic damage formulas can run. Multi-target lists and full target redirection remain future work.
- Parsed `Turn` controllers flip the fighter facing when their runtime trigger fires. `Null` is recognized as a no-op controller so authored placeholder states do not appear as missing runtime behavior.
- Parsed `AssertSpecial` flags are stored per fighter for the current tick. Current runtime effects cover `Intro`/`RoundNotOver` round-end suppression, `TimerFreeze`, `NoBarDisplay`, `NoBG`, `NoFG`, `Invisible`, and `NoWalk`. `NoShadow`/`GlobalNoShadow`, guard-lock flags, `NoMusic`, `NoJuggleCheck`, and `Unguardable` are retained as parsed flags but need the corresponding systems before they can change behavior.
- M.U.G.E.N int/float variable banks are available per fighter: `var(n)`, `sysvar(n)`, `fvar(n)`, and `sysfvar(n)`. `VarSet`, `VarAdd`, and `VarRandom` support both `v`/`fv` plus `value`/`range` syntax and direct assignments such as `var(17) = 1`. The expression subset supports numeric arithmetic, comparisons, `&&`/`||`, `!`, `floor`, `ceil`, `abs`, `ifelse`, `Const(...)`, `random`, `AILevel`, `time`, `animtime`, `stateno`, `roundstate`, `life`, `p2life`, `statetype`, `pos`, `vel`, and `p2bodydist x`. This same expression subset is now used by CNS controller triggers and CMD `State -1` gates.
- Any loaded selected-character action with an active parsed CNS `HitDef` tests current Clsn1 boxes against P2 Clsn2 boxes, logs the matching `HitDef`, subtracts parsed damage, applies P1/P2 hit pause, spawns parsed `sparkno`/`sparkxy` from common fight FX, plays parsed `hitsound`, and routes P2 through common get-hit actions using parsed `animtype`, `ground.type`, `ground.hittime`, `ground.slidetime`, `ground.velocity`, `air.velocity`, `fall`, `air.fall`, `fall.animtype`, `fall.recover`, `fall.recovertime`, `fall.damage`, `fall.yvelocity`, and `yaccel`. Fall hits route through the common fall/land action families (`5050`/`5070`/`5071`/`5100`/`5160`/`5170`/`5110`) when those actions exist. `fall.recovertime` defaults to `4` and `fall.yvelocity` defaults to `-4.5` when omitted. Separate `HitDef` controllers in the same state can each connect once, so KFM state `410` can use its two `AnimElem`-triggered hits.
- Common dizzy actions `5300`/`5301` remain hittable in training and fight modes when they are used from known dizzy state numbers. The safety release no longer treats animation `5300` alone as dizzy, and it does not override active custom hit states.
- Holding Down currently uses the standard crouch action path `10 -> 11`; releasing Down exits through action `12` when those actions exist.
- Dummy guard mode cycles `OFF -> STAND -> CROUCH -> AUTO`. Stand guard holds `130` and routes guarded contact through `150 -> 151 -> 130`; crouch guard holds `131` and routes through `152 -> 153 -> 131`. `AUTO` visually holds stand guard but chooses crouch guard for low-only guard flags. The separate guard damage training option can suppress only the chip damage while preserving guard contact effects.
- Player-controlled ground guard now follows M.U.G.E.N-style back/down-back input before hit resolution: holding back can enter stand guard for guardable standing/mid attacks, and holding down-back can enter crouch guard for guardable crouching/low attacks. HitDef/Projectile `hitflag` filtering is enforced before contact resolution using M.U.G.E.N flag expansion (`M = H + L`, `A`, `F`, `D`, `+`, `-`) so attacks only affect legal defender states. Air guard is still a separate compatibility pass.
- Dummy invincibility still lets hits register, sparks render, and sounds play; it only prevents life loss. Dummy freeze keeps P2 idle/anchored while still allowing contact checks and FX/sound.

## Next Implementation Step

Promote the current direct app parsing into engine asset/session APIs. The app code already asks `MugenData` for roster, stage, character-file, and character-constant data, and `FightData` for the current `fight.def` round/combo/powerbar subset. The next splits should move system/motif DEF parsing, loaded character runtime assets, session state, and rendering caches behind explicit modules. The app code should ask for "selected character clip," "selected stage layer," "fight UI layout," and "current fighter frame" instead of parsing binary archives or compatibility rules directly inside screen drawing code.

Immediate follow-up work:

- Split `game/data/system.def` motif settings out of `App.cpp`, following the new `FightData` pattern for `game/data/fight.def`.
- Split selected character runtime loading into an owned fight-session object so P1, P2, helpers, projectiles, sounds, palettes, and stage assets have explicit lifetimes.
- Add parallax-specific and animated BG handling.
- Validate KFM special moves as the baseline, then expand the CNS/CMD subset into the unsupported variable/helper/projectile patterns exposed by Evil Ryu/Evil Ken.
- Replace the prototype-owned common get-hit routing with parsed `common1.cns` state-controller execution.
- Add richer trigger evaluation, broader sound/controller coverage, fall behavior, and richer reset rules.
- Extend first-pass `Pause`/`SuperPause` beyond global freeze timing into M.U.G.E.N-style visual darkening, animation, power, and helper/projectile interactions.
- Extend the runtime effect model beyond `Explod`/`ModifyExplod`/`RemoveExplod` basics into expression-backed positioning/scaling, `trans`, and helper-owned effects.
- Refine `EnvShake` from first-pass world viewport offset into exact M.U.G.E.N waveform/layer behavior if later visual tests show differences.
- Refine first-pass `PalFX`/`BGPalFX`/`EnvColor` into exact M.U.G.E.N palette math, expression-backed color values, and helper-owned visual effects.

## M.U.G.E.N Docs To Follow

Use the bundled 2001 M.U.G.E.N docs as the compatibility reference:

- `docs/overview.txt` - mode flow, file types, and runtime folder roles.
- `docs/air.txt` - animation actions, elements, 60 ticks per second, loopstart, flip/blend flags, and collision boxes.
- `docs/spr.txt` - required character sprites, recommended group numbers, and axis expectations.
- `docs/cns.doc` - finite state machine model, state-time/game-time, coordinates, velocity, and collision behavior.

Important rules from those docs:

- AIR frame durations are measured in 60 Hz game ticks.
- Character Y position uses ground as zero; negative Y means airborne.
- Character sprites are positioned by their SFF axis plus AIR offsets.
- Clsn1 is attack collision; Clsn2 is hittable body collision.
- CNS states are the behavior authority; rendering should reflect current state/action, not drive it.

## Current Rendering Findings

- KFM has M.U.G.E.N portrait sprites at SFF group `9000,0` and `9000,1`.
- KFM body sprites such as group `0,0` are marked as shared-palette sprites in SFF v1.
- Those body sprites use high palette indices such as `240-255`.
- The selected `.act` palette has the useful colors at the opposite end of the file, so M.U.G.E.N/SprMaker palette order must be handled explicitly instead of assuming direct index-to-RGB order.
- The embedded PCX palette on shared-palette body sprites is not reliable for final character colors.
- Background colors look reasonable because the stage sprite currently uses its embedded PCX palette.
- Stage rendering now reads `[BG ...]` elements and applies `spriteno`, `start`, `tile`, `mask`, `layerno`, and SFF axis placement.
- Stage rendering applies `delta` to the current camera and clamps camera movement to the stage camera bounds. Stage `[Bound] screenleft`/`screenright` are parsed for player screen-edge constraints, and selected-character `[Size]` plus one-tick `Width` controllers determine how much of the body may protrude.
- Basic CNS `ScreenBound` controllers are parsed for literal `value` and `movecamera` pairs. `value = 0` lets a state opt out of the default screen clamp for that tick. `movecamera` is stored but not yet fully honored by the camera solver, and richer edge-distance trigger expressions still need parser support.
- AIR collision boxes now render in the debug overlay: Clsn1 attack boxes in red and Clsn2 hurtboxes in blue.
- The first hit interaction is data-driven from KFM AIR/CNS but not yet a full M.U.G.E.N state-controller implementation.
- Evil Ryu and Evil Ken loaded through the selected-character path with SFF v1, ACT, AIR, CNS/ST/common, CMD, SND, and portrait resolution. This proves the file placement and selection path, but not full character behavior; variable expressions, helper-owned projectiles, old-style `Projectile` controllers, AI, supers, PalFX/afterimage, and MUGEN 1.0 edge cases remain the next parser/runtime work.
- Parallax, animated backgrounds, transparency modes beyond simple masking, and foreground/background ordering edge cases still need a proper stage renderer pass.
