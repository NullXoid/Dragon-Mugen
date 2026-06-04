# Feature Ledger

This ledger is the preservation record for Dragon MUGEN. When engine/app code changes, update this file or `docs/REGRESSION_CHECKLIST.md` in the same commit so prior work does not disappear silently.

## Preservation Rule

- New features must not remove previous features without an explicit note here.
- If a feature is intentionally replaced, document the replacement and the reason.
- If a feature is incomplete, document its current working behavior and known gap.
- The active-change guard enforces documentation updates for engine/app code commits.

## Current Runtime Features To Preserve

| Area | Current Behavior | Preserve Until |
| --- | --- | --- |
| M.U.G.E.N folder layout | Runtime content lives under `game/chars`, `game/data`, `game/font`, `game/sound`, `game/stages`, `game/plugins`, and `game/save`. | A deliberate roadmap update changes the content model. |
| Roster loading | Characters and stages are selected through `game/data/select.def`; character folders alone do not make a character selectable. | Always, for M.U.G.E.N compatibility. |
| Character file resolution | Selected character DEF `[Files]` resolves CMD, CNS/ST, `stcommon`, AIR, SFF, SND, and ACT files. | Always, for compatibility-era loading. |
| Select-screen loading | Character select uses portraits/icons only and must not load full fight runtime data before the VS path. | A documented memory-model change replaces it. |
| VS loading path | Stage confirmation enters VS first; selected character and stage runtime loading happens on the VS-to-fight path. | A documented flow change replaces it. |
| Modes | Training, Single Player, and VS Mode are distinct mode paths, not one menu with hidden options. | A documented mode-system change replaces it. |
| Training tools | Training has dummy options, hitbox/debug toggles, command HUD/input HUD, move list, power option, guard damage, reset, and P2 control. | Training feature spec is replaced. |
| Single fight rules | Single Fight uses round flow, timer, pips, KO/time-over/draw handling, match result, rematch/menu routing, and local P2 controls. | Fight rules feature spec is replaced. |
| Controller input | Keyboard and SDL3 gamepad mappings feed the same M.U.G.E.N command/input path. | Input runtime feature spec is replaced. |
| SFF/ACT/AIR/SND decode | Current runtime loads SFF v1 PCX sprites, ACT palettes, AIR frames/collision boxes, and SND samples for the active compatibility roster. | Decoder feature spec is replaced. |
| CMD routing | Commands feed a buffered command system and State -1 routing rather than hardcoded character buttons. | Command runtime feature spec is replaced. |
| CNS subset | Current runtime supports the documented controller/trigger subset in `docs/COMPATIBILITY_AUDIT.md`. | CNS runtime feature spec is replaced. |
| Hit/guard/fall basics | HitDef/projectile contact, guard, fall/recovery mapping, hit sparks, hit sounds, hit counters, and powerbar basics exist as first-pass behavior. | Hit runtime feature spec is replaced. |
| Compatibility roster | KFM, Evil Ryu, and Evil Ken are the current local compatibility set and must keep load-level checks passing. | The active roster changes in `select.def` and compatibility docs. |
| Benchmark chars | `DragonBench`, `A.Ben`, and `I.Chie` are reserved original benchmark character folders and are not selectable until real M.U.G.E.N files exist. | They become real documented content. |

## Current Non-Goals

- Do not hardcode individual downloaded characters.
- Do not replace M.U.G.E.N backend files with Dragon-only sidecars.
- Do not add shop, equipment, tournament, storyboards, networking, or editor work before the compatibility play loop is stable.
