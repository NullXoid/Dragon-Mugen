# DLC 1 Arena Mode

Arena Mode is an optional gameplay prototype for Dragon MUGEN Core. It adds a main menu entry that starts a free-for-all battle with one player fighter and one to three CPU fighters.

## How To Enter

From the main menu, choose `ARENA MODE`, select a fighter, adjust CPU count on the Arena Setup screen, then start the match.

## Editable Content

Classic MUGEN characters, stages, states, animations, sounds, commands, and fight files remain editable through normal MUGEN files.

Arena-specific labels, default stage, CPU count limits/default, selection mode, rules, and result titles are editable in `dlc/arena/arena_config.json`.

## Prototype Limits

This first pass reuses the existing 2D fight loop and adds a small free-for-all layer. CPU fighters use random roster selections and simple nearest-living-enemy targeting.

This DLC does not add Z-axis movement, scroller stages, story mode, shops, equipment, crafting, world maps, NPC dialogue, tournament progression, Dragon Mode, or advanced arena AI.
