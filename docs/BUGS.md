# Bugs

Record runtime failures here before fixing them. This file is for observed behavior, not speculative compatibility gaps.

## Bug Report Template

```text
## Bug: short title

Status:
Severity:
Area:
Character:
Mode:
Stage:
Input device:
Build/commit:

Reproduction:
1.
2.
3.

Expected:
Actual:
Notes:
Possible suspect files:
Blocking:
```

## Current Pass

Computer Use could verify event-style keys such as `F1` and `F2`, but it could not reliably exercise SDL polled gameplay controls such as held movement or `A/S/D/Z/X/C` attacks. That is recorded as a verification limitation in `docs/LIVE_VERIFICATION_MATRIX.md`, not as a Dragon MUGEN runtime bug.

Manual user testing after commit `4581704` confirmed the Pass 12 frontend keyboard/controller paths work, and surfaced two real air-state bugs below.

## Bug: controller D-pad does not navigate match-complete win screen

Status: fixed by code-path inspection; physical controller retest pending
Severity: medium
Area: controller input / match result menu
Character: Evil Ken
Mode: Single Player
Stage: Mountainside Temple
Input device: physical controller
Build/commit: observed after `2c71808`

Reproduction:
1. Launch `build\dragon_mugen.exe`.
2. Enter Single Player.
3. Play through to the `MATCH COMPLETE` screen.
4. Try to move the result-menu selection with controller D-pad Up/Down.

Expected:
Controller D-pad Up/Down should move the result-menu highlight the same way keyboard Up/Down does. Controller confirm/cancel buttons should keep working.

Actual:
Keyboard Up/Down moved the win-screen result menu, and controller buttons worked, but controller D-pad did not move the result-menu highlight.

Notes:
The match-result gamepad branch in `FrontendFlow.h` mapped confirm/cancel buttons before returning from the FightView path, so D-pad buttons never reached the shared D-pad mapping.

Fix evidence:
`gamepadMenuKeyForButton` now maps `SDL_GAMEPAD_BUTTON_DPAD_UP` and `SDL_GAMEPAD_BUTTON_DPAD_DOWN` to `SDLK_UP` and `SDLK_DOWN` while the match result screen is open. Build and scripted verifier regressions passed; physical controller retest is still needed to mark this live PASS.

Possible suspect files:
`engine/src/FrontendFlow.h`

Blocking:
Does not block CPU baseline or architecture recovery. Blocks claiming match-result controller D-pad navigation as live-verified until retested on hardware.

## Bug: Single Player CPU opponent is passive / training-dummy-like

Status: fixed by scripted verifier; manual retest pending
Severity: high
Area: Single Player / CPU control / fight runtime
Character: Evil Ken
Mode: Single Player
Stage: Mountainside Temple
Input device: physical keyboard/controller user smoke
Build/commit: c7ea079 era; observed after SelectionState boundary work

Reproduction:
1. Launch `build\dragon_mugen.exe`.
2. Enter Single Player.
3. Select Evil Ken or the current tested fighter.
4. Select Mountainside Temple.
5. Start the match and play through to match completion.

Expected:
CPU opponent should move, attack, guard, or otherwise produce gameplay input. CPU behavior should route through the normal input/CMD/CNS path:
`CPU decision -> FighterInputState -> command buffer -> CMD/CNS -> runtime`.

Actual:
Single Player match flow and match-complete presentation worked, but before the fix the CPU behaved like a passive training dummy and did not meaningfully fight back. A first CPU input fix made P2 move and enter attack states, but user retest then showed CPU attacks did not damage P1 because P2-to-P1 hit resolution was still gated to local P2 controls.

Evidence:
User-supplied screenshot on 2026-06-05 shows `MATCH COMPLETE`, `SINGLE PLAYER`, Evil Ken winning `2 - 0` by decision on Mountainside Temple. User report says the CPU opponent was basically passive/training-dummy-like during the match.

Notes:
This is not a SelectionState issue. The fix does not force CPU `ChangeState` directly; it produces `FighterInputState` for CPU-controlled P2 and routes through the normal command/CMD/CNS path.
Single Player hitbox-debug display is not claimed by this bug fix; the focused fix is CPU attack hit application through the existing HitDef path.

Fix evidence:
`--verify cpu-baseline` passes with `pass=7 partial=0 fail=0 blocked=0`. The verifier proves Single Player reaches fight phase, P2 moves toward P1, P2 enters attack state/action `200`, CPU P2 can damage P1 (`p1_life_before=977`, `p1_life_after=885`), P1 can still hit/damage P2, timer stability remains intact, and the scenario exits cleanly.

Possible suspect files:
`engine/src/App.cpp`, CPU control/update path, FighterInputState production for CPU-controlled fighters

Blocking:
Does not block the SelectionState boundary. No longer blocks scripted baseline CPU fighting behavior claims. More advanced CPU behavior and manual retest remain future work.

## Bug: diagonal jump can continue indefinitely while held

Status: fixed by scripted verifier; physical retest pending
Severity: high
Area: jump / air movement / input hold
Character: Kung Fu Man, likely broader
Mode: Training
Stage: Mountainside Temple
Input device: physical keyboard and controller
Build/commit: 4581704; fixed in current air-state regression worktree

Reproduction:
1. Launch `build\dragon_mugen.exe`.
2. Enter Training with Kung Fu Man on Mountainside Temple.
3. Hold Up + Forward or Up + Back.
4. Keep holding the direction pair while airborne.

Expected:
The character should perform one jump arc by default, then land. Holding Up + Forward/Back should not keep extending the jump or allow repeated air jumps. Double jump should be a separate future setting with an explicit default and limit.

Actual:
The diagonal jump has no end while Up + Forward/Back remains held. The character can keep moving in that direction indefinitely and can effectively perform infinite jumps in the air until the direction keys are released.

Notes:
User-supplied screenshot evidence on 2026-06-05 shows the Training fight after Pass 12 manual smoke, and user report confirms the issue with keyboard and controller. The immediate fix restores a single-jump limit by default. A Main Settings double-jump option remains a future feature and was not added in this fix.

Fix evidence:
`build\dragon_mugen.exe --verify kfm-air-state` now passes `diagonal_jump_forward_lands`, `diagonal_jump_back_lands`, `diagonal_jump_forward_from_walk_lands`, and `diagonal_jump_back_from_walk_lands` with `saw_air=1`, `landed=1`, `reentered_air_after_landing=0`, `final_y=0.000000`, `final_vy=0.000000`, `final_state=0`, `final_state_type=S`, and `final_on_ground=1`.

Possible suspect files:
`engine/src/App.cpp`, jump/air-state input and movement handling

Blocking:
No for scripted runtime coverage. Physical keyboard/controller retest is still useful before the next broad architecture pass.

## Bug: air attack can leave character stuck at airborne height

Status: fixed by scripted verifier; physical retest pending
Severity: high
Area: air attack / landing / ground position
Character: Kung Fu Man, likely broader
Mode: Training
Stage: Mountainside Temple
Input device: physical keyboard and controller
Build/commit: 4581704; fixed in current air-state regression worktree

Reproduction:
1. Launch `build\dragon_mugen.exe`.
2. Enter Training with Kung Fu Man on Mountainside Temple.
3. Jump.
4. Perform an air attack.

Expected:
The character should complete the air attack, continue the normal fall/landing path, and return to the real stage floor.

Actual:
The character can get stuck in the air after the attack, as if the ground level became the height where the air attack occurred.

Notes:
User report on 2026-06-05 describes the fighter becoming a suspended state after an air attack. Screenshot evidence includes KFM airborne/combat states with hitbox/debug overlays active after Pass 12 manual smoke. This is treated as an air-state/landing bug, not as a frontend extraction regression.

Fix evidence:
`build\dragon_mugen.exe --verify kfm-air-state` now passes `air_attack_lands` with `saw_air=1`, `saw_air_attack=1`, `landed_after_attack=1`, `final_y=0.000000`, `final_vy=0.000000`, `final_state=0`, `final_state_type=S`, and `final_on_ground=1`.

Possible suspect files:
`engine/src/App.cpp`, air attack state handling, landing detection, position reset/ground clamp logic

Blocking:
No for scripted runtime coverage. Physical keyboard/controller retest is still useful before the next broad architecture pass.

## Bug: state entry skips `Time = 0` transitions

Status: fixed in current verification pass
Severity: high
Area: CNS state execution
Character: Kung Fu Man
Mode: Training
Stage: Mountainside Temple
Input device: scripted internal verification input
Build/commit: cf93a57 plus Playable Core Proof worktree changes

Reproduction:
1. Run `build\dragon_mugen.exe --verify kfm-baseline`.
2. Let the verifier wait for controllable idle.
3. Observe KFM remains in common intro/preintro state `190`.

Expected:
KFM should route through common intro state `190`/authored intro state `191` and eventually reach controllable idle state `0`.

Actual:
KFM stayed in state `190` with `ctrl=0`, blocking movement and all downstream playable-core checks.

Observed values:
`state=190 anim=190 ctrl=0` after 360 verifier frames.

Notes:
The likely cause is that a state entered by `ChangeState` was having `stateTime` incremented at the end of the same frame, causing `Time = 0` controllers in the newly entered state to miss their only trigger window.

Possible suspect files:
`engine/src/App.cpp`

Blocking:
Yes. It blocked the KFM baseline movement gate.

## Bug: KFM held movement stayed at start position

Status: fixed in current verification pass
Severity: high
Area: movement / common state compatibility
Character: Kung Fu Man
Mode: Training
Stage: Mountainside Temple
Input device: scripted internal verification input
Build/commit: cf93a57 plus Playable Core Proof worktree changes

Reproduction:
1. Run `build\dragon_mugen.exe --verify kfm-baseline`.
2. Wait for KFM to reach controllable idle.
3. Hold symbolic `right` for 60 frames.

Expected:
KFM should move horizontally using M.U.G.E.N-compatible walk behavior.

Actual:
KFM stayed at `x=-70`, producing `delta=0`.

Observed values:
`x_before=-70.000000 x_after=-70.000000 delta=0.000000`

Notes:
The engine was applying a hardcoded state `0` walk velocity below the common stand state's `abs(vel x) < 2` stop threshold. KFM's common walk path expects state `20` and `const(velocity.walk.fwd.x/back.x)`.

Possible suspect files:
`engine/src/App.cpp`, `engine/include/dragon/MugenData.h`, `engine/src/MugenData.cpp`

Blocking:
Yes. It blocked the KFM baseline movement gate.

## Bug: KFM walk animation freezes while movement continues

Status: fixed in current worktree and physically retested
Severity: medium
Area: animation / common walk state
Character: Kung Fu Man
Mode: Training
Stage: Mountainside Temple
Input device: physical keyboard
Build/commit: cf93a57 plus current verification worktree changes

Reproduction:
1. Launch `build\dragon_mugen.exe`.
2. Enter Training with Kung Fu Man on Mountainside Temple.
3. Hold Left or Right on the physical keyboard.

Expected:
KFM should keep playing the walking animation while the movement key is held.

Actual:
Fixed. Before the fix, KFM started to walk and then froze/slid while movement continued.

Observed values:
Manual physical keyboard reports from 2026-06-04:
- Before the first targeted fix: "he starts to walk but then freezes on frame but can still move."
- After the first targeted fix: "before the last change he would walk then stop now he doesnt walk at all just moves acording the the direction pushed."
- After the second targeted fix: "ok its all good now."

Notes:
This proves the physical keyboard path reaches movement runtime. The first targeted fix kept state `20` active, which exposed that plain held-walk entry was not reliably selecting walk action `20`/`21`. The second targeted fix explicitly starts the correct walk action when held input enters common walk state `20`, and physical keyboard retest confirmed the walk behavior is now good.

Possible suspect files:
`engine/src/App.cpp`

Blocking:
No.
