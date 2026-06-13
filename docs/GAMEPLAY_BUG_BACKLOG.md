# Gameplay Bug Backlog

Snapshot: 2026-06-12, `arena-mode-clean` at `59dcc76`.

Purpose: stabilize the current Training, Single Player, VS, and Arena gameplay loops before adding story mode or tournament mode. This file tracks confirmed bugs, high-risk partial systems, and manual verification gaps that directly affect moment-to-moment play.

## Priority Rules

- `P0` blocks credible local gameplay or makes a selected mode misleading.
- `P1` hurts normal play feel, round flow, controls, or match readability.
- `P2` is verification debt or polish that should not block early gameplay stabilization.
- `Confirmed` means an observed runtime failure exists.
- `Partial` means the feature works in a limited path but has a known missing runtime behavior.
- `Verify` means the code may be fine, but the branch needs a live/manual pass before claiming it.

## P0 - Fix Before New Modes

### VS selected P2 visuals do not have full P2 runtime

Status: `Fixed - verified`

Evidence: `docs/FEATURE_LEDGER.md` records that VS Mode has independent P2 character selection, portrait/name presentation, and separate P2 visual animation clips, but fight runtime still shares P1 CMD/CNS/hitdefs/sounds/constants.

Impact: VS can show Evil Ken or Evil Ryu as Player 2 while still executing Player 1's runtime behavior. That makes local VS misleading and blocks credible two-character gameplay.

Fix: VS now loads the selected opponent into a full runtime bundle and routes fighter 2 constants, states, commands, hitdefs, clips, sounds, and victory quotes through the selected P2 runtime. `vs-p2-runtime` verifies KFM P1 keeps KFM runtime counts while P2 loads Evil Ryu runtime counts and can enter a P2-only authored state.

Acceptance:

- VS loads P1 and P2 runtime files from their selected characters independently.
- P2 commands, state controllers, hitdefs, constants, sounds, and victory quotes come from P2's selected character.
- Existing Training, Single Player, and Arena per-fighter runtime behavior stay unchanged.
- Add or update a verifier that selects different P1/P2 characters and proves a P2-only authored behavior is running from P2's selected runtime.

### Classic Single Fight KO, double-KO, draw, and pause actions need dedicated checks

Status: `Partial`

Evidence: `docs/FEATURE_LEDGER.md` records live evidence for timer/HUD, pause/resume, local P2 controls, time-over, round result, match result, rematch, and VS KO/result presentation, but says KO/double-KO/draw handling and non-Resume pause actions still need dedicated work/checks.

Impact: story/tournament modes would depend on reliable fight conclusions and pause routing. If those paths are wrong, higher-level modes will inherit broken progression.

Acceptance:

- Single Player and VS correctly handle P1 KO, P2 KO, double-KO, and time-over draw.
- Round pips, match result, rematch, mode/menu return, and restart behavior are deterministic.
- Pause menu actions beyond Resume are verified in both Single Player and VS.
- Add verifiers or a documented live pass for the above cases.

## P1 - Core Gameplay Feel And Controls

### Training Show Me can miss contextual P2 dummy moves

Status: `Fixed - verified`

Evidence: directional Training command demos could fail to make the P2 dummy perform the selected authored move because release-prefixed motion atoms were emitted as neutral input and terminal button-only steps did not keep the final motion direction held. Evil Ken's first displayed `Ground Recovery Roll` entry also failed from Show Me because the authored CMD requires recoverable hit-fall state `5050`/`5071`; neutral reset made the dummy crouch/punch instead of roll.

Impact: Training could show a selected move but have the dummy perform no special, or make the command look flipped/wrong from the P2 side.

Fix: Show Me now treats release-prefixed motion atoms as held directions before release, carries the previous motion direction into terminal button-only steps, holds non-final motion steps long enough for the command buffer, and stages recovery-roll-style entries in a valid recoverable hit-fall context. Hit-state command routing now checks legal State -1 entries before normal hit-fall recovery handling. `evilken-training-demo-hit` verifies Evil Ken `Ground Recovery Roll` buffers P2 `BQCD_x` and enters roll state/action, verifies Evil Ken `S.Jab` hit demonstration, and verifies right-side P2 entering Evil Ken `Hadouken` state `1000`.

### Guard, fall recovery, guard effects, and KO coverage remain unverified

Status: `Partial`

Evidence: `docs/FEATURE_LEDGER.md` records KFM hit contact, damage, hitpause, and hit sound as live-proved, but guard, fall/recovery, guard spark/sound, KO, and broader character compatibility remain unverified.

Impact: basic fighting game readability depends on blocking, knockdown, recovery, and KO feedback. Missing coverage can hide bad hit reactions or stuck states.

Acceptance:

- KFM, Evil Ken, and Evil Ryu have scripted or live coverage for guard, guard sound/spark, knockdown, recovery, and KO.
- Failed guard or recovery cases are promoted to confirmed bugs in `docs/BUGS.md`.

### Broader CMD/runtime compatibility remains partial

Status: `Partial`

Evidence: `docs/FEATURE_LEDGER.md` records KFM normal routing and scripted special/super checks for KFM, Evil Ken, and Evil Ryu. Neutral ground special variants are now covered by button strength for the current roster, but air-only specials, throws, alpha counters, custom combo, the full super catalog, and broader character compatibility are still partial.

Impact: command buffering and State -1 routing are central to gameplay. Current proof is good for the included characters, but compatibility gaps may appear as soon as more characters are added.

Acceptance:

- Keep the current KFM, Evil Ken, and Evil Ryu special/super verifier set green.
- Add one compatibility-focused check before adding any new playable character.
- Document unsupported CMD/CNS behavior in `docs/COMPATIBILITY_AUDIT.md`.

### CPU behavior is a baseline, not real AI

Status: `Partial`

Evidence: `docs/FEATURE_LEDGER.md` records deterministic CPU input generation and successful `cpu-baseline`, while `docs/BUGS.md` still notes more advanced CPU behavior as future work.

Impact: Single Player and Arena are playable, but CPU behavior can still feel shallow or repetitive.

Acceptance:

- CPU keeps moving, attacking, and taking damage without passive dummy behavior.
- Add at least one behavior improvement pass after P0 fight correctness is stable.
- Do not hardcode character-specific AI into the engine.

### Manual Arena setup and GUI navigation need a current live pass

Status: `Verify`

Evidence: `docs/FEATURE_LEDGER.md` records Arena scripted runtime verifiers as passing, but manual GUI navigation was not rerun in the latest checkpoint.

Impact: Arena runtime may be solid while setup/menu routing still has hidden usability bugs.

Acceptance:

- Live pass confirms Arena Mode reaches Character Select and Arena Setup.
- Arena Setup starts 1, 2, and 3 CPU matches.
- CPU slots, stage, timer, Z Axis, and Camera Rotate can be changed without affecting Training, Single Player, or VS.
- Arena result rows route correctly: Play Again, Arena Setup, Fighter Select, and Mode Select.

## P2 - Verification And Polish Debt

### Training options need a broader live pass

Status: `Verify`

Evidence: `docs/FEATURE_LEDGER.md` records dummy presence, F1 hitboxes, F2 options, reset, command HUD, input HUD, and P2 keyboard smoke, but says remaining options need live verification.

Acceptance:

- Verify dummy guard/stand/crouch/jump behavior, power option, guard damage, command HUD, input HUD, move list, reset, and P2 control.
- Promote any failed option to a confirmed bug.

### Visual feedback smoke needs to be rerun on the current branch

Status: `Verify`

Evidence: the polish checkpoint recorded hit sparks and hit logs, but combo counter pulse, audible impact, render-only shake feel, match-result presentation, Single Player CPU visual smoke, and VS/P2 visual smoke were not rerun in that checkpoint.

Acceptance:

- Current branch live pass confirms hit spark, hit sound, combo pulse, camera shake feel, match-result presentation, Single Player CPU presentation, and VS P2 presentation.

### Physical input retests should stay on the checklist

Status: `Verify`

Evidence: older bugs for controller D-pad match-result navigation, diagonal held jumps, and air attacks stuck in air are marked fixed by code/scripted verification or later user retest, but hardware/controller paths should stay part of release smoke.

Acceptance:

- Physical keyboard confirms diagonal jumps and air attacks return to ground.
- Physical controller confirms match-result D-pad navigation and confirm/cancel.
- Keyboard and controller both feed the command buffer.

## Current Green Checks To Preserve

- Arena CPU count/default/resolution verifiers.
- Arena Z-axis keyboard/gamepad controls.
- Arena hit depth, push depth, raw draw order, rotated draw order, camera rotate toggle/projection.
- Arena CPU depth alignment and modifier sidestep.
- Arena per-fighter runtime and OpenBOR scroll stage.
- KFM guard recovery and specials/supers.
- Evil Ken specials/supers, air-special landing, power-charge helper, and trip grounding.
- Evil Ryu specials/supers, air-special landing, and dash.
