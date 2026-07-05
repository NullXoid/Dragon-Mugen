# Dragon Asset Lab Completion Notice

Implemented the remaining browser-side Asset Lab feature set under `tools/dragon_asset_lab`.

## Implemented

- Kept the existing local dashboard for `A.Ben`, `I.Chie`, and local-only ignored `EvilRyu`/`EvilKen` probes.
- Split the app into path/schema helpers, workspace inspection, command operations, and the HTTP/UI layer.
- Added action cards with curated preview/contact context and source run preview/contact context.
- Added explicit `selected_source_frames` editing for owned character curated actions.
- Validates frame selections against available source frame counts and rejects out-of-range or malformed input.
- Writes manifest changes atomically with timestamped `.bak` backups.
- Added browser promotion through `engine/tools/ltx_sprite_pipeline.py promote`, with command output shown in the UI.
- Adds backups before promotion because the existing promote command updates run and curated manifests.
- Blocks derived-action promotion when the run manifest action differs from the curated action.
- Added A.Ben full-action and walk-only rebuild buttons around `engine/tools/build_aben_walk_sff.py`.
- Added source video import/register support into `source_videos`, with optional `ltx_sprite_pipeline.py prepare`.
- Added Comfy/LTX config for server URL, video workflow JSON, image-to-image workflow JSON, and output folder.
- Stores config in ignored `artifacts/asset_lab/dragon_asset_lab_config.json`.
- Added optional Comfy HTTP `/prompt` submission, disabled unless explicitly configured.
- Added proof helper buttons for `dev_check --skip-build`, CPU baseline, and roster smoke with roster screenshot output.
- Preserved safe media serving: only approved media suffixes under the repo root are served.

## Intentionally Deferred

- No I.Chie SFF builder is invented; the UI reports that no safe builder exists yet.
- Direct Comfy support submits configured workflow API JSON as-is and does not chase Comfy internal folders or mutate workflows.
- Live interactive game launch remains a command stub; scripted verifiers are run instead.
- Derived curated actions such as current A.Ben `crouch` can be edited but not promoted through the browser until they have action-matching run manifests.

## Verification Notes

Expected verification commands for this change:

```powershell
python -m py_compile tools\dragon_asset_lab\app.py tools\dragon_asset_lab\lab_paths.py tools\dragon_asset_lab\lab_workspace.py tools\dragon_asset_lab\lab_ops.py engine\tools\ltx_sprite_pipeline.py engine\tools\build_aben_walk_sff.py
python engine\tools\dev_check.py . --skip-build
git diff --check
python tools\check_file_sizes.py .
```

Runtime smoke should include loading `/`, opening an allowed asset URL, and confirming invalid frame selections are rejected without modifying manifests.
