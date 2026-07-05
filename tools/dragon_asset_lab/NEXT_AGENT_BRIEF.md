# Dragon Asset Lab Next Agent Brief

## Audit Result

Asset Lab is a working local sidecar at the current checkpoint. The dashboard loads, owned character pages render, safe media serving blocks unsupported and traversal paths, invalid frame selections are rejected without modifying manifests, and the `ltx_sprite_pipeline.py prepare/promote` path works on a disposable MP4.

Current passing checks:

```powershell
python -m py_compile tools\dragon_asset_lab\app.py tools\dragon_asset_lab\lab_paths.py tools\dragon_asset_lab\lab_workspace.py tools\dragon_asset_lab\lab_ops.py engine\tools\ltx_sprite_pipeline.py engine\tools\build_aben_walk_sff.py
python engine\tools\dev_check.py . --skip-build
git diff --check
python tools\check_file_sizes.py .
cmake --build build --target dragon_mugen --config Debug
build\dragon_mugen.exe --verify cpu-baseline
build\dragon_mugen.exe --verify roster-compatibility-smoke
```

`roster-compatibility-smoke` currently passes with partials for local probe characters. Do not treat that as an Asset Lab failure, but do not let local probe characters blur owned-character readiness.

## Required Next Work

1. Add an owned-only proof path.
   - The current Asset Lab proof button runs broad `roster-compatibility-smoke`, which includes local/probe characters on this machine.
   - Add or route to an owned-only verifier for `A.Ben` and `I.Chie`, or add an Asset Lab proof mode that filters to owned characters.
   - Keep local probe character inspection available only as explicit browse/reference context.

2. Make local probes opt-in in the UI.
   - `OWNED_CHARACTERS` is `A.Ben`, `I.Chie`.
   - `LOCAL_TEST_CHARACTERS` currently appear beside owned characters when present.
   - Default dashboard navigation should prioritize owned characters and clearly label or hide local probes behind an opt-in section.

3. Add the I.Chie asset pipeline path.
   - I.Chie is browseable, but has no curated manifest/source runs/SFF builder yet.
   - Create the same durable folder contract used by A.Ben:
     `game/chars/I.Chie/source_art/ltx_runs`, `curated_game_sprites`, and `source_videos`.
   - Do not invent unsafe SFF generation if the I.Chie build path is not ready; first make the dashboard show the missing pipeline state clearly.

4. Upgrade Comfy/LTX submission from raw workflow POST to practical workflow adapters.
   - Current direct submit sends configured workflow JSON as-is.
   - Add explicit adapters for known local LTX image-to-video and image-to-image workflows when their node ids are known.
   - The adapters should inject prompt/reference/output settings without chasing Comfy internals.
   - Keep raw workflow POST available only as an advanced/manual mode.

5. Add dashboard visual smoke proof.
   - Start the Asset Lab server, capture the A.Ben page and I.Chie page, and verify action cards/config/proof controls are visible.
   - This can be a small script or documented manual check, but it should not leave a server running.

## Do Not Change

- Do not commit or track third-party/probe character assets.
- Do not remove the existing A.Ben pipeline contract.
- Do not mutate ComfyUI installs or depend on ComfyUI internal folders.
- Do not touch unrelated story-board changes in the current worktree.
