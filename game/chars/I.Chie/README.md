# I.Chie Prototype Bundle

This owned prototype bundle includes a generated `I.Chie.sff` built from the user-provided front reference image plus the DEF/CMD/CNS/AIR text foundation.

I.Chie is currently selectable and runtime-loadable, but her production fighter animation set is not ready yet. Treat the current fighter actions as placeholder coverage until curated standing, walk, jump, punch, kick, and hit reaction folders are added under `source_art/`.

Dragon shop/NPC content:
- `I.Chie_shopkeeper_pose.png` is a derived saleswoman pose that keeps the current prototype head, hair, body, legs, and palette.
- `I.Chie.sff` includes shopkeeper sprite `9100,0`.
- `I.Chie.air` includes action `9100`.
- `I.Chie.cns` includes non-combat state `9100`.
- `I.Chie.dragon.def` marks the character as a shop saleswoman and points shop code at action/state `9100`.

Current readiness:
- Shop/NPC presentation: active.
- Selectable roster/runtime loading: active.
- Production fighter action art: deferred.

Useful checks:
- `build/dragon_mugen.exe --verify owned-character-readiness`
- `build/dragon_mugen.exe --verify roster-compatibility-smoke`
- `python engine/tools/dev_check.py . --skip-build`
