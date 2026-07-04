# A.Ben Prototype Bundle

This owned prototype bundle includes:

- `A.Ben.sff`
- `A.Ben.air`
- `A.Ben.cmd`
- `A.Ben.cns`
- preview portrait/icon images
- curated source art under `source_art/curated_game_sprites`
- source videos under `source_videos`
- shop-facing A.Ben assets under `shop`
- older derived prototype action PNGs under `generated_actions`

The curated LTX/Comfy post-processing path is the forward asset workflow. `engine/tools/ltx_sprite_pipeline.py` prepares exported videos into contact sheets, preview GIFs, manifests, and promoted `384x672` action frames. `engine/tools/build_aben_walk_sff.py` rebuilds the active A.Ben SFF from those curated action folders.

Current curated action targets:

- standing/idle: SFF group `0`
- walking: SFF group `20`
- dash: SFF group `100`
- neutral jumping: SFF group `40`
- forward diagonal jumping: SFF group `42`
- back diagonal jumping: SFF group `43`
- punching: SFF group `200`
- kicking: SFF group `230`

The current diagonal jump frames are derived from the curated neutral jump source and stored in `source_art/curated_game_sprites/frames/jump_forward` and `source_art/curated_game_sprites/frames/jump_back`.

The `generated_actions/` bridge frames and `engine/tools/build_aben_action_sff.py` remain prototype reference material from the earlier action-sprite pass. Replace or retire them once the curated source-video pipeline covers each combat action cleanly.

Useful checks:

- `python engine/tools/audit_mugen_compat.py game`
- `build/dragon_mugen.exe --verify roster-compatibility-smoke`
- `build/dragon_mugen.exe --verify kfm-movement-direction-audit`
- `python engine/tools/dev_check.py . --skip-build`
