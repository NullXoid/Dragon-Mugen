# A.Ben Prototype Bundle

This local bundle includes a generated prototype `A.Ben.sff` plus the DEF/CMD/CNS/AIR text foundation.

The active trial source art lives under `source_art/curated_game_sprites`.
Shop-facing A.Ben assets live under `shop/` so the shop and fighter presentation share the same character folder.

Current pass target:
- standing/idle: SFF group `0`
- walking: SFF group `20`
- neutral jumping: SFF group `40`
- forward diagonal jumping: SFF group `42`
- back diagonal jumping: SFF group `43`
- punching: SFF group `200`

The source videos used for this pass are stored in `source_videos/`.
The current diagonal jump frames are derived from the curated neutral jump source and stored in
`source_art/curated_game_sprites/frames/jump_forward` and
`source_art/curated_game_sprites/frames/jump_back`.
