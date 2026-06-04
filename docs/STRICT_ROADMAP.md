# Strict Roadmap

This file is the technical contract for Dragon MUGEN Core. If an implementation choice conflicts with this document, update this document first and explain why. No temporary platform or renderer paths should be added just to make a demo faster.

## Non-Negotiable Direction

- Language: C++20 or newer for the engine/runtime.
- Build: CMake.
- Platform layer: SDL3.
- Renderer: bgfx.
- Immediate content target: official 2001 DOS Kung Fu Man and Mountainside Temple stage.
- First playable mode: training/single-fight sandbox.
- Editor direction later: separate creator-facing editor, likely C# Avalonia or Qt, but not part of milestone one.
- Networking: out of scope.
- Engine shape: small early-M.U.G.E.N-style runtime first, expanded carefully.
- Runtime/content folder structure follows M.U.G.E.N's layout and names.
- Dragon-only features must be documented in [DRAGON_EXTENSIONS.md](DRAGON_EXTENSIONS.md) with their file location, section/key format, defaults, and compatibility impact.
- Compatibility audits for non-KFM test characters are tracked in [COMPATIBILITY_AUDIT.md](COMPATIBILITY_AUDIT.md).

## Explicitly Not Allowed

- No Win32-only app layer.
- No DirectX-only renderer.
- No Godot/Unity runtime pivot.
- No browser/web prototype as the main engine.
- No hand-authored fake KFM data when real KFM reference data is available.
- No menu polish before KFM/stage parsing and rendering are working.
- No RPG/shop/equipment work until the training sandbox can load, draw, and simulate KFM.
- No engine-owned content layout that hides M.U.G.E.N concepts behind unrelated folder names.

## Folder Structure Contract

The runtime/content root must look and feel like M.U.G.E.N. A creator should be able to understand where content belongs without learning an engine-internal layout.

The project README and runtime folder READMEs must point back to this roadmap so contributors know this layout is intentional, not incidental.

Repository layout:

```text
dragon_mugen_core/
  engine/                 # C++ engine source, not shipped as creator content
    include/
    src/
    tools/
  game/                   # runtime root, M.U.G.E.N-style
    chars/
    data/
    font/
    sound/
    stages/
    plugins/
    save/
  docs/
  build/
  CMakeLists.txt
  README.md
```

Runtime layout rules:

- `game/chars/<character>/` contains character `.def`, `.air`, `.cmd`, `.cns`, `.sff`, `.snd`, `.act`, and character storyboards.
- `game/stages/` contains stage `.def` and `.sff` files.
- `game/data/` contains `mugen.cfg`, `select.def`, `system.def`, `fight.def`, `common1.cns`, fight FX, and later engine ruleset files.
- `game/font/` contains font assets.
- `game/sound/` contains shared/global sounds.
- `game/plugins/` is reserved for future optional runtime extensions.
- `game/save/` is reserved for local profiles, progression, and settings.
- The engine may add new systems later, but they should extend this model rather than replace it.
- `game/data/select.def` is the roster/stage-select authority. Character folders may exist without becoming selectable.
- Dragon-only project defaults belong in `game/data/dragon.def`; Dragon-only character extensions belong beside the character as `<character>.dragon.def`; Dragon-only stage extensions belong beside the stage as `<stage>.dragon.def`; local user settings belong in `game/save/settings.def`.
- Every Dragon extension must keep the relevant M.U.G.E.N file authoritative. For example, stage background images stay in `game/stages/<stage>.def` and `game/stages/<stage>.sff`; the Dragon sidecar can add preview/training/editor metadata, but not replace the stage definition.
- Engine modules must preserve this folder model. Roster/stage/character file resolution belongs in the M.U.G.E.N data layer (`MugenData` today), and `fight.def` settings belong in fight data (`FightData` today), not in screen-specific app code. As compatibility systems mature, split them into real ownership modules instead of growing one giant app file.
- `engine/tools/guard_architecture.py` is the executable guard for this rule and runs through the `dragon_architecture_guard` CMake target before `dragon_core` builds. If it fails, fix the ownership regression instead of weakening the guard.

Reference content:

- The initial `game/` folder is seeded from the official 2001 DOS M.U.G.E.N package because it includes Kung Fu Man and the KFM stage.
- The original archive copy remains in `mugen-official-archive/`.
- Public releases of our engine should replace official M.U.G.E.N assets with original content.

## Milestone 0: Content Baseline

Goal: prove the engine can inspect the original content without guessing.

Already started:

- Import `mugen-2001.04.14-dos` KFM and stage into the M.U.G.E.N-style `game/` runtime root.
- Parse M.U.G.E.N-style section/key text files.
- Inspect KFM DEF, AIR, CMD, CNS, stage DEF, and SFF headers.

Exit criteria:

- `engine/tools/inspect_mugen_content.py game` passes.
- C++ command-line loader reads KFM DEF and stage DEF.
- Counts match the reference data:
  - KFM AIR actions: 105
  - KFM CNS statedefs: 45
  - KFM CMD commands: 31
  - KFM character SFF images: 253
  - KFM stage BG elements: 7
  - KFM stage SFF images: 7

## Milestone 1: SDL3 Window

Goal: open the real engine window using the real platform layer.

Tasks:

- Add SDL3 to CMake using one stable dependency path.
- Create a 960x540 window.
- Run a fixed 60 Hz loop.
- Handle keyboard/controller input through SDL3.
- Show the first mode-select screen.
- Keep mode-select state separate so Training can be implemented next.
- Mode-select layout rule: the menu is a side-anchored rail. It can render as `| option` on the left side or `option |` on the right side. Basic Up/Down navigation must only move the selector. Rail-side switching is reserved for a later higher-level game-mode switch inspired by Flying Dragon, and must not be tied to ordinary menu selection.

Exit criteria:

- `dragon_mugen` opens an SDL3 window.
- Escape exits.
- Window title says `Dragon MUGEN Core`.
- Frame loop updates at 60 Hz.
- No Win32-only display code exists.
- Mode select is user-friendly enough to test startup flow.
- Training is present as the first highlighted option.
- Moving Up/Down changes the selected menu item without changing rail side.
- Future game-mode switching may change rail side, but that behavior is out of scope until the mode system is defined.

## Milestone 2: bgfx Renderer Bootstrap

Goal: replace placeholder drawing with the actual renderer backend.

Tasks:

- Initialize bgfx from the SDL3 native window handle.
- Create a clear pass.
- Draw simple colored quads.
- Add debug text or simple bitmap text path.
- Keep renderer calls behind an engine-owned interface.

Exit criteria:

- SDL3 creates the window.
- bgfx clears the frame and renders a test quad.
- Runtime code calls engine renderer APIs, not raw bgfx everywhere.

## Milestone 1.5: Character Select Shell

Goal: route Training, Single Player, and VS/local through M.U.G.E.N-style character and stage selection before entering the future fight sandbox.

Tasks:

- Read character entries from `game/data/select.def`.
- Resolve each listed character to its `.def` file under `game/chars/`.
- Keep the C++ roster/stage/character-file resolution in the data layer. `App.cpp` may consume `CharacterSlot`, `StageSlot`, and `CharacterFiles`, but it must not rebuild M.U.G.E.N path rules ad hoc.
- Fall back to scanning `game/chars/` only when `select.def` is missing or empty.
- Display selectable character entries.
- Use `game/data/system.sff` motif sprites for the title and character-select presentation instead of debug panels.
- Start with Kung Fu Man selected.
- Read stage entries from `game/data/select.def` character stage values and `[ExtraStages]`.
- Resolve each listed stage to its `.def` file under `game/stages/`.
- Fall back to scanning `game/stages/` only when `select.def` has no valid stage entries.
- Confirming a character routes to stage select.
- Confirming a stage routes to the VS screen first; the fight session is prepared from the VS screen path before fight view.
- Escape returns from character select to mode select.
- Escape returns from stage select to character select.

Exit criteria:

- Selecting Training from mode select opens character select.
- Selecting Single Player from mode select opens character select.
- Selecting VS Mode from mode select opens character select.
- Character select shows `Kung Fu Man`.
- Additional local compatibility characters appear only when listed in `game/data/select.def`.
- Stage select shows `Mountainside Temple`.
- Arrow keys move the character-select grid cursor only when the destination cell contains a character.
- Up/Down moves stage selection without changing the mode rail behavior.
- Enter on character select confirms the roster entry and advances to stage select without loading the full character runtime.
- Character select loads only roster select portraits from the character `9000,0` icon and `9000,1` face sprites.
- Character select chooses P1 only for Training and the current VS/local flow; the opponent slot must not mirror the active cursor selection.
- Confirming character select writes explicit session slots: P1 character plus mode-owned opponent type (`Dummy`, `CPU`, or `LocalP2`).
- Enter on stage select advances to the VS screen first; the VS path then loads the selected character DEF `[Files]` entries for CMD, CNS/ST, `stcommon`, AIR, SFF, SND, palettes, and the selected stage background before fight view.
- Training reaches the fight view with the selected character and selected stage.
- Escape returns to mode select.

## Milestone 1.75: VS Screen and Fight View Shell

Goal: confirm the old-school flow before real sprite decoding: mode select, character select, stage select, VS screen, then fight view.

Tasks:

- Add a VS screen after stage select.
- Show selected P1, the mode-owned opponent slot, and selected stage.
- Stage confirmation must show the VS screen first, using select-safe portraits and stage metadata.
- After the VS screen has been presented, prepare the selected character runtime, selected stage background, camera, round state, and fighter start positions.
- Auto-advance from VS screen to fight view after a short pause.
- Enter on VS screen starts immediately.
- Entering fight view from VS must use the prepared session and must not reload/reset again.
- Fight view displays selected character/stage routing before full sprite decode.
- Document memory management before SFF/background loading.

Exit criteria:

- Training reaches VS screen after stage selection.
- VS screen reaches fight view.
- Fight view shows selected character and selected stage names.
- `docs/MEMORY_MODEL.md` exists and defines how sprite/stage/background assets will be loaded and cached.
- Match modes must have distinct menu/rules paths from Training. Do not implement them by merely hiding Training options.

## Milestone 3: SFF v1 + ACT Decode

Goal: decode 2001 M.U.G.E.N sprite assets.

Tasks:

- Parse SFF v1 headers and subfile records.
- Decode PCX sprite payloads.
- Load ACT palettes.
- Apply palette to indexed sprites.
- Convert decoded sprites to RGBA texture data.
- Add tests/inspection output for KFM and stage sprite counts.

Exit criteria:

- KFM SFF reports 253 decoded sprites.
- Stage SFF reports 7 decoded sprites.
- At least one KFM sprite and one stage sprite can be uploaded as textures.
- Shared-palette character sprites use selected ACT palette handling, not embedded PCX palettes.
- KFM portrait sprites `9000,0` and `9000,1` are usable by select/VS screens.

## Milestone 4: AIR Animation Playback

Goal: draw KFM idle animation.

Tasks:

- Parse `[Begin Action n]` blocks.
- Parse frame sprite group/index, offsets, duration, flip, blend.
- Parse collision boxes enough for debug display.
- Play action 0/idle on a loop.
- Load basic movement clips for training startup: `0`, `20`, `21`, `41`, `42`, `43`.

Exit criteria:

- KFM appears in the SDL/bgfx window.
- Idle animation advances at correct frame durations.
- Sprite origin/axis behavior is visibly reasonable.
- AIR action `0` drives the current KFM idle display.
- AIR frame offsets and horizontal/vertical flip flags affect rendering.

## Milestone 5: Stage Rendering

Goal: draw Mountainside Temple behind KFM.

Tasks:

- Parse stage DEF.
- Resolve stage SFF sprites.
- Draw static, animated, tiled, and parallax BG elements at a basic level.
- Use 320x240 logical coordinates first.
- Use `[StageInfo] zoffset` and `[PlayerInfo]` starts for fight placement.
- Respect SFF axes and `[BG ...] layerno` so background and foreground layers draw in the correct order.
- Parse `[Camera]` start, bounds, tension, vertical follow, and floor tension.
- Parse `[Bound] screenleft` and `screenright`, and clamp normal fighter movement to the camera-visible play area using character body/edge widths unless a supported character state controller opts out.
- Apply `[BG ...] delta` to stage layers as the camera moves.

Exit criteria:

- Mountainside Temple appears behind KFM.
- All 7 stage BG elements are accounted for.
- The view resembles the original stage well enough for engine work.
- Stage rendering reads `[BG ...]` elements from the selected stage DEF.
- Stage backgrounds use `spriteno`, `start`, `delta`, `tile`, `mask`, and `layerno`.
- Camera scrolls horizontally from player movement and vertically from jumps within stage camera bounds.
- Parallax-specific parameters and animated BG handling may remain incomplete until the next stage pass.

## Milestone 6: Training Sandbox Simulation

Goal: make the first interactive runtime.

Tasks:

- Fixed-step simulation.
- Fighter position, velocity, facing, ground, gravity.
- P1 keyboard input.
- P2 idle dummy.
- Single Fight local P2 input path, separate from Training dummy behavior.
- Basic state machine.
- Reset fight state from selected stage starts during stage confirmation before the VS screen appears.
- P1 arrows support walk left/right, jump, and Down crouch for the first training pass.
- `R` resets the current fight session.
- `F1` toggles hitboxes quickly.
- `F2` opens training options.
- Keyboard `A/S/D` maps to M.U.G.E.N punch commands `x/y/z`; keyboard `Z/X/C` maps to kick commands `a/b/c`.
- Keyboard Space maps to M.U.G.E.N command `s` for P1, Semicolon maps to `s` for local P2, and SDL gamepad Start maps to `s` for both players through the same input-buffer path.
- SDL3 gamepad input maps Xbox `X/Y/LB` or PlayStation `Square/Triangle/L1` to `x/y/z`, and Xbox `A/B/RB` or PlayStation `Cross/Circle/R1` to `a/b/c`.
- Main menu Options exposes canvas size, controller label style, and P1/P2 gamepad assignment. These settings are runtime-only until local settings move to `game/save/settings.def`.
- In Single Fight, P2 keyboard `U/O/P` maps to M.U.G.E.N commands `x/y/z`, `N/M/,` maps to `a/b/c`, and P2 movement uses `I/J/K/L`.
- KFM `cmd` `[Statedef -1]` routes standing normal commands to states `200`, `210`, `230`, and `240`, and Down-held crouch normal commands to states `400`, `410`, `430`, and `440`.
- Loaded KFM CNS attack states supply `anim`, `ctrl`, `velset`, `statetype`, `movetype`, `physics`, and `HitDef` data.
- Simple CNS `ChangeState` controllers with literal `value` support animation-end transitions, timed transitions, direct velocity/position landing checks, and optional `ctrl`.
- CNS `PlaySnd` controllers play parsed `value = group,index` through the SFX voice mixer using the runtime trigger subset. The current subset supports `F` common-sound prefixes, `channel`, `lowpriority`, `volume`, `loop`, and `persistent`.
- CNS `PlaySnd` and `StopSnd` controllers execute through one ordered audio-controller pass. The pass follows M.U.G.E.N special-state order (`State -3`, `State -2`, then current state) and preserves `PlaySnd`/`StopSnd` order within each parsed state.
- CNS audio controllers with trigger lines must not execute from partial fallback triggers. If the full trigger expression is outside the current parser subset, skip the sound controller until the missing trigger system is implemented.
- Common hit/recovery mapping now routes through available guard idle/end, crouch get-hit, air get-hit/recovery, bounce/ground-impact, and wake-up actions. Every branch must continue to check for the selected character's exact AIR action before using it because not all characters provide the full common frameset. Keep future common action mapping tracked in `docs/ANIMATION_MAPPING_AUDIT.md`.
- Fall bounce routing must preserve the M.U.G.E.N order: `5050 -> 5100 -> 5160 -> 5170 -> 5110 -> 5120 -> 0` for normal fall bounce, while trip falls stay on the `5070/5071 -> 5110` path. Do not gate bounce animation on `fall.recover`; recovery controls whether the player can air-recover, not whether the body bounces.
- Dizzy actions `5300`/`5301` are common hittable states. They must clear stale one-tick hit filters while active and must not leave the fighter permanently invulnerable in training.
- CNS `CtrlSet` controllers apply parsed `value = 0/1` using the runtime trigger subset, including `Time` and `AnimElem` patterns.
- CNS `PosAdd` controllers use the runtime trigger subset, including multiple `triggerN` entries on one controller, and apply parsed `x`/`y` in the fighter's facing direction.
- Simple CNS `ChangeAnim` controllers support target `value`, optional `elem`, `movecontact`, and simple `AnimElemTime(...)` comparisons for KFM standing strong punch.
- CNS `VelSet`, `VelAdd`, `VelMul`, `PosSet`, `PosFreeze`, `HitVelSet`, `HitBy`, `NotHitBy`, `Explod`, `AssertSpecial`, `SprPriority`, and `StateTypeSet` controllers support plain numeric movement/state values, selected character `Const(...)` values, runtime triggers, default per-tick persistence, and `persistent = 0`.
- Supported runtime controller families must execute in M.U.G.E.N special-state order: `State -3`, `State -2`, then the current state, skipping duplicate current-state dispatch when the fighter is already in `-3` or `-2`.
- `HitBy` and `NotHitBy` must remain character-authored CNS behavior. They apply timed filters against parsed `HitDef attr` data and must not become hardcoded per-character invulnerability rules.
- `HitOverride` must be treated as a defender-owned incoming-hit override. Per M.U.G.E.N 1.1 behavior, it should not create a normal attacker target link. `TargetState` and `TargetBind` operate on the attacker target recorded by a normal/guarded `HitDef`, keyed by the authored `HitDef id` when an `id` filter is present.
- Current target-controller support is intentionally narrow: `HitOverride attr/stateno/time`, `TargetState value/id`, `TargetBind pos/time/id`, `TargetDrop excludeID`, `TargetFacing value/id`, `TargetLifeAdd value/id/kill/absolute`, `TargetPowerAdd value/id`, `TargetVelAdd x/y/id`, and `TargetVelSet x/y/id`. Target controller values may use the current expression subset; target selection is still a single stored target link. Multiple targets and full throw/cinematic behavior remain separate roadmap items.
- `Turn` is a normal runtime controller and must only flip facing; it must not implicitly move the fighter or change target ownership. `Null` is an explicit no-op compatibility controller.
- Parsed `AssertSpecial` flags currently affect round-end suppression, timer freeze, HUD/background/foreground hiding, fighter invisibility, and default walk locking. Parsed but not-yet-behavioral flags remain documented until their matching systems exist.
- Old-style `Projectile` controllers support practical lifecycle, movement, and presentation fields: `projremove`, `projremovetime`, `projcancelanim`, `projpriority`/`priority`, `projmisstime`, `pausemovetime`, `supermovetime`, `projedgebound`, `projscreenbound`, `projstagebound`, `projheightbound`, `postype`, `offset`, `velocity`, `remvelocity`, `accel`, `velmul`, `projscale`, and `projshadow`. Runtime projectiles resolve variable-backed spawn/movement/scale expressions at creation time, respect owner pause/superpause move time, remove at parsed edge/stage/height bounds, apply removal velocity/animations, let finite hit/remove/cancel animations finish by AIR length, scale projectile collision boxes to match `projscale`, and perform documented simple projectile cancellation: equal priority cancels both, higher priority cancels lower and decrements. Exact shadow shape/color parity and deeper looped-remove edge cases remain separate compatibility work.
- M.U.G.E.N variable banks `var(n)`, `sysvar(n)`, `fvar(n)`, and `sysfvar(n)` are stored per fighter. `VarSet`, `VarAdd`, and `VarRandom` are supported for direct variable assignments and `v`/`fv` syntax. CNS triggers and CMD `State -1` gates can evaluate the current expression subset for variables, arithmetic, comparisons, `&&`/`||`, `!`, `floor`, `ceil`, `abs`, `ifelse`, `Const(...)`, `random`, `AILevel`, `time`, `animtime`, `stateno`, `roundstate`, `life`, `p2life`, `statetype`, `pos`, `vel`, and `p2bodydist x`.
- `HitDef` and old-style `Projectile` contact must resolve expression-backed `damage`, `guard.damage`, `pausetime`, spark numbers/offsets, hit/guard sound pairs, `envshake.*`, `fall.envshake.*`, `palfx.*`, ground/air hit times, ground/air velocities, guard velocity, and fall fields at the actual hit moment. The attacker is the expression owner and the defender is exposed as `p2`/`enemynear`, so variable-backed fields such as `fall.recover = (var(10) <= 0)` stay compatible with authored CNS behavior. `S` sound prefixes use selected-character sound lookup; `F` sound prefixes force common/fight sound lookup.
- `HitDef` and old-style `Projectile` contact must enforce `hitflag` before collision application. Expand M.U.G.E.N composite flags such as `M = H + L`, honor air/fall/downed and `+`/`-` get-hit restrictions, and skip illegal defender states instead of applying damage and correcting afterward.
- `GetHitVar(...)` stores a defender hit snapshot when `HitDef`/projectile contact enters a hit or guard state. The current supported names are `animtype`, `groundtype`, `airtype`, `slidetime`, `hittime`, `ctrltime`, `xvel`, `yvel`, `yaccel`, `fall`, `fall.recover`, `fall.recovertime`, `fall.damage`, `fall.xvel`, `fall.yvel`, and `hitcount`; `target, GetHitVar(...)` works through the current single stored target link.
- Character `[Size]`, `[Velocity] run.fwd/run.back`, and `[Movement] yaccel` are parsed for body widths and movement constants.
- Simple CNS `Width` and `PlayerPush` controllers support one-tick edge/player width overrides and push opt-out.
- Minimal CNS controller subset:
  - `ChangeState`
  - `SelfState`
  - `ChangeAnim`
  - `ChangeAnim2`
  - `VelSet`
  - `VelAdd`
  - `VelMul`
  - `PosSet`
  - `PosFreeze`
  - `HitVelSet`
  - `CtrlSet`
  - `StateTypeSet`
  - `Width`
  - `PlayerPush`
  - `SprPriority`
  - `Explod`
  - `AssertSpecial`
  - `VarSet`
  - `VarAdd`
  - `VarRandom`
  - `PlaySnd`
  - `StopSnd`
- `HitDef` parsed first, then expanded into common get-hit behavior
- Minimal trigger subset:
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

Exit criteria:

- P1 KFM can stand, walk, jump, crouch, and execute at least one standing or crouching normal attack.
- The first standing and crouching normal attack passes can damage the dummy and route it through a basic common get-hit reaction.
- P2 can idle as a dummy.
- Single Fight can run local P1 vs P2 control without Training dummy options.
- Reset returns both fighters to initial state.
- Single Fight must read M.U.G.E.N-compatible round settings from `game/data/fight.def`, including `match.wins`, round-start display timing, KO/Double-KO/Time-Over display timing, `over.waittime`, double-KO hit grace, `over.wintime`, win-message timing, and round-over timing.
- Single Fight/Fight HUD must read M.U.G.E.N-compatible powerbar placement from `game/data/fight.def` `[Powerbar]` before falling back to defaults. First-pass rectangle gauges are acceptable until full `fight.sff` sprite-based lifebars/powerbars land, but the data source must be `fight.def`.
- Each round must enter CNS state `5900` first when the loaded character/common CNS defines it, run its authored initialization controllers during the round-start phase, and only fall back to state `0` when `5900` is unavailable or the authored state routes there.
- A match must progress through `RoundStart -> Fight -> RoundFinish -> RoundResult -> MatchResult`; ties and double KOs do not award a round win and advance to another round.
- The final `MatchResult` screen is Dragon/Flying-Dragon-inspired UI, not M.U.G.E.N behavior. It must stay documented in `DRAGON_EXTENSIONS.md` and remain a post-match layer above M.U.G.E.N-compatible round scoring.
- Fighters must not walk through each other on the ground unless the active state disables push with a supported `PlayerPush` controller. Push uses parsed `[Size]`/`Width` values, not hardcoded prototype spacing.
- Fighters must not leave the visible play area under normal movement, push, hit reaction, or round-finish movement. The current runtime applies stage `[Bound] screenleft`/`screenright`, parsed character edge widths, and basic CNS `ScreenBound value = 0` as the character-code escape hatch.
- Ground movement and jump are still engine-owned until common CNS movement states are parsed, but attacks must enter through CMD/CNS state routing. Engine-owned jumps must preserve takeoff direction and use the M.U.G.E.N common-state `Anim + 3` peak animation when the selected AIR defines it.
- CMD `[Command]` sections feed a real input buffer for buttons, directions, simple motions, simultaneous `+` steps, hold `/`, release `~`, and broad `$` directions; State -1 routing must use buffered command names instead of hardcoded attack buttons. `[Defaults] command.time` and `command.buffer.time` must be parsed from the selected character command file. Button release atoms such as `~x`/`~a` are detected as real release transitions. Direction release atoms keep M.U.G.E.N's common motion-prefix behavior practical for authored commands such as `~D, DF, F, x`. Dragon may add player-input leniency that does not alter the source CMD files; currently terminal attack-button steps allow cardinal final directions to match their matching diagonal so QCF-style specials remain practical on keyboard and pad. Forward and backward dashes must enter through original `FF` and `BB` command definitions when the selected character provides them.
- State -1 `ChangeState value` may be a full M.U.G.E.N expression, not only a literal state number. Button-strength branches such as `ifelse(command = "DP_z", 1700, ifelse(command = "DP_y", 1600, 1500))` must evaluate from the same buffered command list used by the trigger gates.
- CMD/CNS expression gates must expose first-pass M.U.G.E.N contact and opponent predicates: `movecontact`, `movehit`, `moveguarded`, `numtarget`, `p2dist`, `p2bodydist`, `p2stateno`, `p2statetype`, `p2movetype`, `frontedgedist`, `backedgedist`, `enemynear, ...`, and `helper(id), ...`.
- KFM special moves are the active baseline after normals: Palm, Knee, Upper, and hyper startup paths should enter through the original `kfm.cmd`, execute from original `kfm.cns`, and keep compatibility work focused on missing CNS/runtime pieces rather than Dragon-only move hardcoding.
- Evil Ryu/Evil Ken special validation follows KFM. Their current blockers are deeper variable/redirection behavior, exact target/helper/projectile behavior, projectile priority/cancellation, AI gates, and M.U.G.E.N 1.0-specific helper/projectile trigger semantics.

## Milestone 7: Hitboxes and Debug Overlay

Goal: make the sandbox useful for engine development.

Tasks:

- Keep the existing camera/origin debug readout available through training options.
- Draw Clsn1 and Clsn2 boxes.
- Render Clsn1 attack boxes in red and Clsn2 hurtboxes in blue for the current AIR frame.
- Implement simple hit detection.
- Apply basic damage and knockback from parsed `HitDef` entries for loaded attack states.
- Track separate `HitDef` controllers in the same state so multi-hit normals can apply each parsed hit once.
- Route ground hits into common get-hit actions from parsed `animtype` and `ground.type`.
- Apply parsed `pausetime`, `ground.hittime`, and `ground.slidetime` to the active fight session.
- Parse first-pass `HitDef` fall fields: `fall`, `air.fall`, `fall.animtype`, `fall.recover`, `fall.recovertime`, `fall.damage`, `fall.yvelocity`, and `yaccel`, then route fall hits through common fall/land actions when those actions exist.
- Preserve and evaluate expression-backed `HitDef`/`Projectile` damage, pause, spark, sound, envshake, palfx, hit-time, velocity, guard velocity, and fall fields at contact time instead of collapsing them during parse.
- Expose parsed hit fields through `GetHitVar(...)` so `common1.cns` and selected character common states can drive recovery, slide, fall velocity, and hit animation decisions.
- Spawn hit sparks from parsed `sparkno`/`sparkxy` using `game/data/fightfx.air` and `game/data/fightfx.sff`.
- Play parsed `hitsound` using character `.snd` first and shared `game/data` sounds as fallback.
- Play parsed timed/global `PlaySnd` state sounds using the same character/shared sound archive lookup and stop channel-owned/looped sounds through parsed `StopSnd`.
- Parse basic guard fields from `HitDef`: `guardflag`, guard damage, `guard.sparkno`, `guardsound`, and `guard.velocity`.
- Toggle hitboxes, origins, floor line, debug readout, hit flash, hit sparks, hit log, hit sound, dummy invincibility, dummy life auto-restore, dummy freeze, dummy guard, guard damage, command HUD, input HUD, training power mode, and move-list category from training options.
- Add a command-training-style move-list page sourced from the selected character's CMD `State -1` labels, filtered to currently loadable states, and filterable by all, normal, special, and super move categories.
- Keep the move list compatible with positive command OR groups from CMD `State -1`, so alternate inputs for one move stay visible as one command strip.
- Load KFM/common guard AIR actions and route dummy guard through standing common states `130`, `150`, and `151`, and crouch common states `131`, `152`, and `153`.
- Reset training positions from training options without leaving the fight.

Exit criteria:

- Camera, fighter origin, zoffset, world position, velocity, facing, AIR action, and AIR frame can be inspected.
- Hitboxes/hurtboxes can be displayed.
- Training options can turn debug visuals, hit feedback, hit sound, and basic dummy behavior on/off without leaving the fight.
- Training options can open a character move list without leaving the fight.
- Single Fight has a distinct pause menu with resume, restart match, fighter select, stage select, and mode select.
- Single Fight has a minimal match rules layer: bidirectional hit detection, main-menu timer setting, parsed `fight.def` match wins/timing and `[Combo]` hit-counter presentation, tie-round replay, KO/Double-KO/timeout callouts, parsed in-world round-finish hold through `over.time`, winner/loser/draw poses through selected-character states where available, scored round pips/results, Dragon-only match result menu, deterministic rematch/menu input, and round reset.
- Main menu Options has match timer, canvas size, and Dragon UI scale settings. They must remain separate from Training options and future local/user settings should follow `docs/DRAGON_EXTENSIONS.md`.
- Hit sparks render from the common `game/data` fight FX files, not from the character folder.
- Hit counters render from `game/data/fight.def` `[Combo]` settings and count only unguarded hit chains. The active chain resets when the defender leaves hit/guard recovery; the display remains for parsed `displaytime`.
- Hit, guard, and timed state sounds play from the correct M.U.G.E.N sound archive placement.
- Rapidly triggered sound effects must mix as capped active voices and must not append full samples into a delayed playback backlog.
- Looped/channel-owned CNS sounds must be stoppable by the character's `StopSnd` controllers instead of relying on global mixer caps.
- Fall-causing hits must show fall/land animations from the loaded character/common AIR data instead of snapping straight from hit shake to stand recovery.
- KFM can hit the dummy.
- The dummy enters `5000` hit-shake and `5001` recovery before returning to idle.
- Dummy guard can cycle between off, standing guard, crouch guard, and auto guard; convert a guardable KFM hit into guard damage, guard spark, guard sound, and guard pushback through common guard-hit states; and disable guard damage independently for training without breaking KFM file compatibility.
- Training options include `P2 CONTROL`, which temporarily switches the active training opponent from `Dummy` to `LocalP2` and disables dummy-only behavior while enabled.
- Console/log reports hit events cleanly.
- `python engine/tools/audit_mugen_compat.py game` passes load-level checks for the active `select.def` roster and documents remaining parser/runtime warnings.

## Future Milestones

After the first training sandbox works:

- Proper title/menu flow.
- Arcade single-player.
- Broader dummy AI controls, high/low/air guard states, scripted dummy movement, and recorded input playback.
- Broader audio playback, including common state sounds, UI sounds, and music.
- Lifebars.
- Storyboard/cutscene parser for title, intro, ending, credits, and game-over images.
- Optional video playback support for story/title presentation once the image storyboard path is stable.
- Shop/equipment/leveling system.
- Tournament/campaign shell.
- External editor.

## Current Rule

The current code task is still Milestone 7: keep tightening selected-character hit interaction by expanding CNS/common-state behavior, hit sparks, sounds, and reset rules while preserving the KFM baseline. Do not regress runtime loading back to hardcoded KFM paths; KFM, Evil Ryu, Evil Ken, and future test characters must flow through `game/data/select.def` and the selected character DEF `[Files]` section. Do not collapse the project into one giant app file: each completed compatibility area should move toward a real data/runtime/rendering module with explicit ownership, and the architecture guard must stay passing. Do not start RPG/shop/equipment/tournament systems until KFM movement, one normal attack, dummy hitboxes, reset, and one working hit interaction are stable. The bgfx renderer remains required engine direction and should be addressed before broadening content or editor work.
