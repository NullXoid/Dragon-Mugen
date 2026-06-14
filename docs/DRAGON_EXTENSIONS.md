# Dragon Extensions

This document is the registry for every Dragon MUGEN feature that is not plain M.U.G.E.N behavior.

The engine must keep the M.U.G.E.N backend structure. Characters, stages, common effects, common states, fonts, sounds, and storyboards still live where a M.U.G.E.N creator expects them. Dragon-only features may extend that structure, but they must not hide or replace it.

The canonical customization policy is [REPOSITORY_POLICY.md](REPOSITORY_POLICY.md). M.U.G.E.N files are creator-owned source of truth for M.U.G.E.N-style content. Dragon sidecars extend behavior; they must not replace `.def`, `.cmd`, `.cns`, `.air`, `.sff`, `.snd`, `.act`, `select.def`, `fight.def`, `system.def`, or motif files where supported.

## Core Rule

Use M.U.G.E.N files first.

If a feature can be represented by normal M.U.G.E.N content, it belongs in the normal M.U.G.E.N file:

- Character behavior: `game/chars/<character>/<character>.cns`
- Character commands: `game/chars/<character>/<character>.cmd`
- Character animation/collision: `game/chars/<character>/<character>.air`
- Character sprites: `game/chars/<character>/<character>.sff`
- Character sounds: `game/chars/<character>/<character>.snd`
- Stage camera/player starts/backgrounds: `game/stages/<stage>.def`
- Stage sprites: `game/stages/<stage>.sff`
- Common states: `game/data/common1.cns`
- Fight effects: `game/data/fightfx.air` and `game/data/fightfx.sff`
- Fight UI: `game/data/fight.def`
- System/title/select UI: `game/data/system.def`
- Select list: `game/data/select.def`
- Global M.U.G.E.N options: `game/data/mugen.cfg`

If a feature cannot be represented by normal M.U.G.E.N content, it is a Dragon extension and must be documented in this file before or during implementation.

## Extension File Contract

Dragon extension files use M.U.G.E.N-style section/key text syntax so creators do not have to learn a second data format for simple options.

Dragon extension files are sidecars. They sit beside the M.U.G.E.N files they extend.

```text
game/
  data/
    mugen.cfg
    system.def
    fight.def
    dragon.def              # Dragon global defaults, not M.U.G.E.N
  chars/
    kfm/
      kfm.def
      kfm.air
      kfm.cmd
      kfm.cns
      kfm.sff
      kfm.snd
      kfm.dragon.def        # Dragon character extras, not M.U.G.E.N
  stages/
    kfm.def
    kfm.sff
    kfm.dragon.def          # Dragon stage extras, not M.U.G.E.N
  save/
    settings.def            # local user settings, not creator content
```

Rules:

- Do not require Dragon sidecars for M.U.G.E.N compatibility.
- Classic Mode must not require `.dragon.def`.
- Dragon Mode may use `.dragon.def` later, but if the sidecar is missing it must use safe defaults or mark Dragon enhancements unavailable.
- Do not move M.U.G.E.N data out of M.U.G.E.N files just because Dragon can read a sidecar.
- Do not write local player settings into character, stage, or data definition files.
- Use `game/save/settings.def` for local settings and profiles.
- Use `game/data/dragon.def` for game-wide Dragon defaults that ship with a project.
- Use `game/chars/<character>/<character>.dragon.def` for optional character metadata that M.U.G.E.N does not understand.
- Use `game/stages/<stage>.dragon.def` for optional stage metadata that M.U.G.E.N does not understand.
- The current sidecar convention is M.U.G.E.N-style `.dragon.def`, not `.dragon/*.json`.
- Do not introduce `.dragon/*.json` runtime sidecars or migrate character/stage sidecars to JSON.
- Prefix extension sections with `Dragon` when practical, for example `[Dragon.Training]` or `[Dragon.StagePreview]`.
- Every new extension must document: file, section, keys, default behavior, whether it is current or planned, and whether it affects M.U.G.E.N compatibility.

## Compatibility Facade Contract

Dragon is the default runtime shell, but loaded character and stage behavior must pass through the compatibility facade before new engine-only behavior is applied.

Current facade metadata:

- `CompatibilityProfile`: `Mugen2001`, `Mugen10`, `Mugen11`, or `Dragon`.
- `DragonRuntimeMode`: `Dragon` or `ClassicMugen`; the app currently defaults to `Dragon`.
- `localcoord`: parsed from character DEF `[Info] localcoord`, defaulting to `320,240`.
- Sidecar availability: `game/data/dragon.def`, character `.dragon.def`, and stage `.dragon.def`.
- Legacy Arena stage extension bridge: existing `[OpenBOR]` and `[DragonOpenBOR]` stage sections remain accepted, but are treated as Dragon extension metadata.

Migration rule:

- New behavior fixes for throws, helpers, hit/fall, stun, screen bounds, jump leniency, power charge helpers, Arena depth, and command training must route compatibility-sensitive decisions through the facade.
- Do not hardcode new mode checks when a facade helper can represent the policy.
- Do not edit character or stage M.U.G.E.N files to make Dragon-specific behavior work.

## Editing Stage Backgrounds

Stage background images are not a Dragon extension. They are normal M.U.G.E.N stage content.

To change a stage background:

1. Edit the stage `.def` in `game/stages/`.
2. Put the referenced image sprites in the stage `.sff` in `game/stages/`.
3. Keep the `.def` and `.sff` together.

Typical stage files:

```text
game/stages/
  kfm.def
  kfm.sff
```

In the stage `.def`, `[BGDef]` chooses the sprite archive:

```ini
[BGDef]
spr = kfm.sff
```

Each `[BG ...]` block places a background element:

```ini
[BG mountains]
type = normal
spriteno = 0,0
start = 0,0
delta = 0.5,0.5
tile = 1,0
mask = 1
layerno = 0
```

Common keys:

- `type`: background type. Current engine support is basic `normal`; animated/parallax-specific behavior is still planned.
- `spriteno`: sprite group and image from the stage SFF.
- `start`: screen/world offset for the background element.
- `delta`: camera-follow multiplier. Smaller values move slower and feel farther away.
- `tile`: repeat in X/Y.
- `mask`: use transparency.
- `layerno`: `0` draws behind fighters, `1` draws in front.

Current engine status:

- Reads `[BGDef] spr`.
- Reads `[BG ...] spriteno`, `start`, `delta`, `tile`, `mask`, and `layerno`.
- Applies camera movement to `delta`.
- Handles basic tiling and foreground/background ordering.
- Still needs a fuller pass for animated backgrounds, parallax-specific stage syntax, advanced transparency, and edge cases.

If Dragon later adds stage options that M.U.G.E.N does not have, they go in `game/stages/<stage>.dragon.def`, not in a replacement stage format.

Example planned stage sidecar:

```ini
[Dragon.StagePreview]
image = previews/kfm.png

[Dragon.TrainingCamera]
lock = 0
defaultZoom = 1.0
```

## Editing Settings

Settings have three categories.

### M.U.G.E.N-Compatible Project Settings

Use M.U.G.E.N files:

- `game/data/mugen.cfg`: video, audio, input, and base engine options.
- `game/data/system.def`: title, menus, select screen, and system visuals.
- `game/data/fight.def`: lifebars, round text, power bars, timer, and fight UI.
- `game/data/select.def`: which characters and stages appear in select.

Current engine status:

- The prototype reads only a subset of these files. Current implemented subsets include `select.def` roster/stage entries, `fight.def` round/combo/powerbar settings, `system.sff` motif/title/select sprites, `fightfx.air`/`fightfx.sff`, and selected character/stage definitions.
- The folder contract still reserves these files as the correct future backend.
- Do not invent a different system layout for things these files already cover.

### Dragon Project Defaults

Use `game/data/dragon.def` for project defaults that are not M.U.G.E.N.

Planned examples:

```ini
[Dragon.Training]
showHitboxes = 0
showOrigins = 0
showFloorLine = 0
showReadout = 0
dummyGuard = off
guardDamage = 1

[Dragon.CommandTraining]
enabled = 1
showOnlyLoadedStates = 1
```

These are project defaults. They are not per-player save data.

### Local User Settings

Use `game/save/settings.def` for local player preferences.

Planned examples:

```ini
[Video]
windowWidth = 960
windowHeight = 540
fullscreen = 0

[Input.Keyboard.P1]
x = A
y = S
z = D
a = Z
b = X
c = C
up = Up
down = Down
left = Left
right = Right

[Input.Gamepad.P1]
assignment = auto
labels = auto
```

Do not commit personal save/settings files as required content unless they are sample defaults.

## Current Dragon-Only Feature Registry

These features are currently in the prototype and are not plain M.U.G.E.N behavior.

| Feature | Current Status | Where It Lives Now | Future Data Location | Compatibility Rule |
| --- | --- | --- | --- | --- |
| SDL3 training/options overlay | Implemented in app code | Runtime only | `game/data/dragon.def` defaults, `game/save/settings.def` user prefs | Must not replace M.U.G.E.N `system.def`/`fight.def` once those are parsed |
| `F1` quick hitbox toggle | Implemented in app code | Runtime only | `game/save/settings.def` | Debug convenience only |
| `F2` training options menu | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Dragon training tool, not character content |
| Reset fight with `R` | Implemented in app code | Runtime only | `game/save/settings.def` input binding later | Must reset from M.U.G.E.N stage starts |
| Hitbox/origin/floor-line/readout toggles | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Debug display must reflect AIR/stage data |
| Hit flash toggle | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Visual feedback only; does not change hit logic |
| Hit spark toggle | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Sparks still come from `game/data/fightfx.*` |
| Hit log toggle | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Debug display only |
| Hit sound toggle | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Sounds still resolve from character/common/fight SND files |
| Dummy invincibility | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Training-only; should not alter character CNS |
| Dummy auto-life | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Training-only |
| Dummy freeze | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Training-only |
| Dummy guard mode | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Uses M.U.G.E.N guard states/actions where possible |
| Guard damage toggle | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Training-only; parsed guard damage remains from HitDef |
| P2-controlled training | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Training-only option that switches the active opponent from `Dummy` to `LocalP2`; dummy-only options stop applying while enabled |
| Training command/input HUD | Implemented in app code | Runtime only, derived from selected character CMD and live input buffer | `game/data/dragon.def`, `game/save/settings.def` | Flying Dragon-inspired training display; commands remain sourced from `.cmd`/`.cns`, and player inputs still map to M.U.G.E.N command names |
| Training power mode | Implemented in app code | Runtime only, uses selected character `[Data] power` | `game/data/dragon.def`, `game/save/settings.def` | Training-only testing aid; `MAX` keeps both meters full without editing character CNS |
| Training move category filter | Implemented in app code | Derived from selected character CMD `State -1` entries | `game/data/dragon.def`, `game/save/settings.def` | Filters command-training page by all, normal, special, or super; move existence stays in CMD/CNS |
| Position reset from options | Implemented in app code | Runtime only | `game/save/settings.def` input binding later | Must reset from M.U.G.E.N `[PlayerInfo]` starts |
| Command-training move list | Implemented in app code | Derived from selected character CMD | Optional `game/chars/<character>/<character>.dragon.def` presentation overrides later | Base move list must remain derived from CMD `State -1` entries |
| Flying-Dragon-inspired mode rail behavior | Partly implemented as layout only | Runtime UI | `game/data/system.def` plus Dragon sidecar if needed | Up/Down selection must not switch rail side |
| Single Player mode | Implemented as first pass in app code | Runtime UI/rules | `game/data/dragon.def` or future campaign files | Uses P1 roster selection plus a mode-owned `CPU` opponent slot; CPU AI and separate opponent runtime are not implemented yet |
| Single Fight pause menu | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Distinct from Training options; must not replace future M.U.G.E.N `system.def`/`fight.def` parsing |
| Single Fight local P2 keyboard controls | Implemented in app code | Runtime only | `game/save/settings.def` | Keyboard labels map to M.U.G.E.N commands; character moves still come from CMD/CNS |
| SDL3 Xbox/PlayStation gamepad controls | Implemented in app code | Runtime only | `game/save/settings.def` | Gamepad buttons map to M.U.G.E.N command names; character moves still come from CMD/CNS |
| Single Fight timer and round rules | Implemented with `FightData` parsing plus app runtime | Parsed `game/data/fight.def` `[Round]` keys | `game/data/fight.def` where M.U.G.E.N-compatible, Dragon sidecar only for non-M.U.G.E.N extras | `match.wins`, round-start, KO/DKO/Time-Over, `over.waittime`, hit-grace, `over.wintime`, win-message, and round-over timing come from M.U.G.E.N data |
| Combo hit counter | Implemented with `FightData` parsing plus app runtime | Parsed `game/data/fight.def` `[Combo]` keys | `game/data/fight.def` | Counts unguarded hit chains and uses parsed `pos`, `start.x`, `counter.font` palette, `counter.shake`, `text.text`, `text.font` palette, `text.offset`, and `displaytime` |
| First-pass power gauge rectangles | Implemented with `FightData` parsing plus app runtime | Parsed `game/data/fight.def` `[Powerbar]` keys | `game/data/fight.def` plus future `fight.sff` rendering | Data comes from M.U.G.E.N `fight.def`; rectangle drawing is temporary until the sprite-based fight UI renderer lands |
| Flying-Dragon-style match result menu | Implemented in app code | Runtime UI after M.U.G.E.N-compatible round scoring | `game/data/dragon.def`, `game/save/settings.def`, future tournament/shop/equipment files | Dragon-only post-match decision layer; must not change character, stage, or `fight.def` compatibility |
| Main Settings screen | Implemented in app code | Runtime only | `game/data/dragon.def`, `game/save/settings.def` | Dragon project/user settings UI; M.U.G.E.N backend files stay authoritative |
| Local Reu Evil Ryu/Evil Ken compatibility entries | Copied into `game/chars` and listed in `game/data/select.def` for local tests | M.U.G.E.N character content, not a Dragon feature | Not applicable | Used only to audit compatibility; public builds must remove or replace unlicensed third-party content |

## Planned Dragon-Only Feature Registry

These are expected future extensions and must stay behind the M.U.G.E.N backend.

| Feature | Planned Location | Compatibility Rule |
| --- | --- | --- |
| Command-training categories/descriptions | `game/chars/<character>/<character>.dragon.def` | Commands and target states still come from `.cmd`/`.cns` |
| Training default settings | `game/data/dragon.def` | Local overrides go to `game/save/settings.def` |
| Controller/key bindings | `game/save/settings.def` | M.U.G.E.N command names remain `x/y/z/a/b/c/...` |
| Stage preview images | `game/stages/<stage>.dragon.def` or a path referenced from it | The actual playable stage remains `.def`/`.sff` |
| Title/story video support | `game/data/dragon.def` or storyboard sidecar | M.U.G.E.N image storyboard support comes first |
| RPG level/shop/equipment data | `game/data/dragon.def`, `game/chars/<character>/<character>.dragon.def`, and `game/save/` | Character combat behavior still comes from `.cmd`/`.cns` |
| Tournament/campaign definitions | `game/data/dragon.def` or dedicated M.U.G.E.N-style data files under `game/data/` | Character/stage references must use normal `chars/` and `stages/` IDs |
| Plugin configuration | `game/plugins/` and `game/data/dragon.def` | Optional only; core runtime must not depend on plugins for KFM compatibility |

## Character Move Lists

The current move-list screen is Dragon-only UI, but the data source is M.U.G.E.N-compatible.

Current source:

- Reads selected character `*.cmd`.
- Uses `State -1` `ChangeState` blocks.
- Displays each block label, such as `Stand Light Punch`.
- Displays target state, such as `200`.
- Filters out entries whose CNS state or AIR action is not currently loaded.

Current input display mapping:

- M.U.G.E.N `x` -> keyboard `A`
- M.U.G.E.N `y` -> keyboard `S`
- M.U.G.E.N `z` -> keyboard `D`
- M.U.G.E.N `a` -> keyboard `Z`
- M.U.G.E.N `b` -> keyboard `X`
- M.U.G.E.N `c` -> keyboard `C`
- `holddown` -> `DOWN`
- `holdfwd` -> `FWD`
- `holdback` -> `BACK`
- `holdup` -> `UP`

Future character sidecar example:

```ini
[Dragon.CommandTraining]
showOnlyLoadedStates = 1

[Dragon.Move.200]
category = Standing
display = Light Punch
notes = Fast jab.

[Dragon.Move.400]
category = Crouching
display = Crouch Light Punch
notes = Low-profile jab.
```

This sidecar can add presentation data, but it must not become the authority for whether a move exists. The move exists because the character CMD/CNS/AIR/SFF/SND define it.

## Match Mode Menus And Controls

Single Player and Single Fight use a distinct pause menu, not the Training options menu.

Current match pause actions:

- `RESUME`
- `RESTART MATCH`
- `FIGHTER SELECT`
- `STAGE SELECT`
- `MODE SELECT`

Current match rules:

- Local P1 and P2 can both control the selected character.
- Hit detection runs P1-to-P2 and P2-to-P1.
- Match format uses `game/data/fight.def` `[Round] match.wins`; the current default is 2 round wins.
- Timer defaults to 99 seconds and is changed from main menu `OPTIONS`.
- KO, double KO, or timeout ends the current round through the `RoundFinish` and `RoundResult` phases.
- `RoundFinish` keeps the fight scene active until parsed `over.time`; it uses parsed `over.hittime` for double-KO grace, parsed `over.waittime` to stop control, and parsed `over.wintime` before applying available selected-character win/lose/draw poses.
- Tie rounds display as a tie and replay without awarding either player a round win.
- The match ends when P1 or P2 reaches the parsed `match.wins` target.
- A M.U.G.E.N-style callout, winner/draw text, and scored pip display appear between rounds.
- Match completion advances to the Dragon-only `MatchResult` decision menu.

Current keyboard mapping:

```text
P1:
  movement: Arrow keys
  x/y/z: A / S / D
  a/b/c: Z / X / C

P2:
  movement: J / L / I / K
  x/y/z: U / O / P
  a/b/c: N / M / ,
```

Current gamepad mapping uses SDL3's standard gamepad layout:

```text
Xbox:
  movement: D-pad or left stick
  x/y/z: X / Y / LB
  a/b/c: A / B / RB

PlayStation:
  movement: D-pad or left stick
  x/y/z: Square / Triangle / L1
  a/b/c: Cross / Circle / R1
```

Compatibility rule:

- Keyboard bindings are local runtime settings and will move to `game/save/settings.def`.
- Gamepad assignment and prompt label style are local runtime settings and will move to `game/save/settings.def`.
- M.U.G.E.N command names stay `x`, `y`, `z`, `a`, `b`, `c`, `holdfwd`, `holdback`, `holdup`, and `holddown`.
- Character move existence stays in `.cmd`, `.cns`, `.air`, `.sff`, and `.snd`.
- Training-only dummy settings must not affect match modes.
- Future M.U.G.E.N-compatible fight UI and timer configuration should come from `game/data/fight.def` where possible.

## Main Settings

The current main menu `OPTIONS` screen is Dragon-only and intentionally separate from Training options.

Current options:

- `MATCH TIMER`: cycles through `30`, `60`, `90`, `99`, `120`, and `180` seconds.
- `CANVAS SIZE`: cycles through `320x240 CLASSIC`, `426x240 WIDE`, and `480x240 EXTRA`.
- `UI SCALE`: cycles through `60%`, `70%`, `80%`, `90%`, and `100%` for Dragon floating panel and overlay density.
- `PAD LABELS`: cycles through `AUTO`, `XBOX`, and `PLAYSTATION` display labels.
- `P1 GAMEPAD`: cycles through automatic assignment, off, and connected gamepad slots.
- `P2 GAMEPAD`: cycles through automatic assignment, off, and connected gamepad slots.

Current behavior:

- The selected timer applies to Single Player and Single Fight rounds.
- Canvas size changes the SDL logical presentation and fight camera width only. It does not move character, stage, fight, or system files out of the M.U.G.E.N-style backend folders.
- UI scale changes Dragon-owned floating panels and callouts, including Main Settings content, Training options, the command-training page, command/input HUD, match pause, and round callout/result popups.
- Training is not affected by the match timer.
- Best 2 out of 3 remains fixed in code for now.
- Gamepad assignment layers on top of keyboard input and feeds the same M.U.G.E.N command names.

Planned data location:

```ini
[Dragon.Match]
timer = 99
roundsToWin = 2
tieRounds = replay

[Input.Gamepad]
labels = auto
p1 = auto
p2 = auto

[Video]
canvas = 426x240
ui.scale = 80
```

Compatibility rule:

- Where M.U.G.E.N has an equivalent fight/system setting, parse that first.
- Dragon settings only cover behavior that M.U.G.E.N does not define or that belongs to local user preferences.

## Character Compatibility Archives

Research character archives live outside `game/` as source copies. For local compatibility tests, they may also be copied into `game/chars/` and activated through `game/data/select.def`.

Current research archive:

```text
content/research_mugen_chars/reu/
  EvilRyu.zip
  EvilKen.zip
  EvilRyu/
  EvilKen/
  README.md
```

These Reu characters are useful for compatibility audits because they are more complex than KFM. They include large CNS files, separate command CNS files, SFF/SND archives, palettes, and storyboard content.

Current local runtime copies:

```text
game/chars/EvilRyu/
game/chars/EvilKen/
```

Current active roster entries:

```ini
EvilRyu, stages/kfm.def
EvilKen, stages/kfm.def
```

Observed local load result:

- Evil Ryu loaded through the selected-character runtime path with 335 rendered AIR actions, 382 parsed states, 189 parsed HitDefs, 64 parsed command-state entries, and 79 decoded character sounds.
- Evil Ken loaded through the selected-character runtime path with 465 rendered AIR actions, 429 parsed states, 179 parsed HitDefs, 63 parsed command-state entries, and 87 decoded character sounds.

The detailed compatibility audit lives in [COMPATIBILITY_AUDIT.md](COMPATIBILITY_AUDIT.md).

Compatibility rule:

- Do not ship these in public builds unless the project has the rights to do so.
- Use them only to inspect required parser/runtime features.
- Keep activation through `game/data/select.def`; do not make every folder under `game/chars` selectable automatically.
- Any compatibility work discovered from them must still be implemented through M.U.G.E.N-compatible parsing where possible.

## Storyboards, Title Images, And Video

M.U.G.E.N image storyboards belong in their normal locations first:

- Global title/system visuals: `game/data/system.def` plus referenced SFF/font/sound assets.
- Character intro: `game/chars/<character>/intro.def` and `intro.sff`.
- Character ending: `game/chars/<character>/ending.def` and `ending.sff`.

Dragon video support is planned but not current. If added, it must be optional and documented as a Dragon extension. It should not replace M.U.G.E.N storyboard image support.

Planned example:

```ini
[Dragon.StoryboardVideo]
intro = videos/opening.webm
fallback = intro.def
```

## Documentation Requirement For New Features

Every new feature must update documentation at the same time as code.

Required updates:

1. `docs/DRAGON_EXTENSIONS.md` if the feature is not plain M.U.G.E.N.
2. `docs/STRICT_ROADMAP.md` if the feature changes scope, milestones, folder rules, or compatibility strategy.
3. The relevant folder README under `game/` if creators need to know where files belong.
4. `docs/MEMORY_MODEL.md` if the feature changes loading, caching, ownership, or asset lifetime.

For each Dragon extension, document:

- Is it current or planned?
- Which file owns it?
- Which section/key names does it use?
- What are the defaults?
- Is it project content or local user data?
- Does it alter M.U.G.E.N compatibility?
- Which M.U.G.E.N file remains authoritative?

If the answer is unclear, do not add the feature yet.
