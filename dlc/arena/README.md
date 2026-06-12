# DLC 1 Arena Mode

Arena Mode is an optional gameplay prototype for Dragon MUGEN Core. It adds a main menu entry that starts a free-for-all battle with one player fighter and one to three CPU fighters, with optional depth movement, first-pass pseudo-yaw camera rotation, and first-pass OpenBOR-style scrolling stages.

## How To Enter

From the main menu, choose `ARENA MODE`, select a fighter, adjust CPU count on the Arena Setup screen, then start the match.

## Editable Content

Classic MUGEN characters, stages, states, animations, sounds, commands, and fight files remain editable through normal MUGEN files.

Arena-specific labels, default stage, CPU count limits/default, selection mode, rules, and result titles are editable in `dlc/arena/arena_config.json`.

## Prototype Limits

This pass reuses the existing fight loop and adds a free-for-all layer. CPU fighters use roster selections, ignore defeated fighters, choose nearest living enemies, and align depth when Z-axis mode is enabled.

Arena Z-axis movement is optional from Arena Setup. When enabled, hold Shift or the left trigger to move Up/Down along depth; normal Up/Down still jump/crouch unless the modifier is held. A quick double-tap of the modifier performs a short sidestep. Depth movement and sidesteps use the fighter's authored walk animation while grounded.

Camera Rotate is also optional and defaults off. It only activates when Z Axis is on. It rotates fighter/helper/projectile/effect projection and draw order from Arena `x + depthZ` space, but combat math and M.U.G.E.N stage backgrounds stay unrotated for this pass.

Simple OpenBOR-style scrolling is opt-in per stage through `[OpenBOR]` or `[DragonOpenBOR]` metadata in the stage `.def`. `OpenBOR Scroll Test` is the current sample. The scroller is Arena-only; Training, Single Player, and VS continue to use normal M.U.G.E.N camera behavior.

This DLC does not add story mode, shops, equipment, crafting, world maps, NPC dialogue, tournament progression, Dragon Mode, hazards, platforms, or advanced arena AI.
