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
constexpr float kShopkeeperX = 238.0f;
constexpr float kShopkeeperDepth = -48.0f;
constexpr float kShopkeeperVisualYOffset = 14.0f;
constexpr float kShopCounterX = -126.0f;
constexpr float kShopCounterW = 660.0f;
constexpr float kShopCounterVisualCenterX = 238.0f;
constexpr float kShopCounterVisualDefaultAspect = 1577.0f / 429.0f;
constexpr float kShopCounterFrontDepth = 48.0f;
constexpr float kShopFloorBaseY = 166.0f;
constexpr float kShopkeeperTargetHeight = 75.0f;
constexpr float kShopPlayerPoseTargetHeight = 82.0f;
constexpr float kShopCounterSolidHalfWidth = 112.0f;
constexpr float kShopCounterServiceHalfWidth = 126.0f;
constexpr float kShopkeeperTalkHalfWidth = 64.0f;
constexpr float kShopCounterSolidLeft = kShopCounterVisualCenterX - kShopCounterSolidHalfWidth;
constexpr float kShopCounterSolidRight = kShopCounterVisualCenterX + kShopCounterSolidHalfWidth;
constexpr float kShopCounterSolidBackDepth = -18.0f;
constexpr float kShopCounterSolidFrontDepth = 70.0f;
constexpr float kShopCounterInteractMinDepth = kShopCounterSolidFrontDepth + 0.5f;
constexpr float kShopCounterInteractMaxDepth = kShopCounterSolidFrontDepth + 36.0f;
constexpr float kShopkeeperTalkMinDepth = kShopPlayerMinDepth;
constexpr float kShopkeeperTalkMaxDepth = kShopCounterInteractMaxDepth;
constexpr float kShopCounterCollisionEpsilon = 0.5f;
constexpr float kShopWalkSpeed = 3.105f;
constexpr float kShopDepthSpeed = 2.115f;
constexpr float kShopRunMultiplier = 1.7f;
constexpr float kShopOpenPlayerPresentationDepth = 112.0f;

float shopDemoWorldZoomTarget(const AppState& state) {
    if (state.shopDemo.shopOpen) {
        return 1.58f;
    }
    if (state.shopDemo.shopkeeperGreetingReady || !state.shopDemo.transactionTitle.empty()) {
        return 1.18f;
    }
    return 1.0f;
}

float shopDemoWorldFocusTargetX(const AppState& state) {
    if (state.shopDemo.shopOpen || state.shopDemo.shopkeeperGreetingReady || !state.shopDemo.transactionTitle.empty()) {
        return kShopkeeperX + 12.0f;
    }
    return kShopCounterVisualCenterX;
}

float shopDemoClampCamera(const AppState& state, float targetX) {
    const float halfView = logicalWidthF(state) * 0.5f / std::max(1.0f, state.shopDemo.worldZoom);
    const float minCamera = kShopRoomLeft + halfView;
    const float maxCamera = kShopRoomRight - halfView;
    if (minCamera > maxCamera) return 0.0f;
    return std::clamp(targetX, minCamera, maxCamera);
}

float shopDemoCounterCameraX(const AppState& state) {
    return shopDemoClampCamera(state, kShopCounterVisualCenterX);
}

float shopDemoVisiblePlayerMinX(const AppState& state) {
    const float halfView = logicalWidthF(state) * 0.5f;
    const float margin = std::clamp(logicalWidthF(state) * 0.09f, 28.0f, 80.0f);
    return std::max(kShopPlayerMinX, shopDemoCounterCameraX(state) - halfView + margin);
}

float shopDemoVisiblePlayerMaxX(const AppState& state) {
    const float halfView = logicalWidthF(state) * 0.5f;
    const float margin = std::clamp(logicalWidthF(state) * 0.09f, 28.0f, 80.0f);
    return std::min(kShopPlayerMaxX, shopDemoCounterCameraX(state) + halfView - margin);
}

void shopDemoUpdateCamera(AppState& state, bool snap = false) {
    const float targetZoom = shopDemoWorldZoomTarget(state);
    state.shopDemo.worldZoom = snap
        ? targetZoom
        : state.shopDemo.worldZoom + (targetZoom - state.shopDemo.worldZoom) * 0.12f;
    state.shopDemo.worldZoom = std::clamp(state.shopDemo.worldZoom, 1.0f, 1.72f);

    const float targetCamera = shopDemoClampCamera(state, shopDemoWorldFocusTargetX(state));
    state.shopDemo.cameraX = snap
        ? targetCamera
        : state.shopDemo.cameraX + (targetCamera - state.shopDemo.cameraX) * 0.16f;
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.cameraX);
}

#include "ShopHubScene.h"

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
    return "A.Ben";
}

std::string shopDemoCharacterName(const AppState& state) {
    if (const CharacterSlot* slot = shopDemoSelectedCharacter(state)) {
        return slot->displayName.empty() ? slot->id : slot->displayName;
    }
    return "A.Ben";
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
    const auto loadOptionalShopSprite = [&](const std::filesystem::path& relativePath, TextureFilter filter = TextureFilter::Linear) {
        TextureSprite sprite = std::filesystem::exists(state.gameRoot / relativePath)
            ? loadUiPngSprite(renderer, state.gameRoot, relativePath)
            : TextureSprite{};
        setShopSpriteRenderStyle(sprite, filter);
        return sprite;
    };
    const auto loadOptionalShopSpriteFirst = [&](std::initializer_list<std::filesystem::path> relativePaths, TextureFilter filter = TextureFilter::Linear) {
        for (const auto& relativePath : relativePaths) {
            if (std::filesystem::exists(state.gameRoot / relativePath)) {
                TextureSprite sprite = loadUiPngSprite(renderer, state.gameRoot, relativePath);
                setShopSpriteRenderStyle(sprite, filter);
                return sprite;
            }
        }
        TextureSprite sprite{};
        setShopSpriteRenderStyle(sprite, filter);
        return sprite;
    };
    state.shopDemo.shopBackdrop = loadOptionalShopSprite("data/shop/i_chie_shop_backdrop.png");
    state.shopDemo.shopCounterFront = loadOptionalShopSprite("data/shop/i_chie_shop_counter_front.png");
    state.shopDemo.shopkeeperPose = loadOptionalShopSpriteFirst({
        "chars/I.Chie/shop/shopkeeper_pose.png",
        "chars/I.Chie/I.Chie_shopkeeper_pose.png",
    });
    state.shopDemo.shopkeeperPose.axisX = state.shopDemo.shopkeeperPose.width / 2;
    state.shopDemo.shopkeeperPose.axisY = std::max(0, state.shopDemo.shopkeeperPose.height - 2);
    state.shopDemo.shopPlayerPose = loadOptionalShopSpriteFirst({
        "chars/A.Ben/shop/shop_player_back_pose.png",
        "data/shop/shop_player_back_pose.png",
    });
    state.shopDemo.shopPlayerPose.axisX = state.shopDemo.shopPlayerPose.width / 2;
    state.shopDemo.shopPlayerPose.axisY = std::max(0, state.shopDemo.shopPlayerPose.height - 2);
    const char* walkFramePaths[] = {
        "chars/A.Ben/shop/walk/shop_player_walk_0.png",
        "chars/A.Ben/shop/walk/shop_player_walk_1.png",
        "chars/A.Ben/shop/walk/shop_player_walk_2.png",
        "chars/A.Ben/shop/walk/shop_player_walk_3.png",
        "chars/A.Ben/shop/walk/shop_player_walk_4.png",
        "chars/A.Ben/shop/walk/shop_player_walk_5.png",
        "chars/A.Ben/shop/walk/shop_player_walk_6.png",
        "chars/A.Ben/shop/walk/shop_player_walk_7.png",
    };
    const char* legacyWalkFramePaths[] = {
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
        frame = loadOptionalShopSpriteFirst({
            walkFramePaths[i],
            legacyWalkFramePaths[i],
        });
        frame.axisX = frame.width / 2;
        frame.axisY = std::max(0, frame.height - 2);
    }
    state.shopDemo.trainingWeightIcon = loadOptionalShopSprite("data/shop/items/training_weight.png", TextureFilter::Nearest);
    state.shopDemo.guardCharmIcon = loadOptionalShopSprite("data/shop/items/guard_charm.png", TextureFilter::Nearest);
    state.shopDemo.dragonSashIcon = loadOptionalShopSprite("data/shop/items/dragon_sash.png", TextureFilter::Nearest);
    state.shopDemo.assetsLoaded = true;
}

void resetShopDemoRoom(AppState& state) {
    ensureShopDemoProgressionLoaded(state);
    state.shopDemo.shopOpen = false;
    state.shopDemo.confirmOpen = false;
    state.shopDemo.playerX = kShopkeeperX - 118.0f;
    state.shopDemo.playerDepthZ = kShopCounterSolidFrontDepth + 18.0f;
    state.shopDemo.worldZoom = 1.0f;
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.playerX);
    shopDemoUpdateCamera(state, true);
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
    state.shopDemo.shopkeeperGreetingReady = false;
    state.shopDemo.playerWalkFrame = 0.0f;
    state.frontend.screenFrame = 0;
}

void enterShopDemo(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoAssets(renderer, state);
    resetShopDemoRoom(state);
    state.frontend.screen = Screen::ShopDemo;
}

enum class ShopInteractionKind {
    None,
    CounterService,
    ShopkeeperTalk,
};

shop_demo::ShopInteractionVolume shopDemoCounterServiceVolume() {
    return {
        kShopCounterVisualCenterX - kShopCounterServiceHalfWidth,
        kShopCounterVisualCenterX + kShopCounterServiceHalfWidth,
        kShopCounterInteractMinDepth,
        kShopCounterInteractMaxDepth,
    };
}

shop_demo::ShopInteractionVolume shopDemoShopkeeperTalkVolume() {
    return {
        kShopkeeperX - kShopkeeperTalkHalfWidth,
        kShopkeeperX + kShopkeeperTalkHalfWidth,
        kShopkeeperTalkMinDepth,
        kShopkeeperTalkMaxDepth,
    };
}

ShopInteractionKind shopDemoInteractionKind(const AppState& state) {
    const float playerX = state.shopDemo.playerX;
    const float playerDepth = state.shopDemo.playerDepthZ;
    // Talk wins where the service and shopkeeper volumes overlap.
    if (shop_demo::shopDemoInsideInteractionVolume(shopDemoShopkeeperTalkVolume(), playerX, playerDepth)) {
        return ShopInteractionKind::ShopkeeperTalk;
    }
    if (shop_demo::shopDemoInsideInteractionVolume(shopDemoCounterServiceVolume(), playerX, playerDepth)) {
        return ShopInteractionKind::CounterService;
    }
    return ShopInteractionKind::None;
}

bool shopDemoPlayerBehindCounter(const AppState& state) {
    return state.shopDemo.playerDepthZ <= kShopCounterSolidBackDepth;
}

std::string shopDemoGreetingText(const AppState& state) {
    std::string profileName = uppercaseCopy(shopDemoProfileName(state));
    if (profileName.empty()) {
        profileName = "WARRIOR";
    }
    return "HI, " + profileName + ".";
}

void shopDemoOpenServicePanel(AppState& state) {
    state.shopDemo.shopOpen = true;
    state.shopDemo.confirmOpen = false;
    state.shopDemo.noticeTicks = 0;
    state.shopDemo.noticeText.clear();
    state.shopDemo.transactionTitle.clear();
    state.shopDemo.transactionDetail.clear();
    state.shopDemo.transactionTicks = 0;
    state.shopDemo.shopkeeperGreetingReady = false;
}

void shopDemoBeginShopkeeperGreeting(AppState& state) {
    state.shopDemo.shopOpen = false;
    state.shopDemo.confirmOpen = false;
    state.shopDemo.pendingAction = ShopPendingAction::None;
    state.shopDemo.pendingItemId.clear();
    state.shopDemo.shopkeeperGreetingReady = true;
    shopDemoSetTransaction(state, "I.CHIE", shopDemoGreetingText(state), 150);
}

bool shopDemoOpenServicePanelAfterGreeting(AppState& state) {
    if (!state.shopDemo.shopkeeperGreetingReady
        || shopDemoInteractionKind(state) != ShopInteractionKind::ShopkeeperTalk) {
        return false;
    }
    shopDemoOpenServicePanel(state);
    return true;
}

bool handleShopDemoShopActionButton(AppState& state) {
    return shopDemoOpenServicePanelAfterGreeting(state);
}

bool shopDemoShopActionKeyPressed(const AppState& state, SDL_Keycode key) {
    return keyMatchesControlAction(state, key, 0, InputAction::LK);
}

std::string shopDemoOpenShopPrompt(const AppState& state) {
    const std::string token = commandButtonDisplayToken("a", commandButtonPromptModeForPlayer(state, 0));
    if (token.empty() || token == "LK") {
        return "LK  OPEN SHOP";
    }
    return "LK / " + token + "  OPEN SHOP";
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
    if (!state.shopDemo.shopOpen && shopDemoInteractionKind(state) != ShopInteractionKind::ShopkeeperTalk) {
        state.shopDemo.shopkeeperGreetingReady = false;
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
    shopDemoUpdateCamera(state);
}

void shopDemoMovePlayer(AppState& state, float dx, float dz) {
    float oldX = std::clamp(state.shopDemo.playerX, kShopPlayerMinX, kShopPlayerMaxX);
    float oldDepthZ = std::clamp(state.shopDemo.playerDepthZ, kShopPlayerMinDepth, kShopPlayerMaxDepth);
    const auto collisionBounds = shopDemoCounterCollisionBounds();
    shop_demo::shopDemoSnapOutOfCounterSolid(collisionBounds, oldX, oldDepthZ);

    float nextX = std::clamp(oldX + dx, kShopPlayerMinX, kShopPlayerMaxX);
    float nextDepthZ = std::clamp(oldDepthZ + dz, kShopPlayerMinDepth, kShopPlayerMaxDepth);
    shop_demo::shopDemoResolveCounterCollision(collisionBounds, oldX, oldDepthZ, dx, dz, nextX, nextDepthZ);

    state.shopDemo.playerX = std::clamp(nextX, shopDemoVisiblePlayerMinX(state), shopDemoVisiblePlayerMaxX(state));
    state.shopDemo.playerDepthZ = nextDepthZ;
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
    if (state.shopDemo.pendingAction == ShopPendingAction::Buy) {
        const std::string profileId = shopDemoProfileId(state);
        const auto stats = effectiveDragonProgressionStatsForProfile(
            state.progression.data,
            state.progression.save,
            profileId,
            shopDemoTargetCharacterId(state));
        if (stats.level < entry->requiredLevel) {
            shopDemoSetNotice(state, "REQUIRED LEVEL " + std::to_string(entry->requiredLevel), 90);
            state.shopDemo.pendingAction = ShopPendingAction::None;
            state.shopDemo.pendingItemId.clear();
            state.shopDemo.confirmOpen = false;
            return;
        }
        const int balance = dragonProgressionGoldForProfile(state.progression.save, profileId);
        if (balance < entry->price) {
            shopDemoSetNotice(state, "NEED G" + std::to_string(entry->price - balance) + " MORE", 90);
            state.shopDemo.pendingAction = ShopPendingAction::None;
            state.shopDemo.pendingItemId.clear();
            state.shopDemo.confirmOpen = false;
            return;
        }
    }
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
    case ShopPendingAction::Buy: {
        if (stats.level < entry->requiredLevel) {
            shopDemoSetNotice(state, "LV " + std::to_string(entry->requiredLevel) + " REQUIRED");
            return false;
        }
        if (!spendDragonProgressionGoldForProfile(state.progression.save, profileId, entry->price)) {
            shopDemoSetNotice(state, "NOT ENOUGH GOLD");
            return false;
        }
        grantDragonProgressionItemForProfile(state.progression.save, profileId, entry->itemId, 1);
        const int newOwned = shopDemoOwnedQuantity(state, entry->itemId);
        saveShopDemoProgression(state);
        shopDemoSetTransaction(
            state,
            "PURCHASED " + entry->name,
            "OWNED: " + std::to_string(newOwned)
                + "  BAL G" + std::to_string(dragonProgressionGoldForProfile(state.progression.save, profileId)));
        return true;
    }
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
    const bool shopActionPressed = shopDemoShopActionKeyPressed(state, key);

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

    if (shopActionPressed && shopDemoOpenServicePanelAfterGreeting(state)) {
        playMenuCursorDoneSound(state);
        return;
    }

    if (frontendKey == FrontendKey::Escape) {
        state.frontend.screen = Screen::ModeSelect;
        playMenuCancelSound(state);
        return;
    }
    if (frontendKey == FrontendKey::Accept) {
        const ShopInteractionKind interaction = shopDemoInteractionKind(state);
        if (interaction == ShopInteractionKind::ShopkeeperTalk) {
            if (!state.shopDemo.shopkeeperGreetingReady) {
                shopDemoBeginShopkeeperGreeting(state);
                playMenuCursorDoneSound(state);
            }
        } else if (interaction == ShopInteractionKind::CounterService) {
            shopDemoOpenServicePanel(state);
            playMenuCursorDoneSound(state);
        } else {
            shopDemoSetNotice(state, "MOVE TO THE COUNTER", 60);
            playMenuCancelSound(state);
        }
        return;
    }

    (void)frontendKey;
}

#include "ShopDemoPanelOverlay.h"

void drawShopDemoHud(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoProgressionLoaded(state);
    const float width = logicalWidthF(state);
    const ShopDemoLayoutRects rects = shopDemoLayoutRects(state);
    const DragonLayoutClass layoutClass = layoutClassForDimensions(CanvasDimensions{ logicalWidth(state), logicalHeight(state) });
    const DragonUiMetrics metrics = dragonUiMetricsForCanvas(CanvasDimensions{ logicalWidth(state), logicalHeight(state) }, uiScale(state));
    const auto& tokens = dragonUiTokens();
    const float s = metrics.pixelScale;
    const std::string profile = compactSettingText(uppercaseCopy(shopDemoProfileName(state)), layoutClass == DragonLayoutClass::Classic ? 10 : 18);
    const std::string gold = "GOLD " + std::to_string(dragonProgressionGoldForProfile(state.progression.save, shopDemoProfileId(state)));
    const std::string rightStatus = profile + "    " + gold;
    const std::string leftLabel = "SHOP HUB";
    const std::string shopName = "I.CHIE ITEM COUNTER";
    const float textY = 8.0f * s;
    const float leftX = 10.0f * s;
    const float leftW = static_cast<float>(leftLabel.size()) * 8.0f * s;
    const float rightW = static_cast<float>(rightStatus.size()) * 8.0f * s;
    const bool performanceHudVisible =
        effectivePerformanceHudMode(state) != PerformanceHudMode::Off && !state.suppressFpsCounter;
    const float performanceHudReserve = performanceHudVisible ? 158.0f * s : 0.0f;
    const float rightX = std::max(leftX + leftW + 8.0f * s, width - performanceHudReserve - rightW - 10.0f * s);
    const float centerLeft = leftX + leftW + 18.0f * s;
    const float centerRight = std::max(centerLeft + 8.0f * s, rightX - 18.0f * s);
    const float centerAvailable = std::max(8.0f * s, centerRight - centerLeft);
    const std::string fittedShopName = fitDebugText(
        shopName,
        static_cast<size_t>(std::max(1.0f, std::floor(centerAvailable / (8.0f * s)))));
    const float fittedShopNameW = static_cast<float>(fittedShopName.size()) * 8.0f * s;

    setColor(renderer, tokens.panelBase, 214);
    fillRect(renderer, rects.topBar.x, rects.topBar.y, rects.topBar.w, rects.topBar.h);
    setColor(renderer, tokens.separatorRed);
    fillRect(renderer, 0.0f, rects.topBar.h - metrics.border, width, metrics.border);
    setColor(renderer, tokens.mutedGold);
    scaledDebugText(renderer, s, leftX, textY, leftLabel);
    setColor(renderer, tokens.primaryTeal);
    scaledDebugText(renderer, s, centerLeft + (centerAvailable - fittedShopNameW) * 0.5f, textY, fittedShopName);
    setColor(renderer, tokens.primaryText);
    scaledDebugText(renderer, s, rightX, textY, rightStatus);

    setColor(renderer, tokens.panelBase, 190);
    fillRect(renderer, 0.0f, rects.helpBar.y, width, rects.helpBar.h);
    setColor(renderer, tokens.primaryTeal, 120);
    drawRect(renderer, 10.0f * s, rects.helpBar.y + 3.0f * s, width - 20.0f * s, rects.helpBar.h - 6.0f * s);

    const ShopInteractionKind interaction = shopDemoInteractionKind(state);
    const std::string footerPrompt = state.shopDemo.shopOpen
        ? "SHOP OPEN"
        : state.shopDemo.shopkeeperGreetingReady && interaction == ShopInteractionKind::ShopkeeperTalk ? shopDemoOpenShopPrompt(state)
        : interaction == ShopInteractionKind::ShopkeeperTalk ? "ENTER  TALK / SHOP"
        : interaction == ShopInteractionKind::CounterService ? "ENTER  BUY / SELL"
        : "ARROWS/WASD MOVE   SHIFT/LT RUN";
    setColor(renderer, interaction != ShopInteractionKind::None ? tokens.primaryTeal : tokens.mutedText);
    scaledDebugText(renderer, s, 16.0f * s, rects.helpBar.y + 8.0f * s, footerPrompt);
    setColor(renderer, tokens.mutedText);
    scaledDebugText(renderer, s, width - 74.0f * s, rects.helpBar.y + 8.0f * s, "ESC BACK");

    if (state.shopDemo.noticeTicks > 0 && !state.shopDemo.noticeText.empty()) {
        setColor(renderer, tokens.mutedGold);
        const std::string notice = compactSettingText(state.shopDemo.noticeText, layoutClass == DragonLayoutClass::Classic ? 28 : 42);
        scaledDebugText(renderer, s, width * 0.5f - static_cast<float>(notice.size()) * 4.0f * s, rects.helpBar.y - 18.0f * s, notice);
    }
}

void drawShopDemo(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoAssets(renderer, state);
    const SDL_Rect worldClip = shopDemoWorldClipRect(state);
    SDL_SetRenderClipRect(renderer, &worldClip);
    drawShopDemoFloor(renderer, state);
    drawShopDemoBackdropProps(renderer, state);
    drawShopDemoShopkeeperShadow(renderer, state);
    drawShopDemoShopkeeper(renderer, state);
    const bool playerBehindCounter = !state.shopDemo.shopOpen && shopDemoPlayerBehindCounter(state);
    if (playerBehindCounter) {
        drawShopDemoPlayerShadow(renderer, state);
        drawShopDemoPlayer(renderer, state);
    }
    drawShopDemoCounterFront(renderer, state);
    if (!playerBehindCounter) {
        drawShopDemoPlayerShadow(renderer, state);
        drawShopDemoPlayer(renderer, state);
    }
    SDL_SetRenderClipRect(renderer, nullptr);

    drawShopDemoHud(renderer, state);
    if (state.shopDemo.shopOpen) {
        drawShopDemoItemPanel(renderer, state);
    }
    drawShopDemoTransactionBanner(renderer, state);

    drawFpsCounter(renderer, state);
    SDL_RenderPresent(renderer);
}

} // namespace
