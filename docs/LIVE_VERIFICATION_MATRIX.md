# Live Verification Matrix

This matrix records observed runtime behavior. Static audits and parser checks do not count as live verification.

Statuses:

- `PASS`
- `FAIL`
- `PARTIAL`
- `BLOCKED`
- `NOT TESTED`

Current pass metadata:

- Date/time started: `2026-06-04 10:54:00 -04:00`
- Commit: `33d86e3`
- Launch command: `build/dragon_mugen.exe game`
- Baseline character: `Kung Fu Man`
- Baseline stage: `Mountainside Temple`

| Area | Character | Mode | Input / Action | Expected Result | Actual Result | Status | Evidence / Notes |
|---|---|---|---|---|---|---|---|
| App shell | N/A | Main | Launch `build/dragon_mugen.exe game` | App opens from game root | Dragon MUGEN window opened | PASS | 2026-06-04, commit `33d86e3`, launched from repo root with PowerShell because Computer Use launcher cannot set working directory/args. |
| App shell | N/A | Main | Main/title screen | M.U.G.E.N-style title/main menu appears | Main screen showed M.U.G.E.N motif, `DRAGON MUGEN CORE`, and Training highlighted | PASS | Screenshot captured through Computer Use. |
| App shell | N/A | Main | Settings screen | Settings menu opens | Could not navigate to Options: Computer Use Enter/Escape worked, but Down did not move the SDL menu selector | BLOCKED | Do not treat this as a game failure; manual verification or better input harness required. |
| App shell | Kung Fu Man | Training | Character select | Character select opens and shows roster icons | KFM, Evil Ryu, and Evil Ken portraits/icons visible | PASS | Enter from highlighted Training opened character select. |
| App shell | Kung Fu Man | Training | Stage select | Stage select opens | Stage Select showed Mountainside Temple | PASS | Enter from character select. |
| App shell | Kung Fu Man | Training | VS screen | VS screen opens before fight | Training VS screen showed KFM, Dummy, and Mountainside Temple | PASS | Enter from stage select. |
| App shell | Kung Fu Man | Training | Fight view | Fight loads selected character and stage after VS | Fight view loaded KFM, Dummy, Mountainside Temple, power bars, command HUD | PASS | Auto-advance from VS. |
| App shell | Kung Fu Man | Training | Escape/back routing | Back out through fight/stage/character/main and exit | Fight -> Stage Select -> Character Select -> Main -> app closed | PASS | Escape sequence verified; `list_apps` showed no Dragon MUGEN window after final Escape. |
| Training tools | Kung Fu Man | Training | `F1` | Hitbox overlay toggles | Blue hitboxes appeared around KFM and Dummy | PASS | F1 key verified live. |
| Training tools | Kung Fu Man | Training | `F2` | Training options opens | Training Options panel opened | PASS | F2 key verified live. |
| Training tools | Kung Fu Man | Training | `R` reset | Fight resets to stage starts | No distinct visual change proved reset | PARTIAL | Key was pressed, but screenshot evidence was inconclusive. |
| KFM baseline | Kung Fu Man | Training | Idle | KFM idles in fight view | KFM idle pose/animation visible | PASS | Fight view remained active. |
| KFM baseline | Kung Fu Man | Training | Held left/right | Fighter walks under held input | Could not perform real held input with Computer Use | BLOCKED | The automation API exposes key press, not reliable held-key control for SDL gameplay input. |
| KFM baseline | Kung Fu Man | Training | Held down | Fighter crouches under held input | Could not perform real held input with Computer Use | BLOCKED | Manual verification required. |
| KFM baseline | Kung Fu Man | Training | Jump | Fighter jumps and returns to ground | Gameplay direction/button input not reliable through automation | BLOCKED | Manual verification required. |
| KFM baseline | Kung Fu Man | Training | Standing normals | Buttons enter authored normal states | `A` and `S` presses produced no visible attack in captured frames | BLOCKED | Do not mark FAIL without manual or harness verification. |
| KFM baseline | Kung Fu Man | Training | Crouching normals | Down plus buttons enter authored crouch normal states | Requires reliable held down plus button input | BLOCKED | Manual verification required. |
| KFM baseline | Kung Fu Man | Training | Hit/guard/KO flow | Damage, guard, KO, and reset behavior can be observed | Not attempted because attack/movement input could not be verified | NOT TESTED | Requires reliable input harness/manual pass. |
| Evil Ryu smoke | Evil Ryu | Training | Load/idle/move/normal/special | Character loads and basic actions do not glitch/crash | Not attempted; character selection navigation blocked by unreliable Down/Right automation input | BLOCKED | Static load audit still passes with warnings. |
| Evil Ken smoke | Evil Ken | Training | Load/idle/move/normal/special | Character loads and basic actions do not glitch/crash | Not attempted; character selection navigation blocked by unreliable Down/Right automation input | BLOCKED | Static load audit still passes with warnings. |
| P2 keyboard | Kung Fu Man | VS or Training P2 control | Movement and attack buttons | P2 responds where local P2 is expected | Not attempted because menu/input navigation was unreliable | BLOCKED | Manual verification or input harness required. |
| Controller input | N/A | Any | Gamepad assignment and input | Connected controller can drive assigned player | No controller verified in this pass yet | BLOCKED | Mark PASS only after a real controller is tested. |
