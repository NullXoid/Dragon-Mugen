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
| Post-fight-HUD extraction GUI smoke | PASS | After render-only Fight HUD extraction, GUI smoke clicked/focused the game window, reached Main -> Training -> KFM -> Mountainside Temple -> fight, confirmed P1/P2 names, lifebars, power gauges, `POWER` labels, bottom status/help line, command/input HUD, F1 hitboxes, F2 Training Options, `R`, visible normal-attack effect, Escape backout, and clean exit. Timer/round pips were not separately tested in a match mode during this pass. |
| Post-fight-result-overlay extraction GUI smoke | PASS | After shared/result overlay extraction, GUI smoke clicked/focused the game window, reached Main -> Training -> KFM -> Mountainside Temple -> VS -> fight, confirmed Fight HUD names/lifebars/power gauges/`POWER` labels, command/input HUD, F1 hitboxes, F2 Training Options, `R`, normal-input smoke, Escape backout, and clean exit. Round/result overlay screens were not separately forced in this pass. |
| Post-AppTypes extraction GUI smoke | PASS | After safe app/UI constants and pure-data structs were extracted to `engine/src/AppTypes.h`, GUI smoke clicked/focused the game window, reached Main -> Training -> KFM -> Mountainside Temple -> VS -> fight, confirmed Fight HUD, command/input HUD, F1 hitboxes, F2 Training Options, `R`, normal-input smoke, Escape backout, and clean exit. No behavior/input/routing/loading/gameplay/controller/CMake/sidecar changes were made. |
| Post-FrontendMenu extraction GUI smoke | PARTIAL | After pure frontend decision helpers were extracted to `engine/src/FrontendMenu.h/.cpp`, build, `dev_check`, and both scripted verifiers passed. GUI smoke clicked/focused the game window and confirmed Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight, Fight HUD, command/input HUD, F1 hitboxes, F2 Training Options, `R`, Escape backout, and clean exit. Computer Use delivered Return/Escape/F1/F2/R but did not visibly deliver SDL arrow or letter-key input, so Options navigation, selector/cursor movement, and normal-input GUI smoke remain automation-blocked for this pass. |
| Post-FrontendMenu manual keyboard/controller smoke | PASS | User manual testing after `4581704` confirmed the keyboard and controller frontend/menu smoke works. Evidence includes Character Select movement to Evil Ken and Evil Ryu, Stage Select, VS/loading, Training fight, command/input HUD, F1 hitboxes, F2 Training Options, normal/hit evidence, and controller operation. Two air-state bugs were found and recorded separately in `docs/BUGS.md`. |
| Post-SelectionState extraction GUI smoke | PASS | After character/stage selection metadata was extracted to `engine/src/SelectionState.h/.cpp`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, and `kfm-air-state` passed. GUI smoke clicked/focused the game window and confirmed Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight, Fight HUD, command/input HUD, F1 hitboxes, F2 Training Options, `R`, one normal input smoke, Escape backout, and clean exit. Computer Use cursor movement stayed automation-blocked. |
| Post-FrontendState extraction GUI smoke | PASS | After UI/front-end state storage was extracted to `engine/src/FrontendState.h`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, and `kfm-air-state` passed. GUI smoke clicked/focused the game window and confirmed Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight, Fight HUD, command/input HUD, F1 hitboxes, F2 Training Options, `R`, one normal input smoke, air-state spot check, Escape backout, and clean exit. Routing/loading/runtime behavior stayed in `App.cpp`; CPU passive behavior remains open. |
| Post-FrontendFlow extraction GUI smoke | PARTIAL | After existing key-flow dispatcher functions moved to transitional `engine/src/FrontendFlow.h`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, and `kfm-air-state` passed. GUI smoke clicked/focused the game window and confirmed Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight, Fight HUD, command/input HUD, F1 hitboxes, F2 Training Options, `R`, one normal input smoke, Escape backout, and clean exit. Computer Use still did not move the main selector into Options, so Options/selector movement remains automation-blocked for this pass. |
| Post-TrainingOptionsBehavior extraction GUI smoke | PASS | After pure Training Options behavior helpers moved to normal module `engine/src/TrainingOptionsBehavior.h/.cpp`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, and `kfm-air-state` passed. GUI smoke clicked/focused the game window and confirmed Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight, F2 Training Options labels/statuses, Hitboxes toggle ON and restore OFF, F2 close, F1 hitboxes, `R`, Escape backout, and clean exit. Runtime side effects, CPU behavior, loading, routing, and sidecar policy stayed unchanged. |
| Post-UiRenderHelpers extraction GUI smoke | PASS | After shared render-only UI helpers moved to transitional `engine/src/UiRenderPrimitives.h` and `engine/src/UiRenderHelpers.h`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, and `kfm-air-state` passed. GUI smoke clicked/focused the game window and confirmed title background, Character Select background, roster icons, fixed Dummy slot, labels/footer, Stage Select, VS/loading, fight HUD, command/input HUD, F1, F2, `R`, one normal input, jump spot-check, Escape backout, and clean exit. Sprite/resource ownership, loading, runtime rendering, gameplay, and CPU behavior stayed unchanged. |
| CPU baseline scripted smoke | PASS | After the focused CPU bug fix and follow-up P2 hit-resolution fix, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. `cpu-baseline` confirms Single Player fight phase, P2 movement toward P1, P2 attack state/action `200`, CPU P2 damaging P1, P1 still hitting/damaging P2, timer stability, and clean exit through the normal `FighterInputState`/CMD/CNS/HitDef path. |
| Post-TrainingState scripted and live checkpoint | PASS | After Training Options storage moved into normal header `engine/src/TrainingState.h`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. User live retest after `4e80ad7` confirmed the Training route, F2 Training Options labels/statuses, F1, `R`, one normal input, diagonal jump landing, air attack landing, Escape backout, clean exit, CPU baseline behavior, and match-result controller D-pad navigation still work. |
| Post-FightMessageState scripted checkpoint | PASS | After shared fight-message storage moved into normal header `engine/src/FightMessageState.h`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. `RuntimeSnapshot::lastHitText` still reports hit-message evidence through `state.messages.lastHitText`. Manual GUI smoke was not rerun in this pass. |
| Post-FightDisplayState scripted checkpoint | PASS | After env-shake display storage and combo counter storage moved into normal header `engine/src/FightDisplayState.h`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. `RuntimeSnapshot::comboHits` still reports combo evidence through `state.display.comboCounters`. Env-shake trigger/update behavior, combo mutation, camera, global pause, match/round state, hit/damage, rendering, CPU behavior, loading, runtime, and sidecar policy stayed in `App.cpp`. Manual GUI smoke was not rerun in this pass. |
| Post-PauseMenuOverlay normal module scripted checkpoint | PASS | After `PauseMenuOverlay.h` became a normal declaration header and render-only drawing moved to `engine/src/PauseMenuOverlay.cpp`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. The module uses `PauseMenuView`, `UiRenderContext`, and public UI primitives instead of App.cpp-local `AppState`. Pause input, selected-option mutation, routing, fight runtime, and screen transitions stayed in `App.cpp` / `FrontendFlow.h`. Manual GUI smoke was not rerun in this pass. |
| Post-MainMenuOverlay normal module scripted checkpoint | PASS | After `MainMenuOverlay.h` became a normal declaration header and foreground drawing moved to `engine/src/MainMenuOverlay.cpp`, build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. The module uses `MainMenuView`, `UiRenderContext`, and public UI primitives instead of App.cpp-local `AppState`. Title background/logo resource drawing, selected-mode state, routing, input, and screen transitions stayed in App.cpp / existing modules. Manual GUI smoke was not rerun in this pass. |

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
| FrontendMenu route after extraction | PASS | `FrontendMenu` extraction preserved Return-driven Training route from Main -> Character Select -> Stage Select -> VS/loading -> fight. |
| FrontendMenu selector/cursor movement after extraction | BLOCKED | Computer Use arrow-key injection did not visibly move SDL main, character, or stage selectors after clicking/focusing the window. This is recorded as automation-blocked, not runtime failure; manual keyboard/controller evidence remains separate. |
| FrontendMenu selector/cursor manual smoke | PASS | User manual testing after `4581704` confirms keyboard/controller selector movement worked; screenshots show Character Select cursor movement to Evil Ken and Evil Ryu. |
| SelectionState boundary route after extraction | PASS | `SelectionState` extraction preserved Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight. Character Select still showed KFM/Evil Ryu/Evil Ken; Stage Select still showed Mountainside Temple with KFM/Dummy details. |
| SelectionState cursor movement after extraction | BLOCKED | Computer Use `Right` did not visibly move the Character Select cursor after clicking/focusing the window. This remains an automation limitation; scripted verifiers and route smoke still passed. |
| FrontendState boundary route after extraction | PASS | `FrontendState` extraction preserved Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight. `screenFrame` reset/increment behavior, route side effects, and loading behavior stayed in `App.cpp`. |
| FrontendFlow boundary route after extraction | PASS | `FrontendFlow.h` extraction preserved Main -> Training -> Character Select -> Stage Select -> VS/loading -> fight and the FightView F1/F2/R/normal/Escape key paths. |
| FrontendFlow selector/options movement after extraction | BLOCKED | Computer Use `Down` still did not visibly move the SDL main selector into Options after clicking/focusing the game window. This remains an automation limitation; manual keyboard/controller evidence remains separate. |
| UiRenderHelpers route/render after extraction | PASS | Shared UI helper extraction preserved the title background, Character Select background, roster icons, fixed Dummy slot, labels/footer, Stage Select, VS/loading, and fight route. |
| UiRenderPrimitives normal seam | PASS | `UiRenderContext.h` and normal `UiRenderPrimitives.h/.cpp` now provide AppState-free UI primitives. Build, `dev_check`, `kfm-baseline`, `evilken-smoke`, `kfm-air-state`, and `cpu-baseline` passed. Manual GUI smoke was not rerun for this primitive-only pass. |
| PauseMenuOverlay normal module conversion | PASS | `PauseMenuOverlay.h` now declares `PauseMenuView` and `drawSingleFightPauseMenu`, while `PauseMenuOverlay.cpp` implements render-only drawing without App.cpp-local types. Build and scripted verifiers passed. Manual pause-menu GUI smoke was not rerun in this pass. |
| MainMenuOverlay normal module conversion | PASS | `MainMenuOverlay.h` now declares `MainMenuView` and foreground draw functions, while `MainMenuOverlay.cpp` implements AppState-free foreground menu drawing without App.cpp-local types. Build and scripted verifiers passed. Manual main-menu GUI smoke was not rerun in this pass. |

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
| Post-fight-HUD extraction Training HUD smoke | PASS | `FightHudOverlay.h` extraction preserved the existing Training command/input HUD, F1 hitboxes, F2 Training Options, `R`, visible normal-attack effect, Escape backout, and clean exit. Training command/input HUD code was not moved in this pass. |
| Post-FrontendMenu extraction Training route smoke | PARTIAL | GUI smoke after `FrontendMenu` extraction confirmed fight HUD, command/input HUD, F1 hitboxes, F2 Training Options, `R`, Escape backout, and clean exit. Computer Use letter-key input did not visibly trigger a normal in this pass, so normal-input GUI smoke is automation-blocked here; `--verify kfm-baseline` still proves normal routing. |
| Post-training-overlay extraction GUI smoke | PASS | After render-only overlay extraction, GUI smoke reached title -> character select -> stage select -> VS -> fight, command/input HUD rendered, F1 hitboxes toggled, F2 Training Options opened, `R` did not crash, and Escape backed out to main/exit. User-supplied screenshots on 2026-06-04 also confirm command/input HUD, F1 hitboxes, F2 Training Options, stage select backout, and Training overlay persistence after the pause-menu extraction. Computer Use could not perform a true held-key physical retest, so held input status remains based on scripted verification and prior user manual evidence. |
| Post-main-menu extraction Training smoke | PASS | After render-only main menu extraction, GUI smoke reached Training fight view, command/input HUD rendered, a normal-attack effect was visible, F1 hitboxes toggled, F2 Training Options opened, `R` did not crash, and Escape backed out to main/exit. |
| Post-TrainingOptionsBehavior Training option smoke | PASS | `TrainingOptionsBehavior` extraction preserved F2 Training Options labels/status values and pure option mutation. Hitboxes toggled ON from the selected row and was restored OFF; F2 closed correctly. F1 and `R` still worked, and the Escape backout/clean exit path remained intact. |
| Post-TrainingState storage split | PASS | `TrainingState.h` now stores `TrainingOptions options`, and all call sites use `state.training.options`. Scripted verifier coverage stayed green after the storage split. User live retest after `4e80ad7` confirmed F2 Training Options labels/statuses still render and the broader Training smoke still works. |
| Post-FightMessageState storage split | PASS | `FightMessageState.h` now stores `lastHitText` and `lastHitTextTicks`, and all AppState call sites use `state.messages`. Scripted verifier coverage stayed green after the storage split. Manual hit-message display smoke was not rerun in this pass. |
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
| Diagonal held jump limit | PASS | `--verify kfm-air-state` now proves Up + Forward and Up + Back from neutral and from held walk each enter air, land by the frame limit, and finish grounded with `reentered_air_after_landing=0`, `final_y=0.000000`, `final_vy=0.000000`, `final_state=0`, and `final_state_type=S`. This restores one-jump default behavior; double jump remains a future explicit setting. Physical keyboard/controller retest is still useful but not claimed by automation here. |
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
| Air attack landing | PASS | `--verify kfm-air-state` now proves jump -> air attack reaches an air attack state and lands with `saw_air_attack=1`, `landed_after_attack=1`, `final_y=0.000000`, `final_vy=0.000000`, `final_state=0`, and `final_on_ground=1`. Physical keyboard/controller retest is still useful but not claimed by automation here. |
| KO | NOT TESTED | Depends on damage flow. |
| Round reset/rematch or back out | PARTIAL | Back out from fight was verified; round reset/rematch was not. |

## 7. Match Rules

| Check | Status | Notes |
| --- | --- | --- |
| `RoundStart` | PARTIAL | Fight loaded through VS; specific RoundStart callout/timing not independently verified. |
| `Fight` | PASS | Fight view loaded and stayed active. |
| Match pause menu rendering after extraction | PASS | Render-only pause menu functions were extracted and post-extraction build/scripted verifiers passed. User-supplied screenshots on 2026-06-04 confirm the Single Player and VS Mode pause menus render with all expected rows and selected-row highlight, and that Resume returns to fight. Restart/Fighter Select/Stage Select/Mode Select actions remain separate untested pause-menu actions. |
| Single Player match-complete screen | PASS | User-supplied screenshot on 2026-06-05 confirms Single Player can reach `MATCH COMPLETE`, show Evil Ken winning `2 - 0` by decision, display Mountainside Temple, and render result-menu rows. This is match-flow evidence only. |
| Match-result controller D-pad navigation | PASS | User originally reported controller confirm/cancel buttons worked on the `MATCH COMPLETE` screen but D-pad Up/Down did not move the result-menu highlight. `FrontendFlow.h` now maps match-result `SDL_GAMEPAD_BUTTON_DPAD_UP/DOWN` to the same `SDLK_UP/DOWN` path as keyboard navigation. User live retest after `4e80ad7` confirmed controller D-pad Up/Down now moves the result-menu highlight and confirm/cancel still work. |
| Single Player CPU active fighting baseline | PASS | `--verify cpu-baseline` proves CPU-controlled P2 now feeds `FighterInputState` through `updateControlledFighter`, moves toward P1, attempts attack state/action `200`, damages P1 through P2 HitDef resolution, can still be hit/damaged by P1, and keeps Single Player timer stability. This is a deterministic baseline, not advanced AI or Single Player hitbox-debug display coverage. |
| Timer/round pips after Fight HUD extraction | NOT TESTED | Fight HUD extraction GUI smoke was Training-only. Single Player/VS timer and round-pip rendering should be checked in a later match-mode smoke before marking this row PASS. |
| Round/result overlay rendering after extraction | NOT TESTED | `FightResultOverlay.h` extraction built and scripted verifiers passed, and Training regression smoke passed. Dedicated RoundStart/RoundFinish/RoundResult/MatchResult screens were not forced or separately observed in this pass. |
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
