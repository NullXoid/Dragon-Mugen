# Runtime Verification Checklist

Use this checklist to separate live behavior from static/parser claims. Do not mark a row as live-passed unless it was observed in the running app with held inputs where movement is involved.

Statuses:

- `PASS`
- `FAIL`
- `PARTIAL`
- `BLOCKED`
- `NOT TESTED`

## 1. App Shell

| Check | Status | Notes |
| --- | --- | --- |
| Launch with `build/dragon_mugen.exe game` | PASS | Verified live on 2026-06-04 with commit `33d86e3`. |
| Launch `build/dragon_mugen.exe` with no game argument | PASS | Verified live on 2026-06-04 with commit `e0bc149`; runtime root auto-resolved and full title art loaded. |
| Main/title screen appears | PASS | Main screen showed M.U.G.E.N motif and Training highlighted. |
| Settings screen opens | BLOCKED | Automation could not move the SDL menu selector with Down; manual verification required. |
| Training path opens | PASS | Enter from main opened Training character select. |
| Single Player path opens | BLOCKED | Automation could not move the SDL menu selector with Down; manual verification required. |
| VS path opens | BLOCKED | Automation could not move the SDL menu selector with Down; manual verification required. |
| Character select opens | PASS | KFM, Evil Ryu, and Evil Ken portraits/icons visible. |
| Stage select opens | PASS | Enter from character select opened Stage Select. |
| VS screen opens | PASS | Enter from stage select opened Training VS screen. |
| Fight view opens | PASS | Auto-advance from VS loaded KFM, Dummy, and Mountainside Temple. |
| Escape/back routing works | PASS | Fight -> Stage Select -> Character Select -> Main observed. |
| Clean exit works | PASS | Escape from main closed the app; no Dragon MUGEN window remained. |

## 2. Character And Stage Selection

| Check | Status | Notes |
| --- | --- | --- |
| KFM selectable | PASS | KFM was the selected live character. |
| Evil Ryu selectable | PARTIAL | Evil Ryu portrait/icon visible; selection was not navigated due automation arrow-key limitation. |
| Evil Ken selectable | PARTIAL | Evil Ken portrait/icon visible; selection was not navigated due automation arrow-key limitation. |
| `DragonBench` folder not selectable unless listed in `select.def` | PARTIAL | Not visible on observed first select page; exhaustive navigation blocked by automation arrow-key limitation. |
| `A.Ben` folder not selectable unless listed in `select.def` | PARTIAL | Not visible on observed first select page; exhaustive navigation blocked by automation arrow-key limitation. |
| `I.Chie` folder not selectable unless listed in `select.def` | PARTIAL | Not visible on observed first select page; exhaustive navigation blocked by automation arrow-key limitation. |
| Mountainside Temple selectable | PASS | Stage Select showed Mountainside Temple selected. |
| Fight loads selected character | PASS | Fight view loaded Kung Fu Man. |
| Fight loads selected stage | PASS | Fight view loaded Mountainside Temple. |

## 3. Keyboard P1 Input

| Check | Status | Notes |
| --- | --- | --- |
| Held left movement | PASS | Manual physical keyboard retest confirms movement and walk animation are good after the second targeted fix. |
| Held right movement | PASS | Manual physical keyboard retest confirms movement and walk animation are good after the second targeted fix. |
| Held crouch | PASS | Manual physical keyboard report says the listed crouch check works. |
| Jump | PASS | Manual physical keyboard report says the listed jump/land check works. |
| Dash forward, if implemented | PASS | Manual physical keyboard report says run forward works. |
| Dash backward, if implemented | PASS | Manual physical keyboard report says dash backward works. |
| `x/y/z` buttons | PASS | Manual physical keyboard report says standing/crouching attacks work; exact per-button matrix still needs a later detailed pass. |
| `a/b/c` buttons | PASS | Manual physical keyboard report says standing/crouching attacks work; exact per-button matrix still needs a later detailed pass. |
| Hold directions | PASS | Manual physical keyboard retest confirms direction holds reach runtime and walk animation cycles correctly. |
| Release directions | PASS | Manual physical keyboard report says the listed movement/crouch checks work, including expected recovery behavior. |
| Simultaneous buttons | PASS | Manual physical keyboard report says Down + attack crouching-normal behavior works. |
| Post-input-extraction smoke | PASS | After input boundary extraction, user confirmed held forward/back walk, crouch, jump, a normal attack, and Esc exit still work. |

## 4. Keyboard P2 Input

| Check | Status | Notes |
| --- | --- | --- |
| Held left movement | PASS | User tested P2 keyboard in a real P2 mode after input boundary extraction and reported working. |
| Held right movement | PASS | User tested P2 keyboard in a real P2 mode after input boundary extraction and reported working. |
| Crouch | PASS | User tested P2 keyboard in a real P2 mode after input boundary extraction and reported working. |
| Jump | PASS | User tested P2 keyboard in a real P2 mode after input boundary extraction and reported working. |
| `x/y/z` buttons | PASS | User tested P2 keyboard in a real P2 mode after input boundary extraction and reported working; detailed per-button matrix can be expanded later. |
| `a/b/c` buttons | PASS | User tested P2 keyboard in a real P2 mode after input boundary extraction and reported working; detailed per-button matrix can be expanded later. |
| P2 can be controlled where expected | PASS | Manual physical keyboard evidence in a real P2 mode. |
| P1/P2 face each other | PARTIAL | User reported P2 keyboard mode works; a dedicated facing-only check was not separately recorded. |
| Collision/player push works | PARTIAL | User reported P2 keyboard mode works; a dedicated player-push/collision-only check was not separately recorded. |

## 5. Training Mode

| Check | Status | Notes |
| --- | --- | --- |
| Dummy appears | PASS | Dummy was visible in Training fight view. |
| `F1` hitbox toggle | PASS | Hitboxes appeared after `F1`. |
| `F2` training options | PASS | Training Options panel opened after `F2`. |
| `R` reset | PASS | Manual physical keyboard report says reset works without crash. |
| Dummy invincibility | NOT TESTED |  |
| Dummy auto-life | NOT TESTED |  |
| Dummy freeze | NOT TESTED |  |
| Dummy guard mode | NOT TESTED |  |
| Guard damage toggle | NOT TESTED |  |
| P2-controlled training option | NOT TESTED |  |
| Command HUD | PASS | Command panel visible in fight view. |
| Input HUD | PASS | Input HUD area visible in fight view. |
| Post-training-overlay extraction GUI smoke | PASS | After render-only overlay extraction, GUI smoke reached title -> character select -> stage select -> VS -> fight, command/input HUD rendered, F1 hitboxes toggled, F2 Training Options opened, `R` did not crash, and Escape backed out to main/exit. User-supplied screenshots on 2026-06-04 also confirm command/input HUD, F1 hitboxes, F2 Training Options, stage select backout, and Training overlay persistence after the pause-menu extraction. Computer Use could not perform a true held-key physical retest, so held input status remains based on scripted verification and prior user manual evidence. |
| Training power mode | NOT TESTED |  |
| Move category filter | NOT TESTED |  |
| Move list derived from CMD `State -1` | NOT TESTED |  |
| Flying Dragon-inspired command display | NOT TESTED |  |

## 6. KFM Baseline Combat

| Check | Status | Notes |
| --- | --- | --- |
| Idle | PASS | Live visual pass plus `--verify kfm-baseline`: `state=0 anim=0 ctrl=1`. |
| Walk forward/back | PASS | Internal scripted input path verified movement deltas. Manual physical keyboard retest confirms walk animation now cycles correctly after the second targeted fix. |
| Crouch | PASS | Internal scripted input path verified `state=11 anim=11`; manual physical keyboard report says crouch works. |
| Jump | PASS | Internal scripted input path verified `y_min=-73.079994 grounded_final=true`; manual physical keyboard report says jump/land works. |
| Standing normals | PASS | Internal scripted CMD/CNS path verified `command=y state_after=210 anim_after=210`; manual physical keyboard report says standing normals work. |
| Crouching normals | PASS | Internal scripted CMD/CNS path verified `command=y state_before=11 state_after=410 anim_after=410`; manual physical keyboard report says crouching normals work. |
| Hit detection | PASS | Internal scripted path verified `contact=1`; manual physical keyboard report says hit/damage works. |
| Damage | PASS | Internal scripted path verified P2 life `1000 -> 943`; manual physical keyboard report says hit/damage works. |
| Guard contact | NOT TESTED | Depends on attack and guard setup. |
| Guard damage | NOT TESTED | Depends on guard contact. |
| Hit pause | PASS | Internal scripted path observed hitpause. |
| Hit spark | PASS | Manual physical keyboard retest confirmed hitpause/spark/sound feedback; scripted final snapshot still does not expose active effect count. |
| Hit sound | PASS | Internal scripted path observed active sounds and hit text `snd 5,2`. |
| Guard spark | NOT TESTED | Depends on guard contact. |
| Guard sound | NOT TESTED | Depends on guard contact. |
| Fall/get-hit routing | NOT TESTED | Depends on successful hit setup. |
| Wake-up/recovery | NOT TESTED | Depends on successful fall/get-hit setup. |
| KO | NOT TESTED | Depends on damage flow. |
| Round reset/rematch or back out | PARTIAL | Back out from fight was verified; round reset/rematch was not. |

## 7. Match Rules

| Check | Status | Notes |
| --- | --- | --- |
| `RoundStart` | PARTIAL | Fight loaded through VS; specific RoundStart callout/timing not independently verified. |
| `Fight` | PASS | Fight view loaded and stayed active. |
| Match pause menu rendering after extraction | PASS | Render-only pause menu functions were extracted and post-extraction build/scripted verifiers passed. User-supplied screenshots on 2026-06-04 confirm the Single Player pause menu renders with all expected rows and selected-row highlight, and that Resume returns to fight. Restart/Fighter Select/Stage Select/Mode Select actions remain separate untested pause-menu actions. |
| `RoundFinish` | NOT TESTED |  |
| `RoundResult` | NOT TESTED |  |
| `MatchResult` | NOT TESTED |  |
| KO | NOT TESTED |  |
| Double KO | NOT TESTED |  |
| Time Over | NOT TESTED |  |
| Tie round replay | NOT TESTED |  |
| Best 2 out of 3 | NOT TESTED |  |
| One-round mode, if supported | NOT TESTED |  |
| Round pips | NOT TESTED |  |
| Rematch | NOT TESTED |  |
| Fighter select routing | NOT TESTED |  |
| Stage select routing | PASS | Escape from fight returned to Stage Select. |
| Mode select routing | PASS | Escape chain returned to main/mode select. |

## 8. Evil Ryu / Evil Ken Smoke Tests

| Check | Status | Notes |
| --- | --- | --- |
| Evil Ken load | PASS | Screenshot-backed visual evidence plus `--verify evilken-smoke` scripted runtime load. |
| Evil Ken idle | PASS | Screenshot-backed visual evidence plus scripted `state=0 anim=0`. |
| Evil Ken walk/repositioning | PASS | Internal scripted input path verified: movement delta `127.250015`. Physical keyboard held input remains manual. |
| Crouch | NOT TESTED |  |
| Evil Ken jump/airborne | PASS | Internal scripted input path verified `airborne_observed=1`; screenshot also shows airborne frames. |
| One normal | PASS | Internal scripted CMD/CNS path verified: `command=x state_after=206 anim_after=206`. |
| One special | NOT TESTED |  |
| One super startup, if reachable | NOT TESTED |  |
| SuperPause, if reachable | NOT TESTED |  |
| Super sound behavior, if reachable | NOT TESTED |  |
| No match-start animation glitch | NOT TESTED |  |
| No match-start sound glitch | NOT TESTED |  |
| No match-start death | NOT TESTED |  |
| No crash on reset/back out | PASS | `--verify evilken-smoke` completed with exit code `0`. Manual reset/backout remains separate. |
| Round intro overlay | PASS | Screenshot-backed visual evidence shows `ROUND 1`. |
| Timer countdown | PASS | Screenshot-backed visual evidence plus scripted timer stability `timer_ticks=5650`. |
| Combo counter display | PASS | Screenshot-backed `4 Rush!` plus scripted `combo_hits=1`; full Evil Ken damage compatibility remains unproven because smoke hit damage was `0`. |

## 9. Controller Input

| Check | Status | Notes |
| --- | --- | --- |
| P1 gamepad assignment | BLOCKED | No controller verified in this pass yet. |
| P2 gamepad assignment | BLOCKED | No controller verified in this pass yet. |
| Xbox labels | BLOCKED | No controller verified in this pass yet. |
| PlayStation labels | BLOCKED | No controller verified in this pass yet. |
| Movement | BLOCKED | No controller verified in this pass yet. |
| Face buttons | BLOCKED | No controller verified in this pass yet. |
| Start/pause | BLOCKED | No controller verified in this pass yet. |
| Back/cancel | BLOCKED | No controller verified in this pass yet. |
