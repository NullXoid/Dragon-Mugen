# Dragon MUGEN Project Handoff

Last updated: 2026-07-11

This document is the quick pickup brief for Codex/browser agents. It summarizes the current project state, what has been completed, what is still open, and the rules that should guide the next pass.

## Current Repo State

- Main working repo: `C:\Users\kasom\projects\dragon-mugen`
- Current branch observed locally: `feature/aben-action-polish`
- Local branch state observed: ahead of origin by 1 commit, with a dirty worktree.
- Recent commits include:
  - `Fix story shop door input trigger`
  - `Promote curated A.Ben karate kick sprites`
  - `Promote wider A.Ben kick sprites`
  - `Promote wider A.Ben walk sprites`
  - `Add sprite-safe Asset Lab LTX first pass`
  - `Use character movelists for training commands`
  - `Stabilize versus loading layout`
  - `Add configurable story board routes`
  - `Polish fight training overlays`
  - `Stabilize presentation resolution scaling`

Before doing feature work, run:

```powershell
git -C C:\Users\kasom\projects\dragon-mugen status --short --branch
```

The worktree contains ongoing edits around overlays, verification scenarios, A.Ben/I.Chie character files, A.Ben curated sprites, `engine/tools/ltx_sprite_pipeline.py`, and `tools/dragon_asset_lab`.

## Project Direction

Dragon MUGEN is a M.U.G.E.N-compatible C++/SDL3 engine with Dragon-owned presentation, story, shop, arena, and asset-pipeline extensions.

Non-negotiable rules from `docs/STRICT_ROADMAP.md` still apply:

- Keep M.U.G.E.N-style `game/` content layout.
- `game/data/select.def` remains roster/stage authority.
- Character/stage data should stay editable in `.def`, `.air`, `.cmd`, `.cns`, `.sff`, `.snd`, `.dragon.def`, or related M.U.G.E.N-style files.
- Do not hardcode character, stage, story, or shop content into engine code when it belongs in data.
- Dragon-only extensions should be documented and should not silently break Classic/M.U.G.E.N behavior.

## Owned Content Policy

Owned original characters:

- `A.Ben`
- `I.Chie`

Other local characters such as KFM, Ken/Ryu variants, Lili variants, DragonBench, and other probes are test/reference/blueprint content only unless explicitly promoted later. Do not treat them as owned shipping content. Do not add DragonClaw or third-party characters as tracked project content.

## Completed Or Mostly Completed

### Resolution And Presentation Scaling

- The game has moved toward a fixed presentation standard of `1280x720`.
- Resolution changes are intended to improve graphics and render quality, not redesign the screen each time.
- Main menu centering and several overlay scaling issues were corrected.
- FPS/performance HUD can include resolution information.

Still verify game-wide: some story, training, arena, loading, and result overlays previously showed old 320x240/aspect behavior.

### Main Menu And Options

- Main menu was redesigned into the Dragon visual style.
- Options support canvas/resolution presentation work.
- Several menu/overlay positioning fixes landed.

Remaining polish: make every Dragon-owned menu and submenu use the same safe layout and not drift or resize oddly between windowed/fullscreen.

### Shop Hub

- Shop uses the HD I.Chie item counter background.
- Counter was changed from stretched/full-width to a placed prop with fixed positioning.
- I.Chie is centered at the counter area.
- A.Ben can approach the counter.
- Redundant under-player prompt was removed in favor of the bottom prompt.
- I.Chie greeting flow exists.
- After greeting, shop can open with light kick / X style input.
- Shop overlay, camera/zoom, and HUD were visually improved.

Known remaining shop work:

- Verify the shop still looks correct after resolution/layout changes.
- Keep counter and shopkeeper world positions fixed in world/camera space so they do not slide when A.Ben moves.
- Later feature idea: I.Chie can leave the counter and fight the player.

### Story Board System

`game/data/story_boards.def` is now the editable route/board source.

Current structure supports:

- Route metadata.
- Board/segment definitions.
- Segment types such as `side_scroller`, `mid_boss`, `shop`, and `arena_boss`.
- Per-wave enemies, spawn positions, clear text, XP, and gold rewards.
- Shop door segment with prompt/cue settings.
- Custom clear/shop cue images.
- Enemy role mapping through `[Enemy Setup]`.

Current enemy role setup:

```ini
[Enemy Setup]
grunts = kfm
mini_bosses = EvilKen
bosses = EvilRyu
```

This is the intended direction: story data names roles such as grunts, mini-bosses, and bosses, and the data file chooses which character folders fill those roles.

Remaining story work:

- Verify difficulty wave counts: easy, normal/medium, hard.
- Verify normal/medium has a mid-boss before final boss and hard has mid-bosses after the intended waves.
- Improve loading and stage-clear screens.
- Support smooth board continuation, shop stops, side-scroller boards, and arena boss conclusions in a user-friendly flow.

### Fight Movement And Character Control

- Held jump repeat was implemented for fight-style movement.
- Ducking was added.
- A.Ben has improved stand, walk, jump, punch, kick, dash/crouch sprite work in progress.
- A.Ben kick frame bounds were expanded to `512x672` to prevent cut-off.
- Vertical/depth movement uses existing walk animation behavior; full eight-direction walk art was intentionally deferred.

Known remaining movement/animation work:

- Verify A.Ben walk loop has a true second step and does not look like only a startup loop.
- Verify foot is not cut off on kick after the 512x672 update.
- Finish punch/kick/dash/jump visual polish and hitboxes.
- Add diagonal jump animations when assets are ready.

### Training Movelist

Training/practice should use the character's actual movelist instead of a hardcoded global move list.

Current A.Ben movelist file:

```text
game/chars/A.Ben/A.Ben.movelist
```

Current entries:

```text
Quick Jab        ^x
Straight Punch   ^y
Boost Palm       ^z
Shin Tap         ^a
Side Kick        ^b
Blue Arc Kick    ^c
```

Remaining work:

- Verify training only shows moves the selected character actually has.
- Add or clean movelist files for I.Chie once her fighter implementation is ready.

### Dragon Asset Lab

Asset Lab exists under:

```text
tools/dragon_asset_lab
```

The repo-owned sprite postprocessor exists at:

```text
engine/tools/ltx_sprite_pipeline.py
```

Current capabilities from the Asset Lab brief:

- Dashboard loads local character pages.
- Owned character pages render.
- Safe media serving blocks unsupported/traversal paths.
- Invalid frame selections are rejected without manifest mutation.
- ComfyUI/LTX output can be processed through the repo pipeline.
- `prepare`/`promote` creates frame rips, contact sheets, previews, manifests, and promoted sprite assets.
- A.Ben action outputs are stored under the character source-art folders.
- A sprite previewer/source-run view exists for reviewing generated contacts and previews.

Asset Lab next steps from `tools/dragon_asset_lab/NEXT_AGENT_BRIEF.md`:

- Add an owned-only proof path for A.Ben and I.Chie.
- Make local test/probe characters opt-in in the UI.
- Add I.Chie asset pipeline folders and state display.
- Add practical workflow adapters for known LTX image-to-video and image-to-image workflows.
- Add dashboard visual smoke proof.

## Current Known Issues

- Worktree is not clean. Do not commit blindly.
- Some generated artifacts/screenshots may still be untracked.
- Some screens still need layout correction after the resolution standardization.
- Story loading and stage-clear screens need visual polish.
- Story mode should be verified against role-based enemy setup and difficulty wave counts.
- Training move display must stay data-driven from character movelists.
- A.Ben and I.Chie are not both complete as fighting characters.
- Local test characters exist but should not blur owned-character readiness.

## Verification Gate For Next Stable Checkpoint

Use this after grouping/cleaning the current work:

```powershell
cmake --build build --target dragon_mugen --config Debug
build\dragon_mugen.exe --verify cpu-baseline
build\dragon_mugen.exe --verify roster-compatibility-smoke
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify story-stage-clear
build\dragon_mugen.exe --verify options-category-navigation
build\dragon_mugen.exe --verify shop-demo-room-hook
build\dragon_mugen.exe --verify shop-room-actor-projection
build\dragon_mugen.exe --verify shop-room-movement-collision
python engine\tools\dev_check.py . --skip-build
git diff --check
python tools\check_file_sizes.py .
```

Add visual checks when UI/presentation changes:

- Main menu at 1280x720, windowed and fullscreen.
- Options screen.
- Story fighter select.
- Story board select.
- Story loading screen.
- Story side-scroller board.
- Stage clear/result screen.
- Shop closed.
- Shop greeting.
- Shop open.
- Training move overlay.
- Arena setup overlay.

## Recommended Next Passes

1. **Clean Working Tree By Feature Group**
   - Separate sprite/character changes, Asset Lab changes, overlay/layout changes, and story-board changes.
   - Remove generated junk that should not be tracked.
   - Commit only coherent, verified groups.

2. **Finish Game-Wide Resolution Layout**
   - Keep 1280x720 as the standard presentation canvas.
   - Resolution should improve clarity, not change UI composition.
   - Fix remaining old-aspect screens: story select, loading, stage clear, arena setup, training overlay, fight result.

3. **Finalize Story Board Roles And Difficulty**
   - Keep generic role names in data: grunts, mini-bosses, bosses.
   - Let `story_boards.def` choose which character folders fill each role.
   - Verify easy/normal/hard wave counts and boss placement.

4. **Polish Loading And Result Screens**
   - Use the Dragon visual system.
   - Add/replace loading portrait using A.Ben/I.Chie owned art.
   - Do not add a new one-off system just for a portrait; replace the relevant character portrait sprite/data.

5. **Asset Lab To Game Sprite Loop**
   - Keep ComfyUI/LTX outside the repo.
   - Use Asset Lab and `ltx_sprite_pipeline.py` for frame rip, contact sheets, selected frame promotion, and manifests.
   - Store source videos and generated contacts under each character's `source_art` folder.
   - Promote only curated frames into game sprites.

6. **Owned Character Readiness**
   - A.Ben: finish stand/walk/jump/punch/kick/dash/duck quality and hitboxes.
   - I.Chie: decide shop-only versus fighter readiness, then build the same source-art pipeline.
   - Keep training/practice movelists data-driven per character.

7. **Future Feature**
   - Shopkeeper combat: I.Chie can leave the counter and fight the player.
   - Treat this as a later gameplay feature, not a quick shop hack.

## Handoff Rules For Browser Codex

- Start by reading this file, then `docs/STRICT_ROADMAP.md`, then the specific subsystem doc or data file you are touching.
- Do not assume local test characters are shippable.
- Do not hardcode content that belongs in M.U.G.E.N-style files or Dragon data files.
- Do not add a special system for one broken visual if replacing the actual asset/data is enough.
- Use screenshots as proof for presentation changes.
- Use verifiers plus `dev_check`, `git diff --check`, and file-size checks before calling a pass stable.
- If changing Asset Lab, read `tools/dragon_asset_lab/NEXT_AGENT_BRIEF.md` first.
