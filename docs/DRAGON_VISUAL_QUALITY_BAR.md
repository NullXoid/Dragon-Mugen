# Dragon Visual Quality Bar

Dragon-owned presentation work has to look intentional at every supported canvas size before it is considered complete.

## Baseline

- Preserve M.U.G.E.N motif assets when a screen is motif-backed. Frame or enhance them; do not replace them with unrelated placeholder effects.
- Every visual effect needs a named purpose: shadow, occlusion, highlight, state feedback, motion, or lighting.
- Do not fake polish with stacked translucent rectangles, random overlays, or hardcoded blocks. Use an authored asset, a simple clean panel treatment, or a renderer path that can be verified.
- Pixel UI must remain nearest-filtered and aligned to the logical grid.
- HD/world art may use linear filtering, but M.U.G.E.N/SFF sprites stay nearest-filtered.
- Layouts must be responsive by canvas class, not manually recoded per resolution.

## Menu And Options Screens

- Main menu rows must never overlap at 320x240, 426x240, 480x240, 854x480, or 1280x720.
- HD menus should not blindly scale to 3x when that makes the panel dominate the screen. Use capped presentation scale where it improves composition.
- Menu shadows must come from motif shading or a clean panel treatment. Blocky layered-opacity effects are not acceptable.
- FPS and performance HUD must show the active logical resolution when enabled.

## Shop Screens

- World composition and UI overlay are separate layers.
- Characters need stable ground contact, shadows, and consistent origin handling.
- Shop overlay must remain readable at every canvas preset, using a full/near-full layout for Classic when needed.

## Verification

Visual work should add or update a verifier when a rule can be checked mechanically. Required recurring checks include:

- `main-menu-responsive-layout`
- `shop-overlay-responsive-layout`
- `shop-overlay-classic-full-layout`
- `shop-overlay-sd-layout`
- `shop-overlay-hd-layout`
- `world-texture-filter-selection`
- `ui-nearest-filter-selection`
