# Dragon Asset Lab MVP Completion Notice

Implemented a local stdlib Python web app prototype under `tools/dragon_asset_lab`.

## Implemented

- Character workspace browser for owned characters `A.Ben` and `I.Chie`.
- Local-only compatibility browser support for ignored `EvilRyu` and `EvilKen` folders when they exist.
- Workspace presence checks for `source_art`, `source_videos`, `shop`, and `source_art/curated_game_sprites`.
- Minimal MUGEN DEF parsing for display metadata and `[Files]` runtime references.
- Runtime file presence checks for CMD, CNS, common CNS, SFF, AIR, SND, palettes, movelists, and storyboards.
- AIR action counting for full local characters.
- Action dashboard for `idle`, `walk`, `jump`, `punch`, `kick`, and `dash`.
- Frame counts from curated frame folders, promoted frame metadata, or selected source frame metadata.
- Contact sheet and preview GIF display from curated `contacts` and `previews` folders.
- Source video detection from `source_videos/manifest.json` and local MP4 fallback globbing.
- Safe local media serving for supported image/video extensions inside the repo root.
- Read-only curated manifest viewer.
- Export/proof command display blocks for existing pipeline, SFF build, and roster verification commands.
- README with run instructions, limits, and intended Comfy/LTX workflow.

## How To Test

From the repo root:

```powershell
python tools/dragon_asset_lab/app.py
```

Open:

```text
http://127.0.0.1:8765/
```

Expected MVP behavior:

- `A.Ben` shows curated contacts/previews and frame counts for available actions.
- `I.Chie` shows existing workspace folders but mostly empty action dashboard data.
- `EvilRyu` and `EvilKen`, when present locally, show DEF metadata, runtime file statuses, and AIR action counts without tracking or copying those assets.
- Manifest viewer is read-only.
- Command blocks are displayed but not executed.

## Verification Performed

- Python syntax check with `python -m py_compile tools/dragon_asset_lab/app.py`.
- Local launch and HTTP query against `/` to confirm the app serves HTML.
- Local query against an A.Ben preview GIF asset URL to confirm media serving works.

## Notes

This MVP intentionally does not call ComfyUI, does not edit manifests, does not rewrite generated assets, and does not touch runtime/gameplay files.
