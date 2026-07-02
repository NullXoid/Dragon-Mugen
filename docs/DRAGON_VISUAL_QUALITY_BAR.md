# Dragon Visual Quality Bar

Dragon-owned presentation work has to look intentional at every supported output resolution before it is considered complete.

## Baseline

- Preserve M.U.G.E.N motif assets when a screen is motif-backed. Frame or enhance them; do not replace them with unrelated placeholder effects.
- Every visual effect needs a named purpose: shadow, occlusion, highlight, state feedback, motion, or lighting.
- Do not fake polish with stacked translucent rectangles, random overlays, or hardcoded blocks. Use an authored asset, a simple clean panel treatment, or a renderer path that can be verified.
- Pixel UI must remain nearest-filtered and aligned to the logical grid.
- HD/world art may use linear filtering, but M.U.G.E.N/SFF sprites stay nearest-filtered.
- Gameplay, menus, and Dragon-owned UI use one stable virtual presentation grid. Resolution presets change output quality/size, not layout geometry.
- The app standard is 1280x720 fullscreen at launch, with a 1280x720 windowed fallback. Fullscreen/windowed controls must be app-wide rather than screen-specific.
- UI scale is an accessibility/readability control. It must not be used as a hidden resolution-specific layout fix.

## Menu And Options Screens

- Main menu rows must never overlap when the output preset is 320x240, 426x240, 480x240, 854x480, or 1280x720.
- HD output must not blindly scale Dragon UI to 3x when that makes panels dominate the screen. Keep the virtual layout stable and use output resolution for cleaner presentation.
- Menu shadows must come from motif shading or a clean panel treatment. Blocky layered-opacity effects are not acceptable.
- FPS and performance HUD must show the active output resolution when enabled.
- Fullscreen can be toggled with `F11` or `Alt+Enter`; `Alt+M` minimizes without changing gameplay/menu state.

## Shop Screens

- World composition and UI overlay are separate layers.
- Characters need stable ground contact, shadows, and consistent origin handling.
- Shop overlay must remain readable at every output preset without reflowing the whole scene just because output resolution changes.

## Verification

Visual work should add or update a verifier when a rule can be checked mechanically. Required recurring checks include:

- `main-menu-responsive-layout`
- `video-resolution-stable-virtual-layout`
- `video-hd-fullscreen-window-policy`
- `shop-overlay-responsive-layout`
- `shop-overlay-classic-full-layout`
- `shop-overlay-sd-layout`
- `shop-overlay-hd-layout`
- `world-texture-filter-selection`
- `ui-nearest-filter-selection`
