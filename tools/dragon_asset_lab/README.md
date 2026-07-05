# Dragon Asset Lab

Dragon Asset Lab is a local stdlib Python web dashboard for Dragon MUGEN sprite asset workspaces. It is a sidecar tool: browsing is available for owned and local probe characters, while write actions are limited to owned production targets.

## Run

From the repo root:

```powershell
python tools/dragon_asset_lab/app.py
```

Then open:

```text
http://127.0.0.1:8765/
```

Optional arguments:

```powershell
python tools/dragon_asset_lab/app.py --repo-root C:\Users\kasom\projects\dragon-mugen --port 8766
```

## Implemented

- Browses owned `A.Ben` and `I.Chie` workspaces, plus ignored local-only `EvilRyu` and `EvilKen` compatibility probes when present.
- Parses character DEF files, reports runtime file presence, and counts AIR action blocks.
- Shows curated contacts/previews alongside source run contacts/previews and source frame counts for curated actions.
- Allows explicit browser edits to curated `selected_source_frames` for owned characters.
- Validates selected frame numbers against the action source frame count before saving.
- Writes manifest edits atomically and creates timestamped `.bak` files before replacing JSON.
- Runs `engine/tools/ltx_sprite_pipeline.py promote` from the browser for action-specific LTX runs and shows stdout/stderr in the page.
- Creates manifest backups before promotion because the existing promote tool rewrites run and curated manifests.
- Blocks unsafe promotion for derived actions whose run manifest action does not match the curated action, such as current A.Ben `crouch` derived from the `jump` run.
- Exposes A.Ben sprite rebuild buttons for the full curated action-source-root path and walk-only rebuild path.
- Imports/registers completed source videos into `game/chars/<character>/source_videos`, with optional `ltx_sprite_pipeline.py prepare`.
- Stores local Comfy/LTX configuration in ignored `artifacts/asset_lab/dragon_asset_lab_config.json`.
- Supports optional direct Comfy HTTP submission to `/prompt` only when explicitly enabled and a workflow JSON path is configured.
- Shows image-to-image workflow configuration/status and submits that workflow JSON as-is when configured.
- Provides proof helper buttons for `dev_check --skip-build`, CPU baseline, and roster compatibility smoke with `DRAGON_ROSTER_SCREENSHOT_DIR` set under `artifacts/asset_lab`.
- Keeps live game launch as a command stub instead of inventing screen automation.
- Serves only allowed local media suffixes from inside the repo root.

## Limits / Remaining

- I.Chie has no dedicated SFF builder yet, so the browser reports that instead of guessing.
- Direct Comfy submission does not mutate workflow graphs or inject prompts; import completed videos for stable handoff.
- Browser promotion is only enabled when the curated action has an action-matching LTX run manifest.
- Source video imports are explicit user actions and are limited to owned characters.
- Local probe characters remain browse-only; do not track or copy third-party character assets.

## Typical Workflow

1. Save or copy a completed Comfy/LTX video through the Source Video Import / Prepare form.
2. Optionally run prepare from the same form to create an `source_art/ltx_runs/<run>` folder.
3. Review source contact/preview context in the action card.
4. Edit `selected_source_frames` and save. Invalid frame numbers are rejected.
5. Promote selected frames from the action card when the run action matches.
6. Rebuild A.Ben sprites with the full curated action-source-root button or walk-only button.
7. Run proof helpers and inspect command logs in the UI.
