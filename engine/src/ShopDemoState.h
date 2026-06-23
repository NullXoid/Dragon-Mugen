#pragma once

// Internal App.cpp implementation shard.
// Runtime state for the lightweight Arena-style shop room test hook.

struct ShopDemoState {
    TextureSprite shopkeeperPose;
    bool assetsLoaded = false;
    bool shopOpen = false;
    float playerX = -92.0f;
    float playerDepthZ = 10.0f;
    float cameraX = 0.0f;
    int selectedItem = 0;
    int noticeTicks = 0;
};
