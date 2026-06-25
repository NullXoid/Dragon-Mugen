#pragma once

// Internal App.cpp implementation shard.
// Arena-style Dragon shop hub runtime.

namespace {

constexpr float kShopRoomLeft = -1120.0f;
constexpr float kShopRoomRight = 1120.0f;
constexpr float kShopPlayerMinX = -1040.0f;
constexpr float kShopPlayerMaxX = 1040.0f;
constexpr float kShopPlayerMinDepth = -82.0f;
constexpr float kShopPlayerMaxDepth = 118.0f;
constexpr float kShopkeeperX = 318.0f;
constexpr float kShopkeeperDepth = -6.0f;
constexpr float kShopCounterX = -126.0f;
constexpr float kShopCounterW = 660.0f;
constexpr float kShopCounterFrontDepth = 48.0f;
constexpr float kShopFloorBaseY = 166.0f;
constexpr float kShopkeeperTargetHeight = 75.0f;
constexpr float kShopPlayerPoseTargetHeight = 82.0f;
constexpr float kShopCounterSolidLeft = kShopCounterX - 34.0f;
constexpr float kShopCounterSolidRight = kShopCounterX + kShopCounterW + 34.0f;
constexpr float kShopCounterSolidBackDepth = -18.0f;
constexpr float kShopCounterSolidFrontDepth = 70.0f;
constexpr float kShopCounterCollisionEpsilon = 0.5f;
constexpr float kShopWalkSpeed = 3.105f;
constexpr float kShopDepthSpeed = 2.115f;
constexpr float kShopRunMultiplier = 1.7f;

float shopDemoClampCamera(const AppState& state, float targetX) {
    const float halfView = logicalWidthF(state) * 0.5f;
    const float minCamera = kShopRoomLeft + halfView;
    const float maxCamera = kShopRoomRight - halfView;
    if (minCamera > maxCamera) return 0.0f;
    return std::clamp(targetX, minCamera, maxCamera);
}

float shopDemoScreenX(const AppState& state, float worldX) {
    return screenCenterX(state) + worldX - state.shopDemo.cameraX;
}

float shopDemoCameraPan01(const AppState& state) {
    const float halfView = logicalWidthF(state) * 0.5f;
    const float minCamera = kShopRoomLeft + halfView;
    const float maxCamera = kShopRoomRight - halfView;
    if (maxCamera <= minCamera) {
        return 0.5f;
    }
    return std::clamp((state.shopDemo.cameraX - minCamera) / (maxCamera - minCamera), 0.0f, 1.0f);
}

float shopDemoBackdropPan01(const AppState& state) {
    return 0.5f + (shopDemoCameraPan01(state) - 0.5f) * 0.28f;
}

void setShopSpriteRenderStyle(TextureSprite& sprite) {
    if (!sprite.texture) {
        return;
    }
    SDL_SetTextureBlendMode(sprite.texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(sprite.texture, SDL_SCALEMODE_LINEAR);
}

float shopSpriteScaleForHeight(const TextureSprite& sprite, float targetHeight) {
    if (sprite.height <= 0 || targetHeight <= 0.0f) {
        return 1.0f;
    }
    return targetHeight / static_cast<float>(sprite.height);
}

void drawShopTextureCover(SDL_Renderer* renderer, const TextureSprite& sprite, const SDL_FRect& dst, float pan01) {
    if (!sprite.texture || sprite.width <= 0 || sprite.height <= 0 || dst.w <= 0.0f || dst.h <= 0.0f) {
        return;
    }
    const float texW = static_cast<float>(sprite.width);
    const float texH = static_cast<float>(sprite.height);
    const float texAspect = texW / texH;
    const float dstAspect = dst.w / dst.h;
    float srcW = texW;
    float srcH = texH;
    if (texAspect > dstAspect) {
        srcW = texH * dstAspect;
    } else {
        srcH = texW / dstAspect;
    }
    const float srcX = std::max(0.0f, texW - srcW) * std::clamp(pan01, 0.0f, 1.0f);
    const float srcY = std::max(0.0f, texH - srcH) * 0.5f;
    SDL_FRect src{srcX, srcY, srcW, srcH};
    SDL_RenderTexture(renderer, sprite.texture, &src, &dst);
}

float shopDemoFloorY(float depthZ) {
    return kShopFloorBaseY + depthZ * 0.38f;
}

shop_demo::ShopCounterCollisionBounds shopDemoCounterCollisionBounds() {
    return {
        kShopPlayerMinX,
        kShopPlayerMaxX,
        kShopPlayerMinDepth,
        kShopPlayerMaxDepth,
        kShopCounterSolidLeft,
        kShopCounterSolidRight,
        kShopCounterSolidBackDepth,
        kShopCounterSolidFrontDepth,
        kShopCounterCollisionEpsilon,
    };
}

void shopDemoWorldRect(SDL_Renderer* renderer, const AppState& state, float worldX, float y, float w, float h) {
    fillRect(renderer, shopDemoScreenX(state, worldX), y, w, h);
}

void shopDemoWorldTextCentered(
    SDL_Renderer* renderer,
    const AppState& state,
    float worldCenterX,
    float y,
    const std::string& text) {
    debugTextCentered(renderer, shopDemoScreenX(state, worldCenterX), y, text);
}

void drawShopDemoFallbackShelfBay(
    SDL_Renderer* renderer,
    const AppState& state,
    float worldX,
    float y,
    float w,
    float h,
    Uint8 accentR,
    Uint8 accentG,
    Uint8 accentB) {
    setColor(renderer, 7, 10, 15, 238);
    shopDemoWorldRect(renderer, state, worldX, y, w, h);
    setColor(renderer, 22, 29, 39, 232);
    shopDemoWorldRect(renderer, state, worldX + 7.0f, y + 8.0f, w - 14.0f, h - 14.0f);
    setColor(renderer, accentR, accentG, accentB, 220);
    shopDemoWorldRect(renderer, state, worldX + 12.0f, y + 14.0f, w - 24.0f, 8.0f);
    setColor(renderer, 214, 173, 76, 225);
    shopDemoWorldRect(renderer, state, worldX + 14.0f, y + h - 16.0f, w - 28.0f, 2.0f);

    for (int row = 0; row < 2; ++row) {
        const float shelfY = y + 35.0f + static_cast<float>(row) * 25.0f;
        setColor(renderer, 96, 70, 45, 205);
        shopDemoWorldRect(renderer, state, worldX + 20.0f, shelfY, w - 40.0f, 4.0f);
        for (int item = 0; item < 5; ++item) {
            const float ix = worldX + 28.0f + static_cast<float>(item) * ((w - 58.0f) / 4.0f);
            const Uint8 glow = static_cast<Uint8>(80 + item * 26);
            setColor(renderer, item % 2 == 0 ? accentR : glow, item % 2 == 0 ? accentG : 74, item % 2 == 0 ? accentB : 112, 210);
            shopDemoWorldRect(renderer, state, ix, shelfY - 12.0f, 7.0f, 11.0f);
            setColor(renderer, 225, 203, 126, 180);
            shopDemoWorldRect(renderer, state, ix + 1.0f, shelfY - 14.0f, 5.0f, 2.0f);
        }
    }
}

void drawShopDemoFallbackDragonMark(SDL_Renderer* renderer, const AppState& state, float worldX, float y) {
    setColor(renderer, 86, 42, 128, 150);
    shopDemoWorldRect(renderer, state, worldX, y + 10.0f, 58.0f, 8.0f);
    shopDemoWorldRect(renderer, state, worldX + 12.0f, y, 8.0f, 36.0f);
    shopDemoWorldRect(renderer, state, worldX + 38.0f, y + 2.0f, 8.0f, 32.0f);
    setColor(renderer, 142, 82, 216, 170);
    shopDemoWorldRect(renderer, state, worldX + 8.0f, y + 14.0f, 42.0f, 3.0f);
    shopDemoWorldRect(renderer, state, worldX + 20.0f, y + 7.0f, 18.0f, 3.0f);
    shopDemoWorldRect(renderer, state, worldX + 23.0f, y + 22.0f, 18.0f, 3.0f);
}

void ensureShopDemoProgressionLoaded(AppState& state) {
    if (!state.progression.loaded) {
        loadProgressionState(state);
    }
    if (state.progression.savePath.empty()) {
        state.progression.savePath = dragonProgressionSavePath(state.gameRoot);
    }
}

void saveShopDemoProgression(AppState& state) {
    try {
        saveDragonProgressionSave(
            state.progression.savePath.empty() ? dragonProgressionSavePath(state.gameRoot) : state.progression.savePath,
            state.progression.save);
    } catch (const std::exception& ex) {
        SDL_Log("Dragon shop save failed: %s", ex.what());
    }
}

std::string shopDemoProfileId(const AppState& state) {
    return dragonProgressionPlayerProfileId(state.progression.save, 0);
}

std::string shopDemoProfileName(const AppState& state) {
    return dragonProgressionPlayerProfileDisplayName(state.progression.save, 0);
}

const CharacterSlot* shopDemoSelectedCharacter(const AppState& state) {
    if (const CharacterSlot* slot = sessionP1CharacterSlot(state.selection)) {
        return slot;
    }
    return characterSlotAt(state.selection, state.selection.selectedCharacter);
}

std::string shopDemoCharacterId(const AppState& state) {
    if (const CharacterSlot* slot = shopDemoSelectedCharacter(state)) {
        return slot->id;
    }
    return "kfm";
}

std::string shopDemoCharacterName(const AppState& state) {
    if (const CharacterSlot* slot = shopDemoSelectedCharacter(state)) {
        return slot->displayName.empty() ? slot->id : slot->displayName;
    }
    return "Kung Fu Man";
}

int shopDemoCharacterCount(const AppState& state) {
    return std::max(1, static_cast<int>(state.selection.characters.size()));
}

const CharacterSlot* shopDemoTargetCharacter(const AppState& state) {
    if (!state.selection.characters.empty()) {
        const int index = std::clamp(
            state.shopDemo.selectedEquipCharacter,
            0,
            static_cast<int>(state.selection.characters.size()) - 1);
        return &state.selection.characters[static_cast<size_t>(index)];
    }
    return shopDemoSelectedCharacter(state);
}

std::string shopDemoTargetCharacterId(const AppState& state) {
    if (const CharacterSlot* slot = shopDemoTargetCharacter(state)) {
        return slot->id;
    }
    return shopDemoCharacterId(state);
}

std::string shopDemoTargetCharacterName(const AppState& state) {
    if (const CharacterSlot* slot = shopDemoTargetCharacter(state)) {
        return slot->displayName.empty() ? slot->id : slot->displayName;
    }
    return shopDemoCharacterName(state);
}

std::vector<ShopCatalogEntry> shopDemoCatalog(const AppState& state) {
    return buildDefaultShopCatalog(state.progression.data);
}

int shopDemoCatalogCount(const AppState& state) {
    return static_cast<int>(shopDemoCatalog(state).size());
}

int shopDemoOwnedQuantity(const AppState& state, std::string_view itemId) {
    const auto entry = inventoryEntryForProfile(state.progression.save, shopDemoProfileId(state), itemId);
    return entry ? std::max(0, entry->quantity) : 0;
}

bool shopDemoItemEquipped(const AppState& state, std::string_view itemId) {
    return isDragonProgressionItemEquippedForProfile(
        state.progression.save,
        shopDemoProfileId(state),
        shopDemoTargetCharacterId(state),
        itemId);
}

const char* shopDemoPanelLabel(ShopPanelMode mode) {
    switch (mode) {
    case ShopPanelMode::Buy: return "BUY";
    case ShopPanelMode::Sell: return "SELL";
    case ShopPanelMode::Equip: return "EQUIP";
    default: return "BUY";
    }
}

std::string shopDemoActionLabel(ShopPendingAction action) {
    switch (action) {
    case ShopPendingAction::Buy: return "BUY";
    case ShopPendingAction::Sell: return "SELL";
    case ShopPendingAction::Equip: return "EQUIP";
    case ShopPendingAction::Unequip: return "UNEQUIP";
    case ShopPendingAction::None:
    default: return "CONFIRM";
    }
}

void shopDemoSetNotice(AppState& state, std::string text, int ticks = 105) {
    state.shopDemo.noticeText = std::move(text);
    state.shopDemo.noticeTicks = ticks;
}

void shopDemoSetTransaction(AppState& state, std::string title, std::string detail, int ticks = 135) {
    state.shopDemo.transactionTitle = std::move(title);
    state.shopDemo.transactionDetail = std::move(detail);
    state.shopDemo.transactionTicks = ticks;
    shopDemoSetNotice(state, state.shopDemo.transactionTitle, ticks);
}

void playShopTransactionSound(AppState& state) {
    playMenuCursorDoneSound(state);
}

void ensureShopDemoAssets(SDL_Renderer* renderer, AppState& state) {
    if (state.shopDemo.assetsLoaded) {
        return;
    }
    const auto loadOptionalShopSprite = [&](const std::filesystem::path& relativePath) {
        TextureSprite sprite = std::filesystem::exists(state.gameRoot / relativePath)
            ? loadUiPngSprite(renderer, state.gameRoot, relativePath)
            : TextureSprite{};
        setShopSpriteRenderStyle(sprite);
        return sprite;
    };
    state.shopDemo.shopBackdrop = loadOptionalShopSprite("data/shop/i_chie_shop_backdrop.png");
    state.shopDemo.shopCounterBack = loadOptionalShopSprite("data/shop/i_chie_shop_counter_back.png");
    state.shopDemo.shopCounterFront = loadOptionalShopSprite("data/shop/i_chie_shop_counter_front.png");
    state.shopDemo.shopkeeperPose =
        loadUiPngSprite(renderer, state.gameRoot, "chars/I.Chie/I.Chie_shopkeeper_pose.png");
    setShopSpriteRenderStyle(state.shopDemo.shopkeeperPose);
    state.shopDemo.shopkeeperPose.axisX = state.shopDemo.shopkeeperPose.width / 2;
    state.shopDemo.shopkeeperPose.axisY = std::max(0, state.shopDemo.shopkeeperPose.height - 2);
    state.shopDemo.shopPlayerPose = loadOptionalShopSprite("data/shop/shop_player_back_pose.png");
    state.shopDemo.shopPlayerPose.axisX = state.shopDemo.shopPlayerPose.width / 2;
    state.shopDemo.shopPlayerPose.axisY = std::max(0, state.shopDemo.shopPlayerPose.height - 2);
    const char* walkFramePaths[] = {
        "data/shop/shop_player_walk_0.png",
        "data/shop/shop_player_walk_1.png",
        "data/shop/shop_player_walk_2.png",
        "data/shop/shop_player_walk_3.png",
        "data/shop/shop_player_walk_4.png",
        "data/shop/shop_player_walk_5.png",
        "data/shop/shop_player_walk_6.png",
        "data/shop/shop_player_walk_7.png",
    };
    for (std::size_t i = 0; i < state.shopDemo.shopPlayerWalkFrames.size(); ++i) {
        TextureSprite& frame = state.shopDemo.shopPlayerWalkFrames[i];
        frame = loadOptionalShopSprite(walkFramePaths[i]);
        frame.axisX = frame.width / 2;
        frame.axisY = std::max(0, frame.height - 2);
    }
    state.shopDemo.trainingWeightIcon = loadOptionalShopSprite("data/shop/items/training_weight.png");
    state.shopDemo.guardCharmIcon = loadOptionalShopSprite("data/shop/items/guard_charm.png");
    state.shopDemo.dragonSashIcon = loadOptionalShopSprite("data/shop/items/dragon_sash.png");
    state.shopDemo.assetsLoaded = true;
}

void resetShopDemoRoom(AppState& state) {
    ensureShopDemoProgressionLoaded(state);
    state.shopDemo.shopOpen = false;
    state.shopDemo.confirmOpen = false;
    state.shopDemo.playerX = kShopkeeperX - 118.0f;
    state.shopDemo.playerDepthZ = kShopCounterSolidFrontDepth + 18.0f;
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.playerX);
    state.shopDemo.selectedItem = 0;
    state.shopDemo.selectedEquipCharacter = std::clamp(
        state.selection.sessionSlots.p1Character,
        0,
        std::max(0, static_cast<int>(state.selection.characters.size()) - 1));
    state.shopDemo.panelMode = ShopPanelMode::Buy;
    state.shopDemo.pendingAction = ShopPendingAction::None;
    state.shopDemo.pendingItemId.clear();
    state.shopDemo.noticeText.clear();
    state.shopDemo.noticeTicks = 0;
    state.shopDemo.transactionTitle.clear();
    state.shopDemo.transactionDetail.clear();
    state.shopDemo.transactionTicks = 0;
    state.shopDemo.tabInputHeld = false;
    state.shopDemo.playerMoving = false;
    state.shopDemo.playerFacingLeft = false;
    state.shopDemo.playerWalkFrame = 0.0f;
    state.frontend.screenFrame = 0;
}

void enterShopDemo(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoAssets(renderer, state);
    resetShopDemoRoom(state);
    state.frontend.screen = Screen::ShopDemo;
}

bool shopDemoNearCounter(const AppState& state) {
    const bool nearCounterX = state.shopDemo.playerX >= kShopCounterX - 96.0f
        && state.shopDemo.playerX <= kShopCounterX + kShopCounterW + 96.0f;
    const bool atFrontAccess = state.shopDemo.playerDepthZ >= kShopCounterSolidFrontDepth + 2.0f;
    const bool atBackAccess = state.shopDemo.playerDepthZ <= kShopCounterSolidBackDepth - 2.0f;
    return nearCounterX && (atFrontAccess || atBackAccess);
}

void shopDemoClampSelection(AppState& state) {
    const int count = shopDemoCatalogCount(state);
    state.shopDemo.selectedItem = count <= 0
        ? 0
        : std::clamp(state.shopDemo.selectedItem, 0, count - 1);
}

FrontendKey shopDemoFrontendKeyFromSdl(SDL_Keycode key) {
    switch (key) {
    case SDLK_ESCAPE: return FrontendKey::Escape;
    case SDLK_UP: return FrontendKey::Up;
    case SDLK_DOWN: return FrontendKey::Down;
    case SDLK_LEFT: return FrontendKey::Left;
    case SDLK_RIGHT: return FrontendKey::Right;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE: return FrontendKey::Accept;
    default: return FrontendKey::Other;
    }
}

void shopDemoMovePlayer(AppState& state, float dx, float dz);
void shopDemoCyclePanel(AppState& state, int direction);

void updateShopDemo(AppState& state) {
    if (state.frontend.screen != Screen::ShopDemo) {
        return;
    }
    if (state.shopDemo.noticeTicks > 0) {
        --state.shopDemo.noticeTicks;
        if (state.shopDemo.noticeTicks <= 0) {
            state.shopDemo.noticeText.clear();
        }
    }
    if (state.shopDemo.transactionTicks > 0) {
        --state.shopDemo.transactionTicks;
        if (state.shopDemo.transactionTicks <= 0) {
            state.shopDemo.transactionTitle.clear();
            state.shopDemo.transactionDetail.clear();
        }
    }

    const bool* keys = gFightInputOverride ? nullptr : SDL_GetKeyboardState(nullptr);
    const FighterInputState input = gFightInputOverride && gFightInputOverride->p1
        ? *gFightInputOverride->p1
        : collectMappedFighterInput(keys, controlProfileForPlayer(state, 0), assignedGamepad(state, 0));
    if (!state.shopDemo.shopOpen) {
        const float speedScale = input.depthModifier ? kShopRunMultiplier : 1.0f;
        const float dx = ((input.right ? kShopWalkSpeed : 0.0f) - (input.left ? kShopWalkSpeed : 0.0f)) * speedScale;
        const float dz = ((input.down ? kShopDepthSpeed : 0.0f) - (input.up ? kShopDepthSpeed : 0.0f)) * speedScale;
        const bool moving = dx != 0.0f || dz != 0.0f;
        state.shopDemo.playerMoving = moving;
        if (dx < 0.0f) {
            state.shopDemo.playerFacingLeft = true;
        } else if (dx > 0.0f) {
            state.shopDemo.playerFacingLeft = false;
        }
        if (moving) {
            shopDemoMovePlayer(state, dx, dz);
            state.shopDemo.playerWalkFrame += input.depthModifier ? 0.24f : 0.16f;
            const float frameCount = static_cast<float>(state.shopDemo.shopPlayerWalkFrames.size());
            while (state.shopDemo.playerWalkFrame >= frameCount) {
                state.shopDemo.playerWalkFrame -= frameCount;
            }
        } else {
            state.shopDemo.playerWalkFrame = 0.0f;
        }
        state.shopDemo.tabInputHeld = false;
    } else if (!state.shopDemo.confirmOpen) {
        state.shopDemo.playerMoving = false;
        state.shopDemo.playerWalkFrame = 0.0f;
        const GamepadDevice* pad = assignedGamepad(state, 0);
        const bool leftShoulder = pad && pad->handle
            && SDL_GetGamepadButton(pad->handle, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        const bool rightShoulder = pad && pad->handle
            && SDL_GetGamepadButton(pad->handle, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
        const bool leftTrigger = pad && pad->handle
            && SDL_GetGamepadAxis(pad->handle, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 12000;
        const bool rightTrigger = pad && pad->handle
            && SDL_GetGamepadAxis(pad->handle, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 12000;
        const bool leftTab = leftShoulder || leftTrigger;
        const bool rightTab = rightShoulder || rightTrigger;
        if ((leftTab || rightTab) && !state.shopDemo.tabInputHeld) {
            shopDemoCyclePanel(state, rightTab ? 1 : -1);
            playMenuCursorMoveSound(state);
        }
        state.shopDemo.tabInputHeld = leftTab || rightTab;
    } else {
        state.shopDemo.playerMoving = false;
        state.shopDemo.playerWalkFrame = 0.0f;
        state.shopDemo.tabInputHeld = false;
    }
    shopDemoClampSelection(state);
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.playerX);
}

void shopDemoMovePlayer(AppState& state, float dx, float dz) {
    float oldX = std::clamp(state.shopDemo.playerX, kShopPlayerMinX, kShopPlayerMaxX);
    float oldDepthZ = std::clamp(state.shopDemo.playerDepthZ, kShopPlayerMinDepth, kShopPlayerMaxDepth);
    const auto collisionBounds = shopDemoCounterCollisionBounds();
    shop_demo::shopDemoSnapOutOfCounterSolid(collisionBounds, oldX, oldDepthZ);

    float nextX = std::clamp(oldX + dx, kShopPlayerMinX, kShopPlayerMaxX);
    float nextDepthZ = std::clamp(oldDepthZ + dz, kShopPlayerMinDepth, kShopPlayerMaxDepth);
    shop_demo::shopDemoResolveCounterCollision(collisionBounds, oldX, oldDepthZ, dx, dz, nextX, nextDepthZ);

    state.shopDemo.playerX = nextX;
    state.shopDemo.playerDepthZ = nextDepthZ;
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.playerX);
}

void shopDemoCyclePanel(AppState& state, int direction) {
    int index = 0;
    if (state.shopDemo.panelMode == ShopPanelMode::Sell) index = 1;
    if (state.shopDemo.panelMode == ShopPanelMode::Equip) index = 2;
    index = (index + direction + 3) % 3;
    state.shopDemo.panelMode = index == 0 ? ShopPanelMode::Buy : index == 1 ? ShopPanelMode::Sell : ShopPanelMode::Equip;
    state.shopDemo.confirmOpen = false;
    state.shopDemo.pendingAction = ShopPendingAction::None;
    state.shopDemo.pendingItemId.clear();
    shopDemoSetNotice(state, std::string(shopDemoPanelLabel(state.shopDemo.panelMode)) + " MODE", 45);
}

void shopDemoCycleEquipCharacter(AppState& state, int direction) {
    const int count = shopDemoCharacterCount(state);
    if (count <= 1 || direction == 0) {
        return;
    }
    state.shopDemo.selectedEquipCharacter =
        (state.shopDemo.selectedEquipCharacter + direction + count) % count;
    state.shopDemo.confirmOpen = false;
    state.shopDemo.pendingAction = ShopPendingAction::None;
    state.shopDemo.pendingItemId.clear();
    shopDemoSetNotice(state, "TARGET " + uppercaseCopy(shopDemoTargetCharacterName(state)), 60);
}

std::optional<ShopCatalogEntry> shopDemoSelectedEntry(const AppState& state) {
    const auto catalog = shopDemoCatalog(state);
    if (catalog.empty()) {
        return std::nullopt;
    }
    const int index = std::clamp(state.shopDemo.selectedItem, 0, static_cast<int>(catalog.size()) - 1);
    return catalog[static_cast<size_t>(index)];
}

ShopPendingAction shopDemoActionForSelection(const AppState& state, const ShopCatalogEntry& entry) {
    switch (state.shopDemo.panelMode) {
    case ShopPanelMode::Buy:
        return ShopPendingAction::Buy;
    case ShopPanelMode::Sell:
        return ShopPendingAction::Sell;
    case ShopPanelMode::Equip:
        return shopDemoItemEquipped(state, entry.itemId) ? ShopPendingAction::Unequip : ShopPendingAction::Equip;
    default:
        return ShopPendingAction::None;
    }
}

void shopDemoBeginConfirm(AppState& state) {
    const auto entry = shopDemoSelectedEntry(state);
    if (!entry) {
        shopDemoSetNotice(state, "NO SHOP STOCK");
        return;
    }
    if (isDragonProgressionGuestProfile(shopDemoProfileId(state))) {
        shopDemoSetNotice(state, "GUEST CAN BROWSE ONLY");
        return;
    }
    state.shopDemo.pendingAction = shopDemoActionForSelection(state, *entry);
    state.shopDemo.pendingItemId = entry->itemId;
    state.shopDemo.confirmOpen = true;
    shopDemoSetNotice(state, shopDemoActionLabel(state.shopDemo.pendingAction) + " " + entry->name + "?", 120);
}

bool shopDemoApplyConfirmedTransaction(AppState& state) {
    ensureShopDemoProgressionLoaded(state);
    const auto entry = shopDemoSelectedEntry(state);
    if (!entry || entry->itemId != state.shopDemo.pendingItemId) {
        shopDemoSetNotice(state, "ITEM CHANGED");
        return false;
    }

    const std::string profileId = shopDemoProfileId(state);
    if (isDragonProgressionGuestProfile(profileId)) {
        shopDemoSetNotice(state, "GUEST CAN BROWSE ONLY");
        return false;
    }
    const std::string characterId = shopDemoTargetCharacterId(state);
    const std::string characterName = shopDemoTargetCharacterName(state);
    const auto stats = effectiveDragonProgressionStatsForProfile(
        state.progression.data,
        state.progression.save,
        profileId,
        characterId);
    const int owned = shopDemoOwnedQuantity(state, entry->itemId);

    switch (state.shopDemo.pendingAction) {
    case ShopPendingAction::Buy:
        if (stats.level < entry->requiredLevel) {
            shopDemoSetNotice(state, "LV " + std::to_string(entry->requiredLevel) + " REQUIRED");
            return false;
        }
        if (!spendDragonProgressionGoldForProfile(state.progression.save, profileId, entry->price)) {
            shopDemoSetNotice(state, "NOT ENOUGH GOLD");
            return false;
        }
        grantDragonProgressionItemForProfile(state.progression.save, profileId, entry->itemId, 1);
        saveShopDemoProgression(state);
        shopDemoSetTransaction(
            state,
            "PURCHASED",
            entry->name + "  -G" + std::to_string(entry->price)
                + "  BAL G" + std::to_string(dragonProgressionGoldForProfile(state.progression.save, profileId)));
        return true;
    case ShopPendingAction::Sell:
        if (owned <= 0 || !removeDragonProgressionItemForProfile(state.progression.save, profileId, entry->itemId, 1)) {
            shopDemoSetNotice(state, "NONE TO SELL");
            return false;
        }
        if (owned <= 1) {
            unequipDragonProgressionItemForProfile(state.progression.save, profileId, characterId, entry->itemId);
        }
        addDragonProgressionGoldForProfile(state.progression.save, profileId, entry->sellPrice);
        saveShopDemoProgression(state);
        shopDemoSetTransaction(
            state,
            "SOLD",
            entry->name + "  +G" + std::to_string(entry->sellPrice)
                + "  BAL G" + std::to_string(dragonProgressionGoldForProfile(state.progression.save, profileId)));
        return true;
    case ShopPendingAction::Equip: {
        std::string reason;
        if (!equipDragonProgressionItemForProfile(
                state.progression.data,
                state.progression.save,
                profileId,
                characterId,
                entry->itemId,
                &reason)) {
            shopDemoSetNotice(state, uppercaseCopy(reason));
            return false;
        }
        saveShopDemoProgression(state);
        shopDemoSetTransaction(state, "EQUIPPED", entry->name + " TO " + characterName);
        return true;
    }
    case ShopPendingAction::Unequip:
        if (!unequipDragonProgressionItemForProfile(state.progression.save, profileId, characterId, entry->itemId)) {
            shopDemoSetNotice(state, "NOT EQUIPPED");
            return false;
        }
        saveShopDemoProgression(state);
        shopDemoSetTransaction(state, "UNEQUIPPED", entry->name + " FROM " + characterName);
        return true;
    case ShopPendingAction::None:
    default:
        shopDemoSetNotice(state, "NO ACTION");
        return false;
    }
}

void handleShopDemoKey(SDL_Renderer* renderer, AppState& state, SDL_Keycode key) {
    ensureShopDemoAssets(renderer, state);
    ensureShopDemoProgressionLoaded(state);
    const FrontendKey frontendKey = shopDemoFrontendKeyFromSdl(key);

    if (state.shopDemo.shopOpen) {
        if (state.shopDemo.confirmOpen) {
            if (frontendKey == FrontendKey::Escape) {
                state.shopDemo.confirmOpen = false;
                state.shopDemo.pendingAction = ShopPendingAction::None;
                state.shopDemo.pendingItemId.clear();
                shopDemoSetNotice(state, "CANCELLED", 45);
                playMenuCancelSound(state);
                return;
            }
            if (frontendKey == FrontendKey::Accept) {
                const bool applied = shopDemoApplyConfirmedTransaction(state);
                state.shopDemo.confirmOpen = false;
                state.shopDemo.pendingAction = ShopPendingAction::None;
                state.shopDemo.pendingItemId.clear();
                if (applied) {
                    playShopTransactionSound(state);
                } else {
                    playMenuCancelSound(state);
                }
                return;
            }
            return;
        }

        if (frontendKey == FrontendKey::Escape) {
            state.shopDemo.shopOpen = false;
            playMenuCancelSound(state);
            return;
        }
        if (key == SDLK_Q) {
            shopDemoCyclePanel(state, -1);
            playMenuCursorMoveSound(state);
            return;
        }
        if (key == SDLK_E) {
            shopDemoCyclePanel(state, 1);
            playMenuCursorMoveSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Left) {
            if (state.shopDemo.panelMode == ShopPanelMode::Equip) {
                shopDemoCycleEquipCharacter(state, -1);
            } else {
                shopDemoCyclePanel(state, -1);
            }
            playMenuCursorMoveSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Right) {
            if (state.shopDemo.panelMode == ShopPanelMode::Equip) {
                shopDemoCycleEquipCharacter(state, 1);
            } else {
                shopDemoCyclePanel(state, 1);
            }
            playMenuCursorMoveSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Up || key == SDLK_W) {
            const int count = std::max(1, shopDemoCatalogCount(state));
            state.shopDemo.selectedItem = (state.shopDemo.selectedItem + count - 1) % count;
            playMenuCursorMoveSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Down || key == SDLK_S) {
            const int count = std::max(1, shopDemoCatalogCount(state));
            state.shopDemo.selectedItem = (state.shopDemo.selectedItem + 1) % count;
            playMenuCursorMoveSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Accept) {
            shopDemoBeginConfirm(state);
            playMenuCursorDoneSound(state);
            return;
        }
        return;
    }

    if (frontendKey == FrontendKey::Escape) {
        state.frontend.screen = Screen::ModeSelect;
        playMenuCancelSound(state);
        return;
    }
    if (frontendKey == FrontendKey::Accept) {
        if (shopDemoNearCounter(state)) {
            state.shopDemo.shopOpen = true;
            state.shopDemo.noticeTicks = 0;
            state.shopDemo.noticeText.clear();
            playMenuCursorDoneSound(state);
        } else {
            shopDemoSetNotice(state, "MOVE CLOSER TO I.CHIE", 60);
            playMenuCancelSound(state);
        }
        return;
    }

    (void)frontendKey;
}

void drawShopDemoFloor(SDL_Renderer* renderer, const AppState& state) {
    const float width = logicalWidthF(state);
    if (state.shopDemo.shopBackdrop.texture) {
        SDL_FRect dst{
            0.0f,
            0.0f,
            width,
            static_cast<float>(kLogicalHeight),
        };
        drawShopTextureCover(renderer, state.shopDemo.shopBackdrop, dst, shopDemoBackdropPan01(state));
        return;
    }

    setColor(renderer, 8, 10, 15);
    fillRect(renderer, 0, 0, width, static_cast<float>(kLogicalHeight));

    setColor(renderer, 14, 19, 27);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 0, kShopRoomRight - kShopRoomLeft, 42);
    setColor(renderer, 32, 24, 33);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 42, kShopRoomRight - kShopRoomLeft, 32);
    setColor(renderer, 20, 20, 24);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 74, kShopRoomRight - kShopRoomLeft, 50);
    setColor(renderer, 47, 36, 27);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 124, kShopRoomRight - kShopRoomLeft, 116);

    setColor(renderer, 28, 36, 48, 210);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 31, kShopRoomRight - kShopRoomLeft, 4);
    setColor(renderer, 199, 153, 70, 205);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 82, kShopRoomRight - kShopRoomLeft, 2);
    setColor(renderer, 62, 178, 158, 150);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 54, kShopRoomRight - kShopRoomLeft, 7);
    setColor(renderer, 28, 20, 28, 178);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 66, kShopRoomRight - kShopRoomLeft, 26);

    const float camera = state.shopDemo.cameraX;
    for (int i = 0; i < 11; ++i) {
        const float y = 131.0f + static_cast<float>(i) * 10.0f;
        const Uint8 shade = static_cast<Uint8>(46 + (i % 2) * 12);
        setColor(renderer, shade, static_cast<Uint8>(34 + (i % 2) * 8), 24, 188);
        fillRect(renderer, 0, y, width, 1.0f);
    }
    for (int i = -12; i <= 12; ++i) {
        const float x = screenCenterX(state) + static_cast<float>(i) * 56.0f - std::fmod(camera, 56.0f);
        setColor(renderer, 72, 52, 32, 120);
        fillRect(renderer, x, 126, 1.0f, 114.0f);
    }
    for (int i = -12; i <= 12; ++i) {
        const float x = screenCenterX(state) + static_cast<float>(i) * 112.0f - std::fmod(camera * 0.55f, 112.0f);
        setColor(renderer, 216, 176, 92, 28);
        fillRect(renderer, x, 126, 42.0f, 114.0f);
    }

    setColor(renderer, 112, 80, 45);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 122, kShopRoomRight - kShopRoomLeft, 3);
    setColor(renderer, 15, 20, 27, 175);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 126, kShopRoomRight - kShopRoomLeft, 6);
}

void drawShopDemoCounterBackArt(SDL_Renderer* renderer, const AppState& state) {
    if (!state.shopDemo.shopCounterBack.texture) {
        return;
    }
    SDL_FRect dst{
        shopDemoScreenX(state, kShopCounterX - 36.0f),
        116.0f,
        kShopCounterW + 72.0f,
        56.0f,
    };
    SDL_RenderTexture(renderer, state.shopDemo.shopCounterBack.texture, nullptr, &dst);
}

void drawShopDemoBackdropProps(SDL_Renderer* renderer, const AppState& state) {
    if (state.shopDemo.shopBackdrop.texture) {
        drawShopDemoCounterBackArt(renderer, state);
        return;
    }

    drawShopDemoFallbackShelfBay(renderer, state, -768.0f, 31.0f, 252.0f, 86.0f, 92, 196, 176);
    drawShopDemoFallbackShelfBay(renderer, state, -466.0f, 28.0f, 270.0f, 90.0f, 138, 82, 216);
    drawShopDemoFallbackShelfBay(renderer, state, 36.0f, 30.0f, 396.0f, 92.0f, 68, 178, 158);
    drawShopDemoFallbackShelfBay(renderer, state, 492.0f, 30.0f, 300.0f, 88.0f, 218, 176, 82);

    drawShopDemoFallbackDragonMark(renderer, state, -312.0f, 54.0f);
    drawShopDemoFallbackDragonMark(renderer, state, 448.0f, 54.0f);

    setColor(renderer, 118, 226, 212);
    shopDemoWorldTextCentered(renderer, state, kShopCounterX + kShopCounterW * 0.5f, 43.0f, "I.CHIE");
    setColor(renderer, 134, 84, 220, 185);
    shopDemoWorldRect(renderer, state, kShopCounterX + 76.0f, 60.0f, kShopCounterW - 152.0f, 3.0f);
    setColor(renderer, 210, 164, 78, 210);
    shopDemoWorldRect(renderer, state, kShopCounterX + 112.0f, 76.0f, kShopCounterW - 224.0f, 3.0f);

    setColor(renderer, 10, 13, 18, 240);
    shopDemoWorldRect(renderer, state, kShopCounterX - 42.0f, 131, kShopCounterW + 84.0f, 51);
    setColor(renderer, 42, 35, 31, 235);
    shopDemoWorldRect(renderer, state, kShopCounterX - 24.0f, 136, kShopCounterW + 48.0f, 39);
    setColor(renderer, 70, 47, 32, 232);
    shopDemoWorldRect(renderer, state, kShopCounterX, 139, kShopCounterW, 32);
    setColor(renderer, 58, 156, 136, 220);
    shopDemoWorldRect(renderer, state, kShopCounterX + 20.0f, 142, kShopCounterW - 40.0f, 6);
    setColor(renderer, 116, 80, 48, 230);
    shopDemoWorldRect(renderer, state, kShopCounterX + 48.0f, 157, kShopCounterW - 96.0f, 6);
    setColor(renderer, 224, 190, 92, 230);
    shopDemoWorldRect(renderer, state, kShopCounterX + 8.0f, 132, kShopCounterW - 16.0f, 2);
    shopDemoWorldRect(renderer, state, kShopCounterX + 26.0f, 177, kShopCounterW - 52.0f, 2);
    drawShopDemoCounterBackArt(renderer, state);
}

void drawShopDemoPlayer(SDL_Renderer* renderer, const AppState& state) {
    const float sx = shopDemoScreenX(state, state.shopDemo.playerX);
    const float sy = shopDemoFloorY(state.shopDemo.playerDepthZ);
    const TextureSprite* playerSprite = &state.shopDemo.shopPlayerPose;
    SDL_FlipMode playerFlip = SDL_FLIP_NONE;
    if (state.shopDemo.playerMoving) {
        const int frameCount = static_cast<int>(state.shopDemo.shopPlayerWalkFrames.size());
        const int frameIndex = std::clamp(static_cast<int>(state.shopDemo.playerWalkFrame), 0, frameCount - 1);
        const TextureSprite& walkFrame = state.shopDemo.shopPlayerWalkFrames[static_cast<std::size_t>(frameIndex)];
        if (walkFrame.texture) {
            playerSprite = &walkFrame;
            playerFlip = state.shopDemo.playerFacingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        }
    }
    if (playerSprite->texture) {
        setColor(renderer, 4, 5, 6, 86);
        fillRect(renderer, sx - 14.0f, sy - 3.0f, 28.0f, 5.0f);
        const float spriteScale = shopSpriteScaleForHeight(*playerSprite, kShopPlayerPoseTargetHeight);
        drawSpriteAtAxis(renderer, *playerSprite, sx, sy, spriteScale, playerFlip);
        setColor(renderer, 230, 220, 172);
        debugTextCentered(
            renderer,
            sx,
            sy - kShopPlayerPoseTargetHeight - 11.0f,
            "P1");
        return;
    }

    setColor(renderer, 4, 5, 6, 80);
    fillRect(renderer, sx - 14.0f, sy - 3.0f, 28.0f, 5.0f);
    setColor(renderer, 72, 206, 150);
    fillRect(renderer, sx - 4.0f, sy - 35.0f, 8.0f, 8.0f);
    setColor(renderer, 44, 92, 132);
    fillRect(renderer, sx - 8.0f, sy - 26.0f, 16.0f, 20.0f);
    setColor(renderer, 34, 62, 84);
    fillRect(renderer, sx - 11.0f, sy - 7.0f, 8.0f, 8.0f);
    fillRect(renderer, sx + 3.0f, sy - 7.0f, 8.0f, 8.0f);
    setColor(renderer, 230, 220, 172);
    debugTextCentered(renderer, sx, sy - 49.0f, "P1");
}

void drawShopDemoShopkeeper(SDL_Renderer* renderer, const AppState& state) {
    if (state.shopDemo.shopkeeperPose.texture) {
        const float spriteScale = shopSpriteScaleForHeight(state.shopDemo.shopkeeperPose, kShopkeeperTargetHeight);
        drawSpriteAtAxis(
            renderer,
            state.shopDemo.shopkeeperPose,
            shopDemoScreenX(state, kShopkeeperX),
            shopDemoFloorY(kShopkeeperDepth),
            spriteScale);
        return;
    }

    const float sx = shopDemoScreenX(state, kShopkeeperX);
    const float sy = shopDemoFloorY(kShopkeeperDepth);
    setColor(renderer, 95, 54, 132);
    fillRect(renderer, sx - 10.0f, sy - 40.0f, 20.0f, 30.0f);
    setColor(renderer, 214, 198, 230);
    fillRect(renderer, sx - 5.0f, sy - 52.0f, 10.0f, 11.0f);
}

void drawShopDemoCounterFront(SDL_Renderer* renderer, const AppState& state) {
    if (state.shopDemo.shopCounterFront.texture) {
        SDL_FRect dst{
            shopDemoScreenX(state, kShopCounterX - 40.0f),
            145.0f,
            kShopCounterW + 80.0f,
            68.0f,
        };
        SDL_RenderTexture(renderer, state.shopDemo.shopCounterFront.texture, nullptr, &dst);
        return;
    }

    setColor(renderer, 14, 10, 10, 242);
    shopDemoWorldRect(renderer, state, kShopCounterX - 28.0f, 161, kShopCounterW + 56.0f, 50);
    setColor(renderer, 68, 42, 29, 238);
    shopDemoWorldRect(renderer, state, kShopCounterX - 10.0f, 166, kShopCounterW + 20.0f, 34);
    setColor(renderer, 112, 70, 39, 235);
    shopDemoWorldRect(renderer, state, kShopCounterX + 14.0f, 170, kShopCounterW - 28.0f, 11);
    setColor(renderer, 48, 30, 22, 238);
    shopDemoWorldRect(renderer, state, kShopCounterX + 2.0f, 192, kShopCounterW - 4.0f, 9);
    setColor(renderer, 224, 198, 102, 235);
    shopDemoWorldRect(renderer, state, kShopCounterX - 2.0f, 160, kShopCounterW + 4.0f, 2);
    shopDemoWorldRect(renderer, state, kShopCounterX + 12.0f, 185, kShopCounterW - 24.0f, 2);
    setColor(renderer, 92, 48, 126, 120);
    shopDemoWorldRect(renderer, state, kShopCounterX + kShopCounterW * 0.5f - 24.0f, 173, 48.0f, 20.0f);
    setColor(renderer, 132, 82, 210, 145);
    shopDemoWorldRect(renderer, state, kShopCounterX + kShopCounterW * 0.5f - 17.0f, 179, 34.0f, 3.0f);
}

#include "ShopDemoPanelOverlay.h"

void drawShopDemoHud(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoProgressionLoaded(state);
    const float width = logicalWidthF(state);
    setColor(renderer, 5, 8, 13, 182);
    fillRect(renderer, 0, 0, width, 26);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 0, 26, width, 1);
    setColor(renderer, 255, 238, 120);
    debugText(renderer, 11, 8, "SHOP HUB");
    setColor(renderer, 124, 208, 246);
    debugTextCentered(renderer, width * 0.5f, 8, "I.CHIE ITEM COUNTER");
    setColor(renderer, 190, 202, 218);
    debugText(renderer, width - 151, 8, compactSettingText(shopDemoProfileName(state), 18));

    const bool nearCounter = shopDemoNearCounter(state);
    setColor(renderer, 6, 8, 12, 164);
    fillRect(renderer, 10, 213, width - 20, 18);
    setColor(renderer, nearCounter ? 108 : 230, nearCounter ? 244 : 190, nearCounter ? 156 : 105);
    const std::string prompt = state.shopDemo.shopOpen
        ? "SHOP OPEN"
        : nearCounter ? "ENTER: SHOP" : "ARROWS/WASD MOVE   SHIFT/LT RUN";
    debugText(renderer, 16, 218, prompt);
    setColor(renderer, 150, 156, 166);
    debugText(renderer, width - 132, 218, "ESC BACK");
    if (state.shopDemo.noticeTicks > 0 && !state.shopDemo.noticeText.empty()) {
        setColor(renderer, 255, 218, 106);
        debugTextCentered(renderer, width * 0.5f, 196, compactSettingText(state.shopDemo.noticeText, 35));
    }
}

void drawShopDemo(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoAssets(renderer, state);
    drawShopDemoFloor(renderer, state);
    drawShopDemoBackdropProps(renderer, state);

    drawShopDemoShopkeeper(renderer, state);
    if (state.shopDemo.playerDepthZ <= kShopCounterFrontDepth) {
        drawShopDemoPlayer(renderer, state);
        drawShopDemoCounterFront(renderer, state);
    } else {
        drawShopDemoCounterFront(renderer, state);
        drawShopDemoPlayer(renderer, state);
    }

    drawShopDemoHud(renderer, state);
    if (state.shopDemo.shopOpen) {
        drawShopDemoItemPanel(renderer, state);
    }
    drawShopDemoTransactionBanner(renderer, state);

    drawFpsCounter(renderer, state);
    SDL_RenderPresent(renderer);
}

} // namespace
