# Feature Ledger

This ledger is the preservation record for Dragon MUGEN. When engine/app code changes, update this file or `docs/REGRESSION_CHECKLIST.md` in the same commit so prior work does not disappear silently.

## Preservation Rule

- New features must not remove previous features without an explicit note here.
- If a feature is intentionally replaced, document the replacement and the reason.
- If a feature is incomplete, document its current working behavior and known gap.
- The active-change guard enforces documentation updates for engine/app code commits.

## Verification Status Model

Use these labels so preservation claims do not get confused with live proof:

- `[VERIFIED-LIVE]` - Manually verified in the running app during a recorded pass or supported by a clearly recorded recent live observation.
- `[VERIFIED-STATIC]` - Confirmed by code inspection, static audit, parser test, or load-level audit, but not manually played live.
- `[CLAIMED]` - Documented or believed to exist, but not recently verified.
- `[PARTIAL]` - Works in a limited case, but not complete.
- `[REGRESSED]` - Previously worked but is currently broken.
- `[BLOCKED]` - Cannot verify yet because another system is missing, broken, inaccessible, or the verification environment cannot exercise it.
- `[NOT-STARTED]` - Planned but not implemented.

Static audits do not count as live verification. Parsed M.U.G.E.N controllers remain `[CLAIMED]` or `[VERIFIED-STATIC]` until exercised in the running game.

## Current Runtime Features To Preserve

| Status | Area | Current Behavior | Preserve Until |
| --- | --- | --- | --- |
| `[VERIFIED-STATIC]` | M.U.G.E.N folder layout | Runtime content lives under `game/chars`, `game/data`, `game/font`, `game/sound`, `game/stages`, `game/plugins`, and `game/save`. | A deliberate roadmap update changes the content model. |
| `[VERIFIED-STATIC]` | M.U.G.E.N customization preservation policy | M.U.G.E.N customization preservation policy is documented in `docs/REPOSITORY_POLICY.md`, `docs/DRAGON_EXTENSIONS.md`, and `docs/STRICT_ROADMAP.md`. This means the policy is repo-visible; it does not mean full runtime enforcement has been audited. Runtime support may still be partial. The policy preserves editability and source-of-truth ownership, not instant perfect compatibility with every M.U.G.E.N character. | Always, for M.U.G.E.N-style customization. |
| `[VERIFIED-STATIC]` | Roster loading | Characters and stages are selected through `game/data/select.def`; character folders alone do not make a character selectable. | Always, for M.U.G.E.N compatibility. |
| `[VERIFIED-STATIC]` | Character file resolution | Selected character DEF `[Files]` resolves CMD, CNS/ST, `stcommon`, AIR, SFF, SND, and ACT files. | Always, for compatibility-era loading. |
| `[PARTIAL]` | Select-screen loading | Character select uses portraits/icons only and must not load full fight runtime data before the VS path. Live pass observed select portraits/icons; full memory behavior remains static-only. | A documented memory-model change replaces it. |
| `[VERIFIED-LIVE]` | VS loading path | Stage confirmation enters VS first; selected character and stage runtime loading happens on the VS-to-fight path. Live pass observed KFM/Mountainside Temple stage select, VS screen, and fight view load. | A documented flow change replaces it. |
| `[PARTIAL]` | Modes | Training, Single Player, and VS Mode are distinct mode paths, not one menu with hidden options. Live pass verified Training path only; Single Player and VS paths still need live navigation. | A documented mode-system change replaces it. |
| `[PARTIAL]` | Training tools | Training has dummy options, hitbox/debug toggles, command HUD/input HUD, move list, power option, guard damage, reset, and P2 control. Live pass verified dummy presence, F1 hitboxes, F2 options, command HUD, and input HUD; remaining options need live verification. | Training feature spec is replaced. |
| `[CLAIMED]` | Single fight rules | Single Fight uses round flow, timer, pips, KO/time-over/draw handling, match result, rematch/menu routing, and local P2 controls. | Fight rules feature spec is replaced. |
| `[CLAIMED]` | Controller input | Keyboard and SDL3 gamepad mappings feed the same M.U.G.E.N command/input path. | Input runtime feature spec is replaced. |
| `[VERIFIED-STATIC]` | SFF/ACT/AIR/SND decode | Current runtime loads SFF v1 PCX sprites, ACT palettes, AIR frames/collision boxes, and SND samples for the active compatibility roster. | Decoder feature spec is replaced. |
| `[VERIFIED-STATIC]` | CMD routing | Commands feed a buffered command system and State -1 routing rather than hardcoded character buttons. | Command runtime feature spec is replaced. |
| `[VERIFIED-STATIC]` | CNS subset | Current runtime supports the documented controller/trigger subset in `docs/COMPATIBILITY_AUDIT.md`. | CNS runtime feature spec is replaced. |
| `[CLAIMED]` | Hit/guard/fall basics | HitDef/projectile contact, guard, fall/recovery mapping, hit sparks, hit sounds, hit counters, and powerbar basics exist as first-pass behavior. | Hit runtime feature spec is replaced. |
| `[VERIFIED-STATIC]` | Compatibility roster | KFM, Evil Ryu, and Evil Ken are the current local compatibility set and must keep load-level checks passing. | The active roster changes in `select.def` and compatibility docs. |
| `[VERIFIED-STATIC]` | Benchmark chars | `DragonBench`, `A.Ben`, and `I.Chie` are reserved original benchmark character folders and are not selectable until real M.U.G.E.N files exist. | They become real documented content. |

## Current Non-Goals

- Do not hardcode individual downloaded characters.
- Do not replace M.U.G.E.N backend files with Dragon-only sidecars.
- Do not introduce `.dragon/*.json` runtime sidecars or migrate `.dragon.def` sidecars to JSON.
- Do not add shop, equipment, tournament, storyboards, networking, or editor work before the compatibility play loop is stable.
