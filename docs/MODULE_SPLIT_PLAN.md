# Module Split Plan

`engine/src/App.cpp` is currently the recovery blocker. The app layer grew into a mixed UI, runtime, parser, renderer, audio, and fight-state file. That is why feature work started stepping on itself.

The architecture guard freezes `App.cpp` at 16,771 lines. New gameplay work should wait until the active feature is split into owned modules.

## Target Ownership

| Area | Intended Owner | Notes |
| --- | --- | --- |
| Roster/stage/file resolution | `MugenData` | Already started. Keep `select.def`, character DEF `[Files]`, chars, and stages here. |
| Fight DEF settings | `FightData` | Already started. Expand for lifebars, powerbars, combo, round, and fight UI data. |
| Screen/mode flow | `ScreenFlow` | Mode select, character select, stage select, VS, fight, result, and pause routing. |
| Motif/screenpack drawing | `MotifRenderer` | `system.def`, `system.sff`, title/select/menu visuals, storyboard hooks later. |
| Fight session and round flow | `FightSession`, `RoundFlow` | Round start/fight/finish/result/match result state machine and score/timer ownership. |
| Fighter runtime | `FighterRuntime` | Fighter position, velocity, facing, life, power, state, animation, common state routing. |
| CNS controller runtime | `CnsRuntime` | Controller execution, trigger/expression evaluation, variables, target/helper redirection. |
| Input command buffering | `CommandRuntime` | SDL input to M.U.G.E.N button/direction buffer and CMD command matching. |
| Hit and guard resolution | `HitRuntime` | Clsn checks, HitDef/projectile contact, guard, push, hit pause, GetHitVar snapshots. |
| Projectiles/helpers/effects | `ActorRuntime`, `EffectRuntime` | Helpers, projectiles, explods, palfx, afterimage, pause/superpause interactions. |
| Audio controller playback | `AudioRuntime` | SND lookup, PlaySnd/StopSnd order, channels, loops, voice caps, fight/common sounds. |
| Stage and camera rendering | `StageRuntime` | Stage DEF BG layers, camera bounds, zoffset, parallax/animation rules. |
| Training tools | `TrainingRuntime` | Training-only options, command HUD, input display, dummy behavior, move list. |
| Persistent settings | `SettingsData` | Future `game/save/settings.def`; keep runtime settings separate from M.U.G.E.N files. |

## Extraction Order

1. Freeze process and feature specs.
2. Extract screen/mode flow without changing behavior.
3. Extract fight session and round flow without changing behavior.
4. Extract command buffering and CMD matching.
5. Extract fighter runtime and CNS controller execution.
6. Extract hit/guard/projectile/effect runtime.
7. Extract training-only UI/tools.
8. Update `docs/FEATURE_LEDGER.md` and `docs/REGRESSION_CHECKLIST.md` after every meaningful extraction so preserved behavior remains explicit.

## Done Means

The architecture recovery feature is complete when:

- `App.cpp` is below 9,000 lines.
- M.U.G.E.N data ownership remains in data/runtime modules.
- Training, Single Player, VS, stage select, VS screen, fight view, and match result still load through `game/data/select.def`.
- KFM, Evil Ryu, and Evil Ken still pass load-level compatibility audit.
- The next gameplay feature can be implemented without adding new systems to `App.cpp`.
