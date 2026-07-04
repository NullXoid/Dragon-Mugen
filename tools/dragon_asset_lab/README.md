# Dragon Asset Lab

Dragon Asset Lab is a narrow local MVP for browsing Dragon MUGEN character sprite asset workspaces. It is intentionally a sidecar tool and does not integrate with the game runtime or ComfyUI internals.

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
python tools/dragon_asset_lab/app.py --repo-root C:\Users\kasom\projects\dragon-mugen-arena --port 8766
```

## Current MVP Scope

- Browses `game/chars/A.Ben` and `game/chars/I.Chie`.
- Shows whether `source_art`, `source_videos`, `shop`, and `source_art/curated_game_sprites` exist.
- Shows action dashboard rows for `idle`, `walk`, `jump`, `punch`, `kick`, and `dash`.
- Reads curated action metadata from `source_art/curated_game_sprites/manifest.json` when present.
- Reads source video metadata from `source_videos/manifest.json` when present.
- Serves local PNG, GIF, MP4, JPG, JPEG, and WEBP files from inside the repo root.
- Shows contact sheets and preview GIFs from `contacts` and `previews`.
- Displays curated manifest action metadata read-only.
- Displays export/proof command stubs instead of executing them.

## Limits

- No ComfyUI API calls.
- No generated asset rewrites.
- No manifest writes. The manifest view is read-only to avoid corrupting curated data while the schema is still settling.
- Local-only prototype. The server binds to `127.0.0.1` by default and only serves supported media files under the repo root.

## Intended Comfy/LTX Workflow

1. Export or place completed LTX/Comfy videos under the character `source_videos` folder.
2. Use the existing pipeline to prepare a reviewable run:

   ```powershell
   python engine/tools/ltx_sprite_pipeline.py prepare --character A.Ben --action walk --video game/chars/A.Ben/source_videos/walk_LTX-2_00068_.mp4
   ```

3. Select frames in the run manifest or pass a selected frame list, then promote them:

   ```powershell
   python engine/tools/ltx_sprite_pipeline.py promote --run-dir game/chars/A.Ben/source_art/ltx_runs/<run-name> --selected 0,6,12
   ```

4. Reopen Dragon Asset Lab to inspect updated contacts, previews, frame counts, and manifest metadata.
5. Run the existing build/proof commands when ready:

   ```powershell
   python engine/tools/build_aben_walk_sff.py
   build/dragon_mugen.exe --verify roster-compatibility-smoke
   ```

Future versions can add schema-aware manifest editing, command execution with logs, and I.Chie-specific promotion flows.
