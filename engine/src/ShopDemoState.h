#pragma once

#include <array>
#include <string>

// Internal App.cpp implementation shard.
// Runtime state for the lightweight Arena-style shop room test hook.

enum class ShopPanelMode {
    Buy,
    Sell,
    Equip,
};

enum class ShopPendingAction {
    None,
    Buy,
    Sell,
    Equip,
    Unequip,
};

struct ShopDemoState {
    TextureSprite shopBackdrop;
    TextureSprite shopCounterFront;
    TextureSprite shopkeeperPose;
    TextureSprite shopPlayerPose;
    std::array<TextureSprite, 8> shopPlayerWalkFrames{};
    TextureSprite trainingWeightIcon;
    TextureSprite guardCharmIcon;
    TextureSprite dragonSashIcon;
    bool assetsLoaded = false;
    bool shopOpen = false;
    bool confirmOpen = false;
    bool tabInputHeld = false;
    bool playerMoving = false;
    bool playerFacingLeft = false;
    bool shopkeeperGreetingReady = false;
    float playerWalkFrame = 0.0f;
    float playerX = -92.0f;
    float playerDepthZ = 10.0f;
    float cameraX = 0.0f;
    float worldZoom = 1.0f;
    int selectedItem = 0;
    int selectedEquipCharacter = 0;
    ShopPanelMode panelMode = ShopPanelMode::Buy;
    ShopPendingAction pendingAction = ShopPendingAction::None;
    std::string pendingItemId;
    std::string noticeText;
    int noticeTicks = 0;
    std::string transactionTitle;
    std::string transactionDetail;
    int transactionTicks = 0;
};
