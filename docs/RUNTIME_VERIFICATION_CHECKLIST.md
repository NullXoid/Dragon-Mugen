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
| Held left movement | BLOCKED | Automation API has no reliable held-key primitive for this SDL input path. |
| Held right movement | BLOCKED | Automation API has no reliable held-key primitive for this SDL input path. |
| Held crouch | BLOCKED | Automation API has no reliable held-key primitive for this SDL input path. |
| Jump | BLOCKED | Gameplay letter/direction input was not reliable through automation. |
| Dash forward, if implemented | BLOCKED | Requires held/repeated directional verification; manual pass required. |
| Dash backward, if implemented | BLOCKED | Requires held/repeated directional verification; manual pass required. |
| `x/y/z` buttons | BLOCKED | `A`/`S` presses produced no visible attack in captured frames; likely automation/input limitation, not marked FAIL. |
| `a/b/c` buttons | BLOCKED | Not attempted after `x/y/z` could not be observed reliably. |
| Hold directions | BLOCKED | Automation API has no reliable held-key primitive for this SDL input path. |
| Release directions | BLOCKED | Requires reliable held-key input. |
| Simultaneous buttons | BLOCKED | Requires reliable gameplay input injection. |

## 4. Keyboard P2 Input

| Check | Status | Notes |
| --- | --- | --- |
| Held left movement | NOT TESTED |  |
| Held right movement | NOT TESTED |  |
| Crouch | NOT TESTED |  |
| Jump | NOT TESTED |  |
| `x/y/z` buttons | NOT TESTED |  |
| `a/b/c` buttons | NOT TESTED |  |
| P2 can be controlled where expected | NOT TESTED |  |
| P1/P2 face each other | NOT TESTED |  |
| Collision/player push works | NOT TESTED |  |

## 5. Training Mode

| Check | Status | Notes |
| --- | --- | --- |
| Dummy appears | PASS | Dummy was visible in Training fight view. |
| `F1` hitbox toggle | PASS | Hitboxes appeared after `F1`. |
| `F2` training options | PASS | Training Options panel opened after `F2`. |
| `R` reset | PARTIAL | `R` was pressed, but no distinct visual state change proved reset from screenshot evidence. |
| Dummy invincibility | NOT TESTED |  |
| Dummy auto-life | NOT TESTED |  |
| Dummy freeze | NOT TESTED |  |
| Dummy guard mode | NOT TESTED |  |
| Guard damage toggle | NOT TESTED |  |
| P2-controlled training option | NOT TESTED |  |
| Command HUD | PASS | Command panel visible in fight view. |
| Input HUD | PASS | Input HUD area visible in fight view. |
| Training power mode | NOT TESTED |  |
| Move category filter | NOT TESTED |  |
| Move list derived from CMD `State -1` | NOT TESTED |  |
| Flying Dragon-inspired command display | NOT TESTED |  |

## 6. KFM Baseline Combat

| Check | Status | Notes |
| --- | --- | --- |
| Idle | PASS | KFM idle pose/animation visible in fight view. |
| Walk forward/back | BLOCKED | Requires reliable held directional input; automation could not provide it. |
| Crouch | BLOCKED | Requires reliable held down input; automation could not provide it. |
| Jump | BLOCKED | Gameplay directional input was not reliable through automation. |
| Standing normals | BLOCKED | `A`/`S` presses produced no visible attack in captured frames; manual verification required. |
| Crouching normals | BLOCKED | Requires reliable held down plus button input. |
| Hit detection | NOT TESTED | Depends on attack input or controlled positioning. |
| Damage | NOT TESTED | Depends on hit detection. |
| Guard contact | NOT TESTED | Depends on attack and guard setup. |
| Guard damage | NOT TESTED | Depends on guard contact. |
| Hit pause | NOT TESTED | Depends on hit detection. |
| Hit spark | NOT TESTED | Depends on hit detection. |
| Hit sound | NOT TESTED | Depends on hit detection. |
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
| Load | NOT TESTED |  |
| Idle | NOT TESTED |  |
| Walk | NOT TESTED |  |
| Crouch | NOT TESTED |  |
| Jump | NOT TESTED |  |
| One normal | NOT TESTED |  |
| One special | NOT TESTED |  |
| One super startup, if reachable | NOT TESTED |  |
| SuperPause, if reachable | NOT TESTED |  |
| Super sound behavior, if reachable | NOT TESTED |  |
| No match-start animation glitch | NOT TESTED |  |
| No match-start sound glitch | NOT TESTED |  |
| No match-start death | NOT TESTED |  |
| No crash on reset/back out | NOT TESTED |  |

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
