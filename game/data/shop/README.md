# I.Chie Shop Art Drop-In Prompts

The shop renderer will use these PNG files when they exist. If any file is missing, the engine keeps the current code-drawn fallback so the shop remains playable.

Save generated files here:

```text
game/data/shop/i_chie_shop_backdrop.png
game/data/shop/i_chie_shop_counter_front.png
```

## 1. Full Room Backdrop

Filename:

```text
i_chie_shop_backdrop.png
```

Recommended size: `1536x864` or `2048x1152`.

Prompt:

```text
Create a wide 16:9 anime action-RPG shop interior background for a 2D fighting game hub.

Scene: a modern martial-arts item shop with dark wood, black metal, warm shelf lighting, cyan neon strips, purple dragon accents, display shelves, small training gear, bottles, charms, and a polished floor with enough open walking space in front.

Composition: side-view game background, camera at player height, wide horizontal layout, service counter area implied but leave the actual counter front out because it will be a separate game layer. Keep the center-right area clear for a shopkeeper sprite and the lower middle clear for the player character.

Style: polished anime game background, detailed but readable at small size, high-contrast shapes, clean lighting, subtle futuristic RPG shop mood, no gritty realism.

Color palette: black, dark brown, muted gold, teal/cyan, purple accents.

Constraints: no people, no characters, no UI panels, no menus, no icons, no charts, no labels, no readable text, no watermark, no logo. Do not create an infographic. Do not create a poster. This is a game stage background only.
```

## 2. Counter Front Layer

Filename:

```text
i_chie_shop_counter_front.png
```

Recommended size: `1536x320`, transparent PNG preferred.

Prompt:

```text
Create a transparent PNG game-layer asset for the front face of a futuristic martial-arts shop counter.

Subject: the front counter wall that can visually cover the lower body of a player walking behind it. Use dark wood panels, black metal edge caps, thin gold trim, a subtle purple dragon emblem, and a warm/cyan edge glow. The prop should feel like a premium RPG item shop counter.

Style: anime action-RPG environment prop, clean readable shapes, strong silhouette, detailed enough for a 2D game but not noisy.

Composition: one long horizontal counter-front segment spanning nearly the full canvas width, with transparent background around it. No floor or back wall.

Constraints: transparent background, no people, no characters, no UI, no text, no labels, no watermark, no logo.

If transparent output is unavailable, render the asset on a perfectly flat solid #00ff00 chroma-key background with no shadows, gradients, texture, reflections, or lighting changes in the green area. Do not use #00ff00 in the counter.
```

## 4. Optional Replacement Shopkeeper Portrait/Standee

This phase does not require a new I.Chie sprite because the shop already loads:

```text
game/chars/I.Chie/I.Chie_shopkeeper_pose.png
```

If a replacement is generated later, preserve her current look: black and purple outfit, long dark hair with purple accents, purple eyes, confident shopkeeper expression, and a small tablet/device in hand.

Prompt:

```text
Create a full-body 2D anime shopkeeper sprite of I.Chie for a fighting game shop hub.

Subject: young adult woman with long dark hair, purple highlights, purple eyes, black and purple outfit, confident friendly saleswoman posture, holding a small glowing shop tablet. She should look like the existing I.Chie concept: sleek, dragon-themed, purple-accented, modern martial-arts shopkeeper.

Pose: standing behind or beside a shop counter, relaxed but ready to sell items, one hand holding the tablet, one arm slightly open in a welcoming gesture.

Style: clean 2D anime game sprite, full body, readable silhouette, consistent proportions, no chibi, no photorealism.

Composition: character isolated with full body visible and generous padding.

Constraints: transparent background, no counter, no UI, no text, no labels, no watermark, no logo.

If transparent output is unavailable, render the sprite on a perfectly flat solid #00ff00 chroma-key background with no shadows, gradients, texture, reflections, or lighting changes in the green area. Do not use #00ff00 in the character.
```
