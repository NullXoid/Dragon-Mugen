# App.cpp Type And Extraction Scope Audit

This audit is documentation-only. It records what remains in `engine/src/App.cpp` after the safe app/UI type extraction and recommends the next extraction passes based on current line ranges, dependency shape, and verifier coverage.

## Baseline

| Item | Current State |
| --- | --- |
| Baseline commit | `afc824a Extract safe app state types from App.cpp` |
| `engine/src/App.cpp` count | `15456` file-size-guard lines |
| Known oversized files | `engine/src/App.cpp` 15456, `engine/tools/audit_mugen_compat.py` 707, `engine/src/MugenData.cpp` 532 |
| `kfm-baseline` | `pass=11 partial=1 fail=0 blocked=0` |
| `evilken-smoke` | `pass=7 partial=0 fail=0 blocked=0` |

`engine/src/AppTypes.h` already owns the Pass 11 safe app/UI constants, enums, and pure-data structs. Do not duplicate those definitions in a later header.

## Already Extracted From App.cpp

| Header | Responsibility | Transitional or Normal | Notes |
| --- | --- | --- | --- |
| `Input.h` | input controls, fighter input state, command input frame, gamepad device metadata | Normal/partial | Already owns `FighterInputState`, `FighterControls`, `CommandInputFrame`, and `GamepadDevice`. |
| `TrainingDebugOverlay.h` | F1 hitbox/debug overlay rendering | Transitional App.cpp internal | Render-only; depends on App.cpp-local render helpers/state. |
| `TrainingCommandOverlay.h` | Training command/input HUD rendering | Transitional App.cpp internal | Render-only; helper formatting remains partly in App.cpp. |
| `TrainingOptionsOverlay.h` | F2 Training Options and move-list rendering | Transitional App.cpp internal | Render-only; pure option labels/status/toggle behavior now lives in `TrainingOptionsBehavior.h/.cpp`; runtime side effects remain outside the overlay. |
| `PauseMenuOverlay.h` | match pause menu rendering | Transitional App.cpp internal | Render-only; pause behavior remains in App.cpp. |
| `MainMenuOverlay.h` | main/title menu rendering | Transitional App.cpp internal | Render-only; menu selection/routing remains in App.cpp. |
| `OptionsMenuOverlay.h` | Options screen rendering | Transitional App.cpp internal | Render-only; settings mutation and device behavior remain in App.cpp. |
| `StageSelectOverlay.h` | Stage Select rendering | Transitional App.cpp internal | Render-only; stage selection/routing/loading remains in App.cpp. |
| `VsScreenOverlay.h` | VS/loading screen rendering | Transitional App.cpp internal | Render-only; fight preparation and loading remain in App.cpp. |
| `CharacterSelectOverlay.h` | Character Select rendering | Transitional App.cpp internal | Render-only; cursor and selection behavior remain in App.cpp. |
| `FightHudOverlay.h` | fight HUD rendering | Transitional App.cpp internal | Render-only; fight state mutation remains in App.cpp. |
| `FightPresentationShared.h` | shared fight presentation helpers | Transitional App.cpp internal | Shared render helpers for HUD/result overlays. |
| `FightResultOverlay.h` | round/result presentation rendering | Transitional App.cpp internal | Render-only; round/match flow remains in App.cpp. |
| `AppTypes.h` | safe app/UI constants, enums, pure-data structs | Normal header | Owns Pass 11 safe types; no SDL/resource ownership. |
| `FrontendMenu.h` / `FrontendMenu.cpp` | pure frontend menu decision helpers | Normal module | Pass 12 moved requested-action decisions, pure cursor/index movement, and copied settings cycling. App.cpp still owns SDL conversion and all side effects. |
| `SelectionState.h` / `SelectionState.cpp` | character/stage selection metadata and pure lookup/display helpers | Normal module | Pass 13 moved selected character/stage/session metadata. `MugenData` still owns roster/stage catalog loading, and App.cpp still owns runtime loading and routing side effects. |
| `FrontendState.h` | UI/front-end state storage | Normal header | Pass 14 moved screen, pending mode, selected mode, menu rail, pause/result selected indices, and `screenFrame` storage. Routing and `screenFrame` reset/increment behavior remain in App.cpp. |
| `FrontendFlow.h` | existing key-flow dispatcher | Transitional App.cpp internal | Pass 15 moved `frontendKeyFromSdl`, `settingCycleDirection`, `handleKey`, and `gamepadMenuKeyForButton`. This removes physical lines from App.cpp but still depends on AppState and App.cpp-local helpers. |
| `TrainingOptionsBehavior.h` / `TrainingOptionsBehavior.cpp` | pure Training Options labels/status/cycling/toggle behavior | Normal module | Pass 16 moved helpers that read/write only `TrainingOptions`. Runtime side effects, F1/F2/R dispatch, loading, routing, fighter mutation, CPU behavior, and sidecar policy remain outside the module. |
| `UiRenderPrimitives.h` | shared UI render primitives and scale/text helpers | Transitional App.cpp internal | Pass 17 moved color/rect drawing, scaled debug text, UI scale scope, fit text, and panel helpers. It still depends on App.cpp-local `AppState` and SDL renderer state. |
| `UiRenderHelpers.h` | shared title/select background and fixed opponent-slot presentation | Transitional App.cpp internal | Pass 17 moved render-only helpers that draw already-loaded title/select sprites and fixed placeholder slots. Sprite helpers, texture creation, and resource ownership remain in App.cpp. |
| `TrainingState.h` | Training Options state storage | Normal header | Pass 19 moved only `TrainingOptions options` storage under `AppState::training`. `lastHitText` and `lastHitTextTicks` intentionally stayed out of TrainingState and moved later to `FightMessageState.h`. |
| `FightMessageState.h` | shared fight-message display storage | Normal header | Pass 20 moved only `lastHitText` and `lastHitTextTicks` storage under `AppState::messages`. Hit/damage, round-flow, HUD rendering, verifier behavior, and runtime systems remain in App.cpp. |
| `FightDisplayState.h` | fight display storage | Normal header | Pass 21 moved only env-shake display storage and combo counter storage under `AppState::display`. Env-shake trigger/update behavior, combo mutation, camera, global pause, palette/env color, match/round state, hit/damage, rendering, CPU behavior, and runtime systems remain in App.cpp. |

## Type Inventory

| Name | Kind | Approx Lines | Current Location / Line Range | Dependencies | Risk | Suggested Target | Confidence | Notes |
| --- | --- | ---: | --- | --- | --- | --- | --- | --- |
| Pass 11 app/UI types | constants/enums/structs | 150 | `AppTypes.h` | standard library only | `SAFE PURE DATA` | Already owned by `AppTypes.h` | HIGH | Includes `Screen`, `PendingMode`, `TrainingOptions`, `MainSettings`, `ComboCounterState`, `FightSessionSlots`. Do not duplicate. |
| `SelectionState` | struct/helpers | 88 actual reduction | `SelectionState.h/.cpp` | `MugenData`, `AppTypes.h` | `MODERATE DATA` | Already owned by `SelectionState.h/.cpp` | HIGH | Stores selection metadata and pure lookup/display helpers only; runtime loading and routing remain in App.cpp. |
| `FrontendState` | struct | 14 | `FrontendState.h` | `AppTypes.h` | `MODERATE DATA` | Already owned by `FrontendState.h` | HIGH | Stores UI/front-end state only; routing, screen transitions, and `screenFrame` reset/increment behavior remain in App.cpp. |
| `TrainingState` | struct | 11 | `TrainingState.h` | `AppTypes.h` | `SAFE PURE DATA` | Already owned by `TrainingState.h` | HIGH | Stores `TrainingOptions options` only. It is a normal header, not an App.cpp-internal transitional header. |
| `FightMessageState` | struct | 12 | `FightMessageState.h` | standard library only | `SAFE PURE DATA` | Already owned by `FightMessageState.h` | HIGH | Stores shared fight-message text and countdown ticks only; message assignment and timer behavior remain in App.cpp. |
| `FightDisplayState` | struct | 19 | `FightDisplayState.h` | `AppTypes.h`, standard library array | `MODERATE DATA` | Already owned by `FightDisplayState.h` | HIGH | Stores env-shake display values and `comboCounters` only; env-shake start/update functions and combo mutation remain in App.cpp. |
| Input boundary types | structs | 53 | `Input.h` | standard library, SDL handle metadata only | `SAFE PURE DATA` | Already owned by `Input.h` | HIGH | Includes `FighterInputState`; do not move to `AppTypes.h`. |
| Guard/comparison/expression enums | enums/structs | 29 | `App.cpp:33-67` | guard logic, expression parsing, command/CNS evaluation | `BEHAVIOR-COUPLED` | `MugenExpressionTypes.h` or `StateRuntimeTypes.h` after parser split | MEDIUM | `GuardStance` is hit/guard runtime; `CompareOp`, `MugenVariableRef`, and related enums are parser/runtime expression support. |
| Texture/resource handles | structs | 8 | `App.cpp:68-75` | SDL texture ownership | `RESOURCE-COUPLED` | `RenderResourceTypes.h` after resource ownership audit | HIGH | `TextureSprite` owns raw SDL texture pointer metadata and should not move alone without destroy/lifetime policy. |
| Collision and AIR parse data | enums/structs | 37 | `App.cpp:76-82`, `1823-1840`, `2077-2086` | AIR parsing, hit boxes, actor rendering, hit detection | `RUNTIME-COUPLED` | `CollisionTypes.h` after hit/render split decision | MEDIUM | `CollisionBox` is shared by rendering and hit detection; moving it may unlock parser/render modules but touches important runtime surfaces. |
| Visual effect specs/runtime | structs/enums | 90 | `App.cpp:83-163`, plus controller refs | state controllers, palette runtime, afterimage/trans/env effects | `RUNTIME-COUPLED` | `VisualEffectTypes.h` after effect runtime audit | MEDIUM | Some specs are pure data, but active effect structs are updated by runtime controllers and rendering. |
| `HitDefinition` | struct | 89 | `App.cpp:164-252` | HitDef parser, hit runtime, damage, guard, effects/audio | `RUNTIME-COUPLED` | `HitTypes.h` later | HIGH | Do not move before hit/damage boundary is audited. |
| `RuntimeProjectile` | struct | 42 | `App.cpp:253-294` | projectile controllers, animation, hit runtime, removal behavior | `RUNTIME-COUPLED` | `ProjectileRuntimeTypes.h` later | HIGH | Tied to projectile movement, collision, pause, and hit behavior. |
| State trigger/controller types | enums/structs | about 730 | `App.cpp:295-1027` | CNS parser, trigger evaluation, runtime controller execution | `BEHAVIOR-COUPLED` | `StateControllerTypes.h` after command/CNS type audit | MEDIUM | Large line-count target, but high blast radius because types are used by parser and runtime execution. |
| Command types | structs/global pointer | about 81 | `App.cpp:1028-1080` | CMD parser, input history, command recognition, verification bridge | `RUNTIME-COUPLED` | `CommandRuntimeTypes.h` after command module audit | MEDIUM | `FightInputOverride` and `gFightInputOverride` are verification/runtime glue and should not be moved casually. |
| Animation/stage/system asset types | structs | 45 | `App.cpp:1081-1126` | SDL textures, AIR/SFF loading, stage rendering, system screens | `RESOURCE-COUPLED` | `RenderAssetTypes.h` after resource split | HIGH | `AnimationFrame` embeds `TextureSprite`; moving data alone does not solve ownership. |
| Runtime effects/audio types | structs | 54 | `App.cpp:1127-1180` | runtime visual effects, SDL audio stream, decoded samples | `RESOURCE-COUPLED` | `AudioTypes.h` and `RuntimeEffectTypes.h` later | MEDIUM | `AudioState` has SDL stream ownership; `RuntimeEffect` is fight runtime state. |
| `FighterState` | struct | 115 | `App.cpp:1181-1294` | animation, input history, CNS state, hit runtime, target binding, variables | `RUNTIME-COUPLED` | Stay in `App.cpp` for now | HIGH | Could split small sub-structs later, but the whole type is too coupled to move now. |
| `AppState` | struct | 325 | `App.cpp:1296-1367` plus aggregated fields | UI, routing, loaded content, runtime state, resources, audio, gamepads | `BEHAVIOR-COUPLED` | Split first into smaller state structs | HIGH | Do not move as a whole. It is the central dependency blocking normal modules. |
| `ScopedUiScale` | RAII struct | 29 | `App.cpp:1621-1649` | SDL renderer, AppState UI scale helpers | `RESOURCE-COUPLED` | `UiRenderScope.h` after render helper extraction | HIGH | Good small future target once render helper dependencies are public. |
| Parser-local helper structs | structs | 7 | `App.cpp:2514-2517`, `5570-5573` | command/CNS parser functions | `BEHAVIOR-COUPLED` | Keep with parser functions until parser module split | HIGH | `ParsedCommandCondition` and `StateTypeCondition` are tiny and should move only with their parser helpers. |
| `ProjectileRemovalReason` | enum | 5 | `App.cpp:12098-12102` | projectile removal runtime | `RUNTIME-COUPLED` | Keep with projectile runtime | HIGH | Moving alone provides no useful boundary. |

## Constants And Enums

| Constant/Enum | Approx Lines | Used By | Already Moved? | Safe To Move? | Suggested Header | Notes |
| --- | ---: | --- | --- | --- | --- | --- |
| Window/logical size constants | 7 | rendering, layout, SDL window | Yes | Already moved | `AppTypes.h` | Use existing definitions; do not duplicate. |
| Training/menu/count constants | 13 | training options, pause/result/options menus | Yes | Already moved | `AppTypes.h` | Use existing definitions; do not duplicate. |
| Character select grid constants | 3 | Character Select rendering/behavior | Yes | Already moved | `AppTypes.h` | Use existing definitions; do not duplicate. |
| `Screen`, `PendingMode`, `OpponentType` | 18 | routing, selection, fight setup | Yes | Already moved | `AppTypes.h` | Use existing definitions; do not duplicate. |
| `MatchPhase`, `RoundEndReason` | 13 | round/match flow and result rendering | Yes | Already moved | `AppTypes.h` | Use existing definitions; do not duplicate. |
| training and gamepad setting enums | 16 | Training Options and Options menu | Yes | Already moved | `AppTypes.h` | `GamepadPromptStyle` is already owned by `AppTypes.h`. |
| `GuardStance` | 6 | hit/guard selection and guard state entry | No | Not yet | `HitRuntimeTypes.h` later | Runtime-coupled; keep until guard/hit split. |
| `CompareOp`, `CommandConditionSubject`, `MugenVariableBank` | 24 | expression, command, CNS trigger parsing/evaluation | No | Not yet | `MugenExpressionTypes.h` later | Move with expression/parser/runtime types, not before. |
| `ActorBlendMode` | 6 | trans/afterimage/render runtime | No | Maybe | `VisualEffectTypes.h` later | Needs visual effect dependency cleanup. |
| state controller operation enums | about 23 | CNS controller parser/runtime | No | Not yet | `StateControllerTypes.h` later | Move only with controller data types. |
| `CollisionKind` | 6 | AIR collision parsing | No | Maybe | `CollisionTypes.h` later | Should move with `CollisionBox` and collision parser decisions. |
| `ProjectileRemovalReason` | 5 | projectile removal behavior | No | No | keep with projectile runtime | Runtime behavior-coupled. |

Future constants moved to normal headers should use `inline constexpr` when they are compile-time values.

## Function / Responsibility Cluster Inventory

| Cluster | Representative Functions | Approx Lines | Risk | Suggested Future Pass | Confidence | Notes |
| --- | --- | ---: | --- | --- | --- | --- |
| Already extracted input boundary | `Input.h`, input mapping integration | 53 external lines | `SAFE PURE DATA` | Already extracted | HIGH | App.cpp still owns event pumping and gamepad menu behavior. |
| Already extracted render overlays | `drawModeSelect`, `drawMainSettings`, `drawCharacterSelect`, `drawStageSelect`, `drawVersusScreen`, HUD/result/training overlays | 1175 external header lines | `SAFE PURE DATA` to `MODERATE DATA` | Convert to normal modules later | HIGH | Still transitional because they depend on App.cpp-local types/helpers. |
| App type prep | `AppTypes.h` | 150 external lines | `SAFE PURE DATA` | Already extracted | HIGH | Normal header. |
| Frontend key flow | `frontendKeyFromSdl`, `settingCycleDirection`, `handleKey`, `gamepadMenuKeyForButton` | 405 external header lines | `BEHAVIOR-COUPLED` | Completed Pass 15 transitional extraction | HIGH | Still App.cpp-internal; later replace with cleaner action-return boundary before converting to a normal module. |
| UI primitive/render helper surface | `setColor`, `scaledDebugText`, `drawPanel`, `ScopedUiScale`, title/select backgrounds, fixed opponent slot | 188 App.cpp reduction, 204 external header lines | `RESOURCE-COUPLED` | Completed Pass 17 transitional extraction | HIGH | Sprite helpers, texture creation, resource ownership, actor/fighter/stage rendering, and runtime rendering stayed in App.cpp. |
| AIR/SFF animation and stage background loading | `loadAirActionData`, `loadCharacterClips`, `loadStageBackground` | about 650 | `RESOURCE-COUPLED` | Asset/cache loading audit | MEDIUM | Touches SDL textures and M.U.G.E.N asset parsing. |
| CNS state/controller parsing | `loadStateDefinitions`, trigger/controller parse helpers | about 2300 | `BEHAVIOR-COUPLED` | State parser module later | MEDIUM | Biggest remaining monolith section, but parser/runtime coupling is high. |
| HitDef loading | `loadHitDefinitions` and helpers | about 300 | `RUNTIME-COUPLED` | Hit type/parser audit later | HIGH | Do not move before hit runtime boundary is ready. |
| CMD parsing and command state loading | `loadCommandStateEntries`, `loadCommandDefinitions`, command atom parsing | about 600 | `RUNTIME-COUPLED` | Command parser/runtime boundary later | MEDIUM | Connected to input buffer and State -1 routing. |
| Runtime character/stage/session loading | `loadSelectedCharacterRuntime`, `prepareFightSession`, `beginFight`, visual asset loaders | about 450 | `RESOURCE-COUPLED` | Loaded content state split first | MEDIUM | Needs resource lifetime and AppState split. |
| Audio scheduling/mixing | `initAudio`, `playSound`, `mixActiveSoundVoices`, `updateAudioMixer` | about 400 | `RESOURCE-COUPLED` | Audio audit or module later | MEDIUM | SDL audio stream ownership and fight sound policy are coupled. |
| Gamepad/options behavior | gamepad open/close/assignment text/cycling, settings cycling | about 330 | `MODERATE DATA` | Frontend/settings behavior pass | MEDIUM | Safer than runtime systems; controller smoke exists but per-button matrix is incomplete. |
| Selection behavior | `moveCharacterSelectCursor`, stage/character label helpers, preferred stage | about 220 | `MODERATE DATA` | Selection behavior pass | MEDIUM | Avoid asset loading and route mutation unless scoped. |
| Training option behavior | `trainingOptionStatus`, `toggleTrainingOption`, mode cycling | 231 external cpp lines, 23 header lines, 221 App.cpp reduction | `MODERATE DATA` | Completed Pass 16 normal module extraction | HIGH | Pure `TrainingOptions` label/status/cycling/toggle behavior now lives in `TrainingOptionsBehavior.h/.cpp`; runtime side effects remain outside the module. |
| Fight session reset/setup | `resetFightRound`, `resetFightState`, `prepareFightSession`, `beginFight` | about 230 | `RUNTIME-COUPLED` | Fight session setup later | MEDIUM | Central to verifiers; avoid until state split. |
| State runtime execution | expression eval, runtime controller execution, helper/projectile/explod spawns | about 3100 | `BEHAVIOR-COUPLED` | Runtime systems later | LOW | Too risky before data/state boundaries. |
| Hit/damage/guard runtime | `applyHitBetween`, guard/get-hit helpers, combo updates | about 1500 | `RUNTIME-COUPLED` | Runtime systems later | MEDIUM | Protected by KFM verifier but still high blast radius. |
| Projectile/helper runtime | projectile collision/hit/update, helper update | about 450 | `RUNTIME-COUPLED` | Runtime systems later | MEDIUM | Evil Ryu/Evil Ken compatibility depends on this. |
| Match/round flow | `updateSingleFightRules`, phase timers, round scoring, match result routing | about 550 | `RUNTIME-COUPLED` | Match rules later | MEDIUM | Dedicated KO/time-over/result tests are still missing. |
| World/fighter rendering | `drawStageLayer`, `drawActor`, runtime effect/projectile drawing, `drawFightView` | about 800 | `RESOURCE-COUPLED` | Actor/world rendering module after render types | MEDIUM | Good future cut, but depends on render/resource types. |
| Main `handleKey` and event loop | `handleKey`, `gamepadMenuKeyForButton`, `pumpEvents`, `fixedUpdate`, `runApp` | about 610 | `BEHAVIOR-COUPLED` | Frontend/menu behavior pass | MEDIUM | Can be split by screen, but must avoid fight runtime behavior creep. |
| Verification bridge | `AppVerificationBridge.h`, `runVerificationScenarioInternal` | 204 external header lines | `BEHAVIOR-COUPLED` | Normal module after state boundary | MEDIUM | Keep as bridge until AppState/FighterState are less internal. |

## AppState Dependency Map

`AppState` currently aggregates nearly every boundary in the app. It should be split into smaller state structs before the whole type moves.

| Group | Fields | Rough Lines If Separable | Future Target | Risk | Can Split Before Moving AppState? |
| --- | --- | ---: | --- | --- | --- |
| UI/menu state | `screen`, `pendingMode`, `selectedMode`, `menuRailOnLeft`, pause/result selected options, `screenFrame` | 14 actual | `FrontendState.h` | MEDIUM | Completed Pass 14. `running` and all transition behavior remain in App.cpp. |
| selection state | `characters`, `stages`, `selectedCharacter`, `loadedP1Character`, `selectedStage`, `sessionSlots` | 50-90 | `SelectionState.h` | MEDIUM | Yes, if loading behavior remains in App.cpp initially. |
| settings state | `mainSettings`, `fightRoundSettings` | 30-60 | `SettingsState.h` or `FrontendState.h` | LOW | Yes. Types already public enough for a small split. |
| training state | `TrainingState::options` | 11 actual header lines | `TrainingState.h` | LOW | Completed Pass 19. Dummy behavior and all training side effects remain in App.cpp/FrontendFlow. |
| fight/message display state | `FightMessageState::lastHitText`, `FightMessageState::lastHitTextTicks` | 12 actual header lines | `FightMessageState.h` | LOW | Completed Pass 20. Hit/damage, round-flow decisions, HUD rendering, verifier scenarios, and runtime behavior remain in App.cpp. |
| fight/display state | `FightDisplayState::envShake*`, `FightDisplayState::comboCounters` | 19 actual header lines | `FightDisplayState.h` | MEDIUM | Completed Pass 21. Env-shake trigger/update behavior, combo mutation, camera, global pause, palette/env color, match/round state, hit/damage, rendering, and runtime systems remain in App.cpp. |
| fight/match state | `matchPhase`, `roundEndReason`, timer, round wins, match flags, global pause, camera, palette/env color | 70-120 | `MatchState.h` / `RoundState.h` | HIGH | Split only after match/round checks improve. |
| fighter runtime state | `fighters`, `helpers`, `projectiles`, `runtimeEffects` | 50-100 | `FightRuntimeState.h` | HIGH | Not yet. This is tied to most runtime systems. |
| loaded content state | `characterConstants`, `hitDefs`, `stateDefs`, command entries/definitions, clips, victory quotes, `content` | 80-130 | `LoadedContentState.h` | HIGH | Needs loading/resource ownership plan first. |
| resource-owned state | portraits/icons/faces/system screens/stage background | 60-100 | `RenderResources.h` | HIGH | Needs texture lifetime module first. |
| audio state | `audio` | 20-50 | `AudioState.h` | HIGH | Split only with audio lifecycle functions. |
| verification/harness state | frame counters and external `gFightInputOverride` bridge | 20-50 | `VerificationState.h` later | MEDIUM | Possible later, but bridge depends on internals. |
| routing/pending action state | screen, pending mode, prepared/load-failed flags, selected result/pause routes | 40-80 | `FrontendState.h` / route-action boundary | MEDIUM | Yes, if route actions remain explicit and tested. |

Answer: `AppState` should be split before moving. Moving it as one type would drag runtime, resources, routing, audio, loaded content, and verification concerns into a public header.

## FighterState Dependency Map

`FighterState` is the core runtime actor state. It should stay in `App.cpp` for now.

| Group | Fields | Split Later? | Risk | Notes |
| --- | --- | --- | --- | --- |
| position/movement | `x`, `y`, velocities, facing, bounds, push, screen bound, freeze flags | Maybe | HIGH | Movement, camera, player push, and edge logic all use these. |
| animation | `stateNo`, `stateTime`, `action`, `animTick`, state/move/physics type, action flags | Maybe | HIGH | Connected to CNS state entry and animation lifetime. |
| input/command state | `inputHistory`, command flags, `ctrl`, move contact/hit/guarded | Maybe | HIGH | Command verifier depends on this path. |
| CNS/state machine state | fired controller ID vectors, variables, sysvars/fvars, previous state | Maybe | HIGH | Runtime controller execution depends on exact semantics. |
| hit/damage state | hitpause/stun/slide, get-hit vars, fall vars, life/power, multipliers, hit protection | No for now | HIGH | KFM verifier protects it, but blast radius is large. |
| targeting state | target index/hit id/ticks, bind fields, owner/helper links | No for now | HIGH | Target controllers and helper ownership depend on this. |
| helpers/projectiles/explods/effects | helper flags, owner/helper IDs, palette/trans/afterimage, projectile contact fields | No for now | HIGH | Compatibility characters depend on helpers/projectiles/effects. |
| loaded character data references | state/action numbers, palette/remap state, victory quote | Maybe | MEDIUM | Could become sub-structs after runtime type audit. |
| AI/control flags | `ctrl`, helper/destroy flags, common-state flags | No for now | HIGH | State transitions and CPU/dummy update rely on them. |
| training/dummy interactions | guard/crouch flags, hit status, training dummy behavior checks | No for now | MEDIUM | Tied to Training Options and KFM smoke. |

Answer: `FighterState` should stay in `App.cpp` for now. Small sub-structs may be extracted later, but the whole type is runtime-heavy and should not move until hit/damage, command/CNS, helper/projectile, and animation boundaries are clearer.

## Do Not Move Yet

These areas should remain in `App.cpp` or current runtime files until more boundaries exist:

- CMD/CNS runtime
- HitDef / damage / guard logic
- round flow / KO / time-over decisions
- helper / projectile / explod runtime
- SuperPause / Pause runtime behavior
- `FighterState` as a whole
- `AppState` as a whole, unless split first
- SDL/resource ownership
- character/stage runtime loading
- audio scheduling, unless separately audited

## Future Extraction Roadmap

### Phase A: Safe Type/Data Extraction

| Proposed Pass | Target | Estimated App.cpp Reduction | Risk | Preconditions | Test Focus |
| --- | --- | ---: | --- | --- | --- |
| Pass A1 | Completed Pass 14 `FrontendState` split for screen, mode, pause/result selected indices, menu rail, and `screenFrame` storage | 6 actual from App.cpp | MEDIUM | Routing and `screenFrame` behavior stayed in App.cpp | build/verifiers passed; GUI route smoke passed |
| Pass A2 | Completed Pass 13 `SelectionState` split for selected character/stage/session slots | 88 actual | MEDIUM | `MugenData` catalog types and `AppTypes.h` session slots already public | build/verifiers passed; GUI route smoke passed; cursor movement remains automation-blocked |
| Pass A3 | Completed Pass 17 shared UI render helper extraction | 188 actual | MEDIUM | Split primitives from sprite-dependent helpers to preserve include order | build/verifiers passed; title/select/fight GUI smoke passed |

### Phase B: Frontend/Menu Behavior

| Proposed Pass | Target | Estimated App.cpp Reduction | Risk | Preconditions | Test Focus |
| --- | --- | ---: | --- | --- | --- |
| Pass B1 | Completed Pass 15 transitional key-flow extraction for existing key dispatcher | 398 actual | MEDIUM | `FrontendState`, `SelectionState`, and `FrontendMenu` already extracted | build/verifiers passed; route/fight key GUI smoke passed; Options/selector movement remains automation-blocked |
| Pass B1a | Completed Pass 12 pure frontend decision boundary | 131 actual | MEDIUM | `AppTypes.h` public menu/settings types | build/verifiers passed; GUI route smoke passed; Options/cursor/normal GUI smoke remains automation-blocked |
| Pass B2 | Extract settings mutation and gamepad assignment display/cycle behavior | 180-260 | MEDIUM | Keep SDL gamepad open/close in App.cpp | Options screen, controller smoke, verifier regression |
| Pass B3 | Completed Pass 16 Training Options behavior extraction, leaving fight runtime untouched | 221 actual | MEDIUM | `TrainingOptions` and related enums already public in `AppTypes.h` | build/verifiers passed; F2 labels/status/toggle GUI smoke passed |

### Phase C: State Splitting

| Proposed Pass | Target | Estimated App.cpp Reduction | Risk | Preconditions | Test Focus |
| --- | --- | ---: | --- | --- | --- |
| Pass C1 | Completed Pass 19 `TrainingState` split for `TrainingOptions` storage only | +1 actual App.cpp line, 11 header lines | LOW | Training behavior, display messages, and runtime side effects stayed in App.cpp | build/verifiers passed; no manual GUI smoke claimed |
| Pass C1a | Completed Pass 20 `FightMessageState` split for shared hit/result message storage | 0 actual App.cpp line change, 12 header lines | LOW | Hit/damage, round-flow, HUD rendering, verifier behavior, and runtime systems stayed in App.cpp | build/verifiers passed; no manual GUI smoke claimed |
| Pass C2 | Completed Pass 21 `FightDisplayState` split for env-shake display storage and combo counters | 5 actual App.cpp line reduction, 19 header lines | MEDIUM | Env-shake trigger/update behavior, combo mutation, camera, global pause, match/round state, hit/damage, rendering, CPU behavior, and runtime systems stayed in App.cpp | build/verifiers passed; no manual GUI smoke claimed |
| Pass C3 | Split `MatchState` / `RoundState` from remaining match/runtime state | 50-100 | HIGH | Dedicated round/timer/result smoke should be added first | Single Player/VS timer, pause, result overlays |
| Pass C4 | Split `LoadedContentState` only after resource lifetime audit | 80-130 | HIGH | Loading/cache ownership plan | load-level audit, KFM/Evil Ken verifiers |

### Phase D: Real Module Conversion

See `docs/OVERLAY_MODULE_CONVERSION_AUDIT.md` before starting overlay conversion. It maps the current App.cpp-internal overlay headers, exact blockers, minimum seams, and recommends creating a public UI render/view seam before converting a specific overlay.

| Proposed Pass | Target | Estimated App.cpp Reduction | Risk | Preconditions | Test Focus |
| --- | --- | ---: | --- | --- | --- |
| Pass D1 | Convert small menu overlays to normal `.cpp` modules | 0 physical App.cpp lines, removes hidden coupling | LOW | Public frontend/render types | main/options/stage GUI smoke |
| Pass D2 | Convert fight HUD/result overlays to normal modules | 0 physical App.cpp lines, removes hidden coupling | MEDIUM | public fight display/read-only state | Training fight HUD, pause/result smoke |
| Pass D3 | Convert verification bridge to normal runner | 0-200 depending design | MEDIUM | AppState/FighterState access boundary | both scripted verifiers |

### Phase E: Runtime Systems Later

| Proposed Pass | Target | Estimated App.cpp Reduction | Risk | Preconditions | Test Focus |
| --- | --- | ---: | --- | --- | --- |
| Pass E1 | Actor/world rendering module | 500-800 | HIGH | Render/resource types split first | Training and VS fight GUI smoke |
| Pass E2 | Audio lifecycle and scheduling module | 300-450 | HIGH | Audio state lifetime plan | hit sound, pause/superpause audio, verifiers |
| Pass E3 | Character/stage runtime loading module | 700-1100 | HIGH | Loaded content/resource state split | load-level audit, KFM/Evil Ken verifiers |
| Pass E4 | State controller parser/types module | 1800-2600 | HIGH | Type-only parser boundary and tests | compatibility audit and verifiers |
| Pass E5 | Hit/damage/guard runtime module | 1000-1600 | HIGH | broader KFM guard/KO tests | KFM combat verifier, manual guard/KO |
| Pass E6 | CMD/CNS runtime module | 1200-2000 | HIGH | command parser/type boundary | KFM/Evil Ken command verifier |

## Line Count Summary

| Item | Estimate |
| --- | ---: |
| Current `App.cpp` count | 14490 after Pass 21 |
| Known extracted headers/modules | 24 extracted headers/modules, including normal `TrainingState.h` / `FightMessageState.h` / `FightDisplayState.h` and transitional `FrontendFlow.h` / UI render helper headers |
| Estimated safe immediate extraction total | 200-450 lines |
| Estimated medium-risk extraction total | 700-1400 lines |
| Estimated high-risk extraction total | 7000+ lines |

Top 10 largest remaining `App.cpp` responsibility clusters:

| Cluster | Rough Lines | Confidence | Risk |
| --- | ---: | --- | --- |
| CNS state/controller parsing | 1800-2600 | MEDIUM | HIGH |
| expression evaluation and runtime controller execution | 1800-2400 | LOW | HIGH |
| hit/damage/guard/get-hit runtime | 1000-1600 | MEDIUM | HIGH |
| character/stage/runtime asset loading | 700-1100 | MEDIUM | HIGH |
| world/fighter/effect/projectile rendering | 500-750 | MEDIUM | HIGH |
| command parsing and recognition | 500-800 | MEDIUM | HIGH |
| main event/key routing and front-end behavior | 500-650 | MEDIUM | MEDIUM |
| match/round flow and result behavior | 450-650 | MEDIUM | HIGH |
| audio scheduling/mixing | 300-450 | MEDIUM | HIGH |
| gamepad/settings/selection behavior | 200-350 | MEDIUM | MEDIUM |

## Next Pass Selection Rule

The next implementation pass should be chosen by this audit, not by the old roadmap.

Prefer a pass that:

1. removes a meaningful amount of `App.cpp`,
2. avoids CMD/CNS, hit/damage, round-flow, and resource ownership,
3. has clear verifier coverage,
4. can be smoke-tested manually,
5. helps convert transitional App.cpp-only headers into normal modules.

If two candidates are similar, prefer the one that unlocks more future modules.

Pass 12 completed the first scoped front-end/menu boundary by moving pure decision helpers into `FrontendMenu.h/.cpp`; `App.cpp` still performs SDL conversion, routing mutation, loading, and fight startup. The focused air-state sprint then fixed and verified held diagonal jump and air-attack landing regressions through `kfm-air-state`. Pass 13 split character/stage selection metadata into `SelectionState.h/.cpp` while leaving `MugenData`, runtime loading, preferred-stage mutation, routing, and fight startup unchanged. Pass 14 split UI/front-end state storage into `FrontendState.h` while leaving routing, transition behavior, `screenFrame` reset/increment behavior, loading, runtime, and CPU behavior in `App.cpp`. Pass 15 moved the existing key-flow dispatcher into transitional `FrontendFlow.h`; it is not a clean module yet because it still calls App.cpp-local helpers and depends on AppState. Pass 16 moved pure Training Options label/status/cycling/toggle helpers into `TrainingOptionsBehavior.h/.cpp` while leaving runtime side effects outside the module. Pass 17 moved shared UI render primitives and title/select presentation into transitional headers while leaving sprite helpers and resource ownership in App.cpp. Pass 18 added baseline CPU behavior through normal `FighterInputState` / CMD/CNS / HitDef routing without advanced AI. Pass 19 moved only `TrainingOptions` storage into normal `TrainingState.h`. Pass 20 moved only shared fight-message text/tick storage into normal `FightMessageState.h`; hit/damage, round-flow, HUD rendering, verifier behavior, and runtime systems remain in App.cpp. Pass 21 moved only env-shake display storage and combo counter storage into normal `FightDisplayState.h`; env-shake trigger/update behavior, combo mutation, camera, global pause, match/round state, hit/damage, rendering, CPU behavior, and runtime systems remain in App.cpp. Good next candidates are a settings state boundary, a `MatchState`/`RoundState` audit, or a follow-up action-return boundary that reduces `FrontendFlow.h`'s App.cpp-local helper dependency before normal module conversion.
