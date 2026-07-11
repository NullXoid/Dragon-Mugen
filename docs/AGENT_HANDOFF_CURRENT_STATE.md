# Dragon MUGEN Agent Handoff Current State

Last updated: 2026-07-11

Repo: `C:\Users\kasom\projects\dragon-mugen`

Current branch at last workstation audit: `feature/aben-action-polish`

Purpose: this document is the short-form project memory for another Codex session or agent. Read this before making changes when the original workstation chat history is unavailable.

## Current Direction

Dragon MUGEN is being shaped into an owned-character fighting game and story/action game built around A.Ben and I.Chie.

The working target is:

- Owned roster first: `A.Ben` and `I.Chie`.
- MUGEN compatibility remains important, but third-party characters are reference or local test material only.
- The game should use a stable 1280x720 virtual presentation layout across screens.
- Resolution presets should improve render output quality, not reposition UI or change screen composition.
- Story mode should support board routes with side-scroller segments, shop stops, mid bosses, and arena-style boss conclusions.
- The Dragon Asset Lab and ComfyUI/LTX work should stay as tooling around assets, not core runtime logic.

## Non-Negotiable Rules

- Keep MUGEN data paths respected: `system.def`, `system.sff`, `system.snd`, `fight.def`, `fightfx.sff`, `select.def`.
- Runtime character and stage behavior should come from character/stage data where possible.
- Do not hardcode character moves into training UI. Use the character movelist/source data.
- Dragon-specific metadata belongs in Dragon-owned data such as `.dragon.def` or story board config, not in compatibility file rewrites.
- Do not add one-off runtime systems for a single asset if replacing the asset or using existing sprite slots solves it.
- Do not track third-party characters as owned content. A.Ben and I.Chie are the owned characters.
- Do not push unless explicitly requested.

## Completed Or Mostly Completed

### Resolution And Presentation

- Added a stable 1280x720 virtual presentation model.
- Canvas/resolution choices now represent output targets rather than layout grids.
- Added fullscreen/window policy work:
  - Fullscreen standard target is 1280x720.
  - Fullscreen toggle is available.
  - Minimize shortcut was requested and implemented in the fullscreen pass.
- Performance HUD can show FPS plus resolution.
- Main menu centering was corrected after visual iteration.

Remaining risk: some screens still show legacy or small misaligned elements, especially training, arena setup, story select/result/loading, and pause.

### Owned Roster

- DragonClaw was removed from owned/tracked direction.
- Owned characters are A.Ben and I.Chie.
- Other characters can exist locally as blueprint, reference, or compatibility test ideas, but should not be treated as repo-owned content.
- Roster readiness work verifies A.Ben and I.Chie presence and scale.

### A.Ben Fighter Work

- A.Ben has improved idle, walk, jump, punch, kick, crouch/duck, dash, and diagonal jump work in progress.
- Held jump repeat is implemented game-wide for fight-style movement.
- A.Ben walk was improved, but vertical/depth movement can still expose animation limitations.
- The sprite source frame convention has been moving from `384x672` toward `512x672` because the kick foot was being clipped.
- A.Ben replacement portrait work targeted the existing character SFF portrait slot rather than adding a new fallback system.

Remaining risk: A.Ben animation curation is still not final. The agent Asset Lab is expected to continue generating and curating sprite material.

### I.Chie Work

- I.Chie is present as shopkeeper and owned character.
- I.Chie shop placement, counter positioning, greeting interaction, and shop opening flow were iterated.
- Roadmap feature added: I.Chie should eventually be able to come from behind the counter and fight the player.

Remaining risk: I.Chie is not a finished fighter. Current combat use may still depend on placeholder or incomplete animation/state data.

### Shop Hub

- Shop hub uses the HD shop background and a single counter concept.
- Counter, I.Chie, A.Ben, contact shadows, and shop camera/zoom were iterated visually.
- A.Ben can approach the counter.
- I.Chie greeting appears before the shop opens.
- Shop opens after the greeting using light kick / PlayStation X style input.
- Shop overlay and shop closed state are visually close to the intended direction.

Remaining risk: keep checking shop hub after unrelated resolution/presentation changes because it is one of the reference-quality screens.

### Story Boards And Routes

- Story mode supports configurable board/route data in `game\data\story_boards.def`.
- Current board structure supports:
  - Side-scroller segments.
  - Mid boss segment.
  - Shop door segment.
  - Arena boss segment.
  - Route resume after shop.
  - Forward/clear cue image support with code fallback.
- Enemy roles are data-driven through `[Enemy Setup]`.
- Current role names are generic:
  - `grunts`
  - `mini_bosses`
  - `bosses`
- Difficulty behavior was shaped around:
  - Easy: one boss at the end.
  - Normal/medium: three waves with a mid boss after the first wave and a boss after wave three.
  - Hard: five waves with mini bosses after waves two and four and a boss after wave five.

Current example setup in `game\data\story_boards.def` may still use local/reference character IDs such as KFM, EvilKen, and EvilRyu as role assignments. Treat those as data examples, not owned content.

### Training Move List

- Training/practice should read moves from the actual character movelist/source data.
- Hardcoded all-character move display was identified as wrong and corrected or partially corrected.

Remaining risk: verify A.Ben training UI only shows moves A.Ben actually has.

### Dragon Asset Lab And Sprite Pipeline

- Asset Lab work is agent-owned and should continue in the separate web/tooling flow.
- Current reported Asset Lab state:
  - Web app accepts image plus prompt.
  - Sends to ComfyUI at `127.0.0.1:8196`.
  - Uses sprite-safe LTX first-pass route.
  - Finds generated MP4.
  - Runs repo sprite pipeline.
  - Writes contact sheets and previews into the A.Ben source folder.
  - Dashboard picks up newest source run.
- Repo-owned postprocessing tool exists:
  - `engine\tools\ltx_sprite_pipeline.py`
- Pipeline purpose:
  - Start from exported video.
  - Rip frames.
  - Generate contact sheets and preview GIFs.
  - Promote selected frames into curated game sprite folders.
  - Keep manual curation in the loop.

Do not make the game runtime depend on ComfyUI.

## Roadmap Status

| Area | Status | Notes |
| --- | --- | --- |
| Stable 1280x720 presentation | Mostly done | Main menu corrected. Remaining screens still need visual sweep. |
| Owned roster cleanup | Mostly done | A.Ben and I.Chie are owned. Avoid tracking DragonClaw or other third-party chars. |
| Shop hub | Strong WIP | Looks close to intended. Keep as visual reference. |
| Shop overlay | Strong WIP | Functional and visually close, but needs continued text-fit checks. |
| Story board routes | Functional WIP | Board route/shop/mid-boss/boss structure exists. Needs more content and polish. |
| Story select/result/loading UI | Partial | Known legacy/misaligned elements remain. |
| Training UI | Partial | Must verify move list comes from char data only. |
| Arena setup UI | Partial | Known legacy/misaligned elements remain. |
| Pause UI | Partial | Known small legacy presentation remains. |
| Fight correctness | Partial | Needs deeper MUGEN rule and controller coverage. |
| MUGEN compatibility depth | Ongoing | Preserve motif/data loading. Do not replace Classic behavior silently. |
| A.Ben animation set | WIP | Walk improved. Kick/crouch/jump/punch need curation and final SFF/AIR validation. |
| I.Chie fighter set | Early WIP | Shopkeeper works better than fighter readiness. |
| Dragon Asset Lab | External WIP | Agents are handling. Repo should consume outputs through pipeline. |
| Architecture cleanup | Ongoing | Avoid growing monolithic UI/runtime files. Group dirty changes before more features. |

## Known Dirty-Worktree Warning

The worktree has had broad active edits from multiple passes and agents. Before new feature work, run:

```powershell
cd C:\Users\kasom\projects\dragon-mugen
git status --short
```

Do not revert unrelated changes. Group dirty files by feature first.

Recent dirty categories observed:

- A.Ben SFF/AIR/CNS and curated sprite frame changes.
- Asset Lab and LTX sprite pipeline changes.
- Story, training, arena, stage select, and result overlay UI files.
- Verification scenario files.
- Generated proof screenshots under `artifacts\`.

Recommended cleanup order:

1. Keep or discard generated proof screenshots.
2. Verify and commit A.Ben `512x672` sprite contract changes if they are the current intended direction.
3. Verify and commit story board/enemy role setup changes.
4. Verify and commit presentation/UI cleanup changes.
5. Run the full verifier gate before pushing.

## Important Files

- Roadmap: `docs\ENGINE_COMPLETION_ROADMAP.md`
- Strict project rules: `docs\STRICT_ROADMAP.md`
- Feature history: `docs\FEATURE_LEDGER.md`
- Story board config: `game\data\story_boards.def`
- A.Ben char folder: `game\chars\A.Ben`
- I.Chie char folder: `game\chars\I.Chie`
- Sprite pipeline: `engine\tools\ltx_sprite_pipeline.py`
- Asset Lab app: `tools\dragon_asset_lab\app.py`

## Suggested Verification Gate

Run these after meaningful changes:

```powershell
cmake --build build --target dragon_mugen --config Debug
build\dragon_mugen.exe --verify owned-character-readiness
build\dragon_mugen.exe --verify roster-compatibility-smoke
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify video-resolution-stable-virtual-layout
build\dragon_mugen.exe --verify video-hd-fullscreen-window-policy
build\dragon_mugen.exe --verify story-board-route-plan
build\dragon_mugen.exe --verify story-stage-select-map
build\dragon_mugen.exe --verify story-stage-board-expansion
build\dragon_mugen.exe --verify story-difficulty-enemy-scaling
build\dragon_mugen.exe --verify story-shop-door-trigger
build\dragon_mugen.exe --verify story-shop-route-resume
build\dragon_mugen.exe --verify story-wave-spawn-scroll
build\dragon_mugen.exe --verify story-stage-clear
python engine\tools\dev_check.py . --skip-build
git diff --check
python tools\check_file_sizes.py .
```

Use visual screenshots for:

- Main menu.
- Options.
- Training/practice.
- Arena setup.
- Story select.
- Story loading.
- Story result.
- Pause menu.
- Shop closed.
- Shop greeting.
- Shop overlay buy/equip.
- A.Ben walk, jump, punch, kick, duck.

## Next Best Work

If continuing from this handoff, the next best pass is a presentation and UX sweep outside the shop:

1. Inspect current screenshots first before changing code.
2. Fix remaining legacy/misaligned elements in training, arena setup, story select, story loading/result, and pause.
3. Confirm every screen uses the stable 1280x720 virtual layout regardless of selected output resolution.
4. Confirm training move display is character-data-driven for A.Ben.
5. Confirm story waves show role names and progression clearly.
6. Then do a clean commit pass.

After that, return to gameplay feel:

- A.Ben final walk loop and vertical movement animation behavior.
- Crouch/duck collision and transitions.
- Better hit reactions.
- Shop/free-roam hitbox interactions.
- Story board enemy spawn feel.

## Browser Codex Startup Checklist

When picking this up from browser Codex:

```powershell
cd C:\Users\kasom\projects\dragon-mugen
git branch --show-current
git status --short
Get-Content docs\AGENT_HANDOFF_CURRENT_STATE.md
Get-Content docs\FEATURE_LEDGER.md -Tail 120
Get-Content docs\ENGINE_COMPLETION_ROADMAP.md
```

Then ask the user which visible issue they want prioritized unless they already gave a concrete next task.
