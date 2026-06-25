# Arena-Style Shop Hub

Status: Complete

## Goal

Add a Flying Dragon-inspired shop as a Dragon-only non-combat hub that reuses Arena presentation: 2.5D actor placement, depth projection, walk/idle actors, and an on-screen shopkeeper/NPC. This is a profile, item, and progression feature layered on top of the existing arena-style runtime, not a M.U.G.E.N character-file feature.

## Source References

- `docs/ENGINE_COMPLETION_ROADMAP.md`
- `docs/FEATURE_LEDGER.md`
- `docs/REGRESSION_CHECKLIST.md`
- `docs/DRAGON_EXTENSIONS.md`
- `engine/src/ArenaModeRuntime.h`
- `engine/src/DragonProgression.h`
- `engine/src/DragonProgression.cpp`
- `engine/src/FrontendFlow.h`
- `engine/src/ShopCatalog.h`
- `engine/src/ShopDemoCollision.h`
- `engine/src/ShopDemoPanelOverlay.h`
- `engine/src/ShopDemoRuntime.h`
- `engine/src/ShopDemoState.h`
- `engine/src/VerificationScenarioShop.cpp`
- `game/data/shop/README.md`
- `game/data/dragon.def`
- `game/save/progression.def`

## Scope

In scope:

- A Dragon-only shop route or room entry point.
- One shop room/stage that uses Arena-style 2.5D projection and actor draw ordering.
- One shopkeeper/NPC actor and one controllable local player actor.
- Buy, sell, equip, unequip, and cancel flows.
- Profile-owned currency, inventory, and item ownership.
- Guest browse/test behavior without persistent saves.
- Keyboard/controller navigation through existing canonical controls.
- Shop transaction feedback, item-detail panels, and confirmation prompts.
- Held directional movement in the shop room; walking should not require repeated taps.
- Shoulder/trigger tab switching for Buy/Sell/Equip.
- Equip target selection across available roster characters.
- Story enemy defeat XP/gold rewards that feed the same profile-owned progression economy.
- A solid counter collision footprint that blocks direct walk-through while preserving front and back aisles.
- A top/back route around the counter so the player can reach the other side without clipping through it.
- A non-combat run modifier using the existing depth-modifier binding (`Shift`/left trigger).
- Optional PNG art layers for the shop backdrop, counter-back layer, and counter-front layer, with documented prompts and a code-drawn fallback when assets are missing.
- Verifiers for room route, actor projection, profile persistence, Guest non-save behavior, and controls.

Out of scope:

- Editing M.U.G.E.N character files to add shop behavior.
- Applying Dragon item bonuses to live combat constants by default.
- Online/cloud profiles.
- Full campaign economy balancing.
- Multiple shopkeepers, branching shop inventories, crafting, gacha, or tournament/shop hybrid flows.
- Replacing Story or Arena result routing.

## Feature Slice

This should land as a complete feature slice, not as a menu stub. The first implementation should let a named profile enter a shop room, walk to a shopkeeper/counter, open buy/sell/equip UI, complete or cancel a transaction, persist owned items/currency, and leave without changing classic M.U.G.E.N fight behavior.

## Ownership

- `ShopRuntime` should own room state, actor positions, interaction focus, selected item, transaction state, and shop mode progression.
- `ShopCatalog` should load shop stock and item pricing from Dragon-owned data files under `game/data/`, reusing existing progression item definitions where possible.
- `ShopDemoPanelOverlay` should own buy/sell/equip panels, item details, currency labels, confirmation prompts, and feedback text.
- `ShopInteraction` should own player-to-shopkeeper/counter range checks and input-to-action routing.
- `DragonProgression` should remain the owner for profile, currency, inventory, item ownership, equipment slots, and persistence.
- Arena projection helpers or a shared projection module should own actor projection/draw-order math. Shop code should not duplicate fight/Arena projection logic.
- `App.cpp` should only perform thin route/update/render integration.
- `VerificationScenarioShop.cpp` should own focused shop verifiers.

## Implementation Checklist

- [x] Add Dragon shop route/state without changing Training, Single Player, VS, Arena fight, or Story fight routing.
- [x] Add shop runtime modules instead of implementing the shop directly in `App.cpp`.
- [x] Add one shop room/stage using Arena-style projection and non-combat actors.
- [x] Add one shopkeeper/NPC interaction point.
- [x] Add buy, sell, equip, unequip, and cancel flows.
- [x] Add profile-owned currency and transaction persistence through `DragonProgression`.
- [x] Keep Guest non-persistent and clearly labeled.
- [x] Add controller/keyboard navigation through the existing action mapping layer.
- [x] Support held movement in the room and shoulder/trigger tab switching in the shop UI.
- [x] Add Equip target selection so items can be equipped to a chosen roster character owned by the active profile.
- [x] Add item detail and transaction feedback UI.
- [x] Add per-enemy Story XP/gold rewards that feed the shop economy.
- [x] Block direct player movement through the counter while keeping front/back lanes and side routes navigable.
- [x] Add shop-room run movement through the existing depth-modifier binding.
- [x] Expand the shop room's walkable floor and verify the front aisle, top/back route, side connections, run speed, and counter collision in one deterministic room verifier.
- [x] Tune walk/run movement down by 10% after live Phase 1 evaluation and keep the verifier constants aligned.
- [x] Add compact scaled shop panel text, item-name/value columns, wrapped detail text, split metadata, and short footer labels so item descriptions, requirements, ownership, target, and controls do not run off the panel.
- [x] Extract the shop panel overlay out of `ShopDemoRuntime.h` and add denser Phase 2 item presentation: slot icons, selected-item effect summaries, compact detail geometry, and balance-aware confirmation/result text.
- [x] Add asset-ready shop room composition hooks for optional backdrop/counter PNG layers, plus prompt documentation for generating the needed shop art without committing bad placeholder images.
- [x] Add focused shop verifiers and update roadmap, ledger, and regression checklist records.

## Verification

Focused checks:

```powershell
cmake --build build --target dragon_mugen
build\dragon_mugen.exe --verify shop-route-entry
build\dragon_mugen.exe --verify shop-room-actor-projection
build\dragon_mugen.exe --verify shop-room-movement-collision
build\dragon_mugen.exe --verify shop-buy-sell-persistence
build\dragon_mugen.exe --verify shop-equip-profile-scope
build\dragon_mugen.exe --verify shop-guest-no-save
build\dragon_mugen.exe --verify shop-controller-keyboard-navigation
build\dragon_mugen.exe --verify shop-panel-text-fit
build\dragon_mugen.exe --verify dragon-progression-enemy-reward
```

Regression checks:

```powershell
build\dragon_mugen.exe --verify dragon-progression-level-items
build\dragon_mugen.exe --verify dragon-progression-player-profiles
build\dragon_mugen.exe --verify story-stage-clear
build\dragon_mugen.exe --verify arena-cpu-1
build\dragon_mugen.exe --verify roster-compatibility-smoke
python engine\tools\dev_check.py . --skip-build
python engine\tools\dev_check.py .
git diff --check
python tools\check_file_sizes.py
```

Manual smoke:

- Enter the shop from the accepted route.
- Confirm the player actor and shopkeeper draw in Arena-style projection/depth order.
- Walk to the counter/shopkeeper and open the shop UI.
- Buy an item, leave, re-enter, and confirm the item persists for the named profile.
- Sell an item and confirm currency/inventory update correctly.
- Equip/unequip an item and confirm the selected profile owns that state.
- Use Q/E, L1/R1, or L2/R2 to switch Buy/Sell/Equip.
- In Equip, use Left/Right to select the target character before confirming.
- Hold Shift or left trigger while moving to confirm faster shop-room movement.
- Confirm the tuned shop walk/run speed feels slower than Phase 1 without returning to tap-to-move behavior.
- Confirm Buy/Sell/Equip item rows use readable icon/name/value columns, descriptions/effects wrap without hiding key meaning, LV/slot/owned/target metadata stays visible, and confirmation/footer controls fit inside the shop panel.
- Confirm the built-in fallback shop room shows shelf bays, neon I.Chie branding, dragon accents, layered wall/floor bands, and a richer counter face even before optional PNG art is installed.
- Drop in generated `game/data/shop/i_chie_shop_backdrop.png`, `i_chie_shop_counter_back.png`, and `i_chie_shop_counter_front.png` and confirm the room switches from fallback geometry to layered art while preserving counter collision and front/back draw order.
- Confirm the player cannot walk through the counter body, but can go around it through the back/top aisle and side openings.
- Clear enemies in Story Mode and confirm each defeated enemy grants profile-owned XP/gold.
- Switch to Guest and confirm no persistent transaction is saved.
- Confirm Training, VS, Arena, and Story still load normal fights without item side effects.

## Done Means

- The shop is a usable Dragon-only hub, not a placeholder menu.
- The feature works for a named local profile and preserves Guest non-persistence.
- Buy/sell/equip state is persisted through `DragonProgression`.
- Story enemy defeats add profile-owned XP/gold for named profiles without saving rewards for Guest.
- The player and NPC are presented with Arena-style projection without enabling combat.
- No M.U.G.E.N character files are edited or required to support the shop.
- Item bonuses remain data-owned and are not applied to live classic combat unless a later Dragon mode explicitly opts in.
- Focused shop verifiers and listed regressions pass.
- The feature is recorded in the roadmap, ledger, and regression checklist.
