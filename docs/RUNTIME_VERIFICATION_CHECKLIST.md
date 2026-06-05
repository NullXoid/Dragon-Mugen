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
| Settings screen opens | PASS | User manually navigated to Options and provided screenshot evidence after Options menu extraction. Prior automation did not navigate there; user later clarified the game window must be clicked/focused before automation keyboard input registers. |
| Training path opens | PASS | Enter from main opened Training character select. |
| Single Player path opens | PASS | User-supplied pause/resume screenshots confirm Single Player reaches fight/pause presentation. |
| VS path opens | PASS | User-supplied VS pause and controller/P2 fight screenshots confirm VS reaches fight presentation. |
| Character select opens | PASS | KFM, Evil Ryu, and Evil Ken portraits/icons visible. |
| Stage select opens | PASS | Enter from character select opened Stage Select. |
| VS screen opens | PASS | Enter from stage select opened Training VS screen. |
| Fight view opens | PASS | Auto-advance from VS loaded KFM, Dummy, and Mountainside Temple. |
| Escape/back routing works | PASS | Fight -> Stage Select -> Character Select -> Main observed. |
| Clean exit works | PASS | Escape from main closed the app; no Dragon MUGEN window remained. |
| Post-main-menu extraction GUI smoke | PASS | After render-only main menu extraction, GUI smoke confirmed no-argument launch, M.U.G.E.N title art, `DRAGON MUGEN CORE`, Training highlight, footer/help text, Training -> character select -> stage select -> VS -> fight, visible normal-attack effect, F1 hitboxes, F2 Training Options, `R`, Escape backout, and clean exit. |
| Post-options-menu extraction GUI smoke | PASS | After render-only Options menu extraction, user-supplied screenshot evidence confirmed the Options screen renders with title/header, selected-row highlight, timer/canvas/UI scale/pad/gamepad values, and Back row. |
| Post-stage-select extraction GUI smoke | PASS | After render-only Stage Select extraction, GUI smoke clicked/focused the game window, reached Main -> Training -> KFM -> Stage Select, confirmed Stage Select title/arena counter/list/highlight/details/footer, confirmed Esc returned to Character Select, confirmed Enter reached fight, then confirmed command/input HUD, F1 hitboxes, F2 Training Options, `R`, Escape backout, and clean exit. |
| Post-VS-loading extraction GUI smoke | PASS | After render-only VS/loading extraction, GUI smoke clicked/focused the game window, reached Main -> Training -> KFM -> Stage Select -> VS/loading, confirmed VS/loading title/panels/portraits/placeholders/VS/stage/ready text, confirmed auto-transition to fight, then confirmed command/input HUD, F1 hitboxes, F2 Training Options, `R`, Escape backout, and clean exit. |
| Post-character-select extraction GUI smoke | PASS | After render-only Character Select extraction, GUI smoke clicked/focused the game window, reached Main -> Training -> Character Select, confirmed title/roster icons/KFM portrait/Dummy panel/cursor/stage label/footer, confirmed Enter to Stage Select, Escape from Stage Select back to Character Select, Enter to VS/fight, command/input HUD, F1 hitboxes, F2 Training Options, `R`, Escape backout, and clean exit. Computer Use arrow-key cursor movement stayed blocked and was not marked PASS. |

## 2. Character And Stage Selection

| Check | Status | Notes |
| --- | --- | --- |
| KFM selectable | PASS | KFM was the selected live character. |
| Evil Ryu selectable | PASS | Evil Ryu portrait/icon was visible, and user-supplied VS/controller evidence shows Evil Ryu loaded into fight. |
| Evil Ken selectable | PARTIAL | Evil Ken portrait/icon visible; selection was not navigated due automation arrow-key limitation. |
| Character Select rendering after extraction | PASS | `CharacterSelectOverlay.h` extraction preserved Character Select title, visible KFM/Evil Ryu/Evil Ken icons, KFM portrait/name, Dummy/opponent panel, cursor/highlight, stage label, and footer/help text. |
| Character Select cursor Right/Left after extraction | BLOCKED | Computer Use `Right` and `d` key attempts did not move the Character Select cursor in this pass, while Enter/Escape routing still worked. Keep this as automation-blocked, not a runtime FAIL. |
| Character Select invalid-slot rejection after extraction | NOT TESTED | Not attempted because cursor movement was blocked by automation. |
| Character Select Enter routing after extraction | PASS | Enter from Character Select reached Stage Select; Enter from Stage Select and VS/loading reached the Training fight. |
| Character Select Esc routing after extraction | PASS | Escape from Stage Select returned to Character Select during the post-extraction smoke. |
| `DragonBench` folder not selectable unless listed in `select.def` | PARTIAL | Not visible on observed first select page; exhaustive navigation blocked by automation arrow-key limitation. |
| `A.Ben` folder not selectable unless listed in `select.def` | PARTIAL | Not visible on observed first select page; exhaustive navigation blocked by automation arrow-key limitation. |
| `I.Chie` folder not selectable unless listed in `select.def` | PARTIAL | Not visible on observed first select page; exhaustive navigation blocked by automation arrow-key limitation. |
| Mountainside Temple selectable | PASS | Stage Select showed Mountainside Temple selected. |
| Stage Select rendering after extraction | PASS | `StageSelectOverlay.h` extraction preserved Stage Select title, list, selected highlight, selected-stage details, fighter/opponent labels, and footer/help text. |
| Stage Select Enter routing after extraction | PASS | Enter from Stage Select proceeded to VS/fight with Mountainside Temple. |
| Stage Select Esc routing after extraction | PASS | Escape from Stage Select returned to Character Select. |
| Fight loads selected character | PASS | Fight view loaded Kung Fu Man in Training, and user-supplied VS/controller evidence loaded Evil Ryu. |
| Fight loads selected stage | PASS | Fight view loaded Mountainside Temple. |
| VS/loading render after extraction | PASS | `VsScreenOverlay.h` extraction preserved `TRAINING VS`, P1/opponent panels, portraits/placeholders, `VS`, stage label, and ready/auto-start text. |
| VS/loading transition after extraction | PASS | Existing auto-start timing still transitioned from VS/loading to fight without changing timing. |

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
| P1/P2 face each other | PASS | User-supplied VS/P2 fight screenshot shows both fighters facing in local Player 2 fight. |
| Collision/player push works | PARTIAL | User-supplied console logs show P1/P2 hit contact in local fight; player-push-only behavior was not isolated. |

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
| Post-main-menu extraction Training smoke | PASS | After render-only main menu extraction, GUI smoke reached Training fight view, command/input HUD rendered, a normal-attack effect was visible, F1 hitboxes toggled, F2 Training Options opened, `R` did not crash, and Escape backed out to main/exit. |
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
| Match pause menu rendering after extraction | PASS | Render-only pause menu functions were extracted and post-extraction build/scripted verifiers passed. User-supplied screenshots on 2026-06-04 confirm the Single Player and VS Mode pause menus render with all expected rows and selected-row highlight, and that Resume returns to fight. Restart/Fighter Select/Stage Select/Mode Select actions remain separate untested pause-menu actions. |
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
| Evil Ryu local VS load | PASS | User-supplied controller/P2 evidence shows Evil Ryu loaded into VS/local Player 2 fight. |
| Evil Ryu local VS hit logs | PASS | User-supplied console output shows P1 and P2 Evil Ryu hit events with damage, spark, and sound fields. This is a local fight smoke pass, not full Evil Ryu compatibility. |

## 9. Controller Input

| Check | Status | Notes |
| --- | --- | --- |
| P1 gamepad assignment | PASS | User-supplied Options screenshot shows a PlayStation/DualSense pad assigned for P1, and console output detects `DualSense Edge Wireless Controller (PlayStation)`. |
| P2 gamepad assignment | PARTIAL | User verified a local Player 2 fight and controller smoke; a separate screenshot of P2 gamepad assignment was not recorded. |
| Xbox labels | NOT TESTED | PlayStation labels were tested; Xbox label mode still needs hardware/manual verification. |
| PlayStation labels | PASS | Options and fight HUD screenshots show PlayStation-style labels such as `Sq/Tri/L1` and `Cross/Cir/R1`. |
| Movement | PASS | User reported controller works in live fight; VS screenshot shows active Evil Ryu vs Player 2 fight. Detailed per-axis matrix can be expanded later. |
| Face buttons | PASS | User-supplied console logs show P1/P2 hit events during controller/P2 fight smoke. Detailed per-button matrix can be expanded later. |
| Start/pause | PARTIAL | Fight HUD renders `Start pause`; the Start button pause action was not separately captured. |
| Back/cancel | NOT TESTED | No separate controller back/cancel evidence recorded yet. |
