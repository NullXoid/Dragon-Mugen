# A.Ben Source Art Notes

This folder is the repo-owned handoff point for A.Ben-generated sprite work.

Current trial source:
- `curated_game_sprites/frames/idle`
- `curated_game_sprites/frames/crouch`
- `curated_game_sprites/frames/walk`
- `curated_game_sprites/frames/jump`
- `curated_game_sprites/frames/jump_forward`
- `curated_game_sprites/frames/jump_back`
- `curated_game_sprites/frames/punch`

Pass criteria for this trial:
- Fighter idle/standing loads from group `0`.
- Fighter duck/crouch loads from group `10`.
- Fighter walk loads from group `20`.
- Fighter neutral jump loads from group `40`.
- Fighter forward diagonal jump loads from group `42`.
- Fighter back diagonal jump loads from group `43`.
- Fighter punch loads from group `200`.
- Shop A.Ben walk/pose assets load from `game/chars/A.Ben/shop`, not from `game/data/shop`.

Comfy/LTX remains an external generator. The repo-owned pipeline starts from exported videos and curated frame folders.
