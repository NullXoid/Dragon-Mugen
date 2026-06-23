#pragma once

// Internal App.cpp implementation shard.
// Lightweight Arena-style shop room proof hook.

namespace {

constexpr float kShopRoomLeft = -420.0f;
constexpr float kShopRoomRight = 420.0f;
constexpr float kShopPlayerMinX = -365.0f;
constexpr float kShopPlayerMaxX = 330.0f;
constexpr float kShopPlayerMinDepth = -34.0f;
constexpr float kShopPlayerMaxDepth = 42.0f;
constexpr float kShopkeeperX = 205.0f;
constexpr float kShopkeeperDepth = 18.0f;
constexpr float kShopFloorBaseY = 174.0f;
constexpr float kShopkeeperScale = 0.62f;

float shopDemoClampCamera(const AppState& state, float targetX) {
    const float halfView = logicalWidthF(state) * 0.5f;
    const float minCamera = kShopRoomLeft + halfView;
    const float maxCamera = kShopRoomRight - halfView;
    if (minCamera > maxCamera) {
        return 0.0f;
    }
    return std::clamp(targetX, minCamera, maxCamera);
}

float shopDemoScreenX(const AppState& state, float worldX) {
    return screenCenterX(state) + worldX - state.shopDemo.cameraX;
}

float shopDemoFloorY(float depthZ) {
    return kShopFloorBaseY + depthZ * 0.38f;
}

void shopDemoWorldRect(
    SDL_Renderer* renderer,
    const AppState& state,
    float worldX,
    float y,
    float w,
    float h) {
    fillRect(renderer, shopDemoScreenX(state, worldX), y, w, h);
}

void ensureShopDemoAssets(SDL_Renderer* renderer, AppState& state) {
    if (state.shopDemo.assetsLoaded) {
        return;
    }

    state.shopDemo.shopkeeperPose =
        loadUiPngSprite(renderer, state.gameRoot, "chars/I.Chie/I.Chie_shopkeeper_pose.png");
    state.shopDemo.shopkeeperPose.axisX = 44;
    state.shopDemo.shopkeeperPose.axisY = 115;
    state.shopDemo.assetsLoaded = true;
}

void resetShopDemoRoom(AppState& state) {
    state.shopDemo.shopOpen = false;
    state.shopDemo.playerX = -92.0f;
    state.shopDemo.playerDepthZ = 12.0f;
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.playerX);
    state.shopDemo.selectedItem = 0;
    state.shopDemo.noticeTicks = 0;
    state.frontend.screenFrame = 0;
}

void enterShopDemo(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoAssets(renderer, state);
    resetShopDemoRoom(state);
    state.frontend.screen = Screen::ShopDemo;
}

bool shopDemoNearCounter(const AppState& state) {
    return std::abs(state.shopDemo.playerX - (kShopkeeperX - 72.0f)) < 78.0f
        && state.shopDemo.playerDepthZ > -18.0f;
}

FrontendKey shopDemoFrontendKeyFromSdl(SDL_Keycode key) {
    switch (key) {
    case SDLK_ESCAPE:
        return FrontendKey::Escape;
    case SDLK_UP:
        return FrontendKey::Up;
    case SDLK_DOWN:
        return FrontendKey::Down;
    case SDLK_LEFT:
        return FrontendKey::Left;
    case SDLK_RIGHT:
        return FrontendKey::Right;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return FrontendKey::Accept;
    default:
        return FrontendKey::Other;
    }
}

void updateShopDemo(AppState& state) {
    if (state.frontend.screen != Screen::ShopDemo) {
        return;
    }
    if (state.shopDemo.noticeTicks > 0) {
        --state.shopDemo.noticeTicks;
    }
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.playerX);
}

void shopDemoMovePlayer(AppState& state, float dx, float dz) {
    state.shopDemo.playerX = std::clamp(state.shopDemo.playerX + dx, kShopPlayerMinX, kShopPlayerMaxX);
    state.shopDemo.playerDepthZ = std::clamp(state.shopDemo.playerDepthZ + dz, kShopPlayerMinDepth, kShopPlayerMaxDepth);
    state.shopDemo.cameraX = shopDemoClampCamera(state, state.shopDemo.playerX);
}

void handleShopDemoKey(SDL_Renderer* renderer, AppState& state, SDL_Keycode key) {
    ensureShopDemoAssets(renderer, state);
    const FrontendKey frontendKey = shopDemoFrontendKeyFromSdl(key);

    if (state.shopDemo.shopOpen) {
        if (frontendKey == FrontendKey::Escape) {
            state.shopDemo.shopOpen = false;
            playMenuCancelSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Up || key == SDLK_W) {
            state.shopDemo.selectedItem = (state.shopDemo.selectedItem + 2) % 3;
            playMenuCursorMoveSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Down || key == SDLK_S) {
            state.shopDemo.selectedItem = (state.shopDemo.selectedItem + 1) % 3;
            playMenuCursorMoveSound(state);
            return;
        }
        if (frontendKey == FrontendKey::Accept) {
            state.shopDemo.noticeTicks = 90;
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
            playMenuCursorDoneSound(state);
        } else {
            state.shopDemo.noticeTicks = 60;
            playMenuCancelSound(state);
        }
        return;
    }

    const float walkStep = 12.0f;
    const float depthStep = 7.0f;
    if (frontendKey == FrontendKey::Left || key == SDLK_A) {
        shopDemoMovePlayer(state, -walkStep, 0.0f);
        return;
    }
    if (frontendKey == FrontendKey::Right || key == SDLK_D) {
        shopDemoMovePlayer(state, walkStep, 0.0f);
        return;
    }
    if (frontendKey == FrontendKey::Up || key == SDLK_W) {
        shopDemoMovePlayer(state, 0.0f, -depthStep);
        return;
    }
    if (frontendKey == FrontendKey::Down || key == SDLK_S) {
        shopDemoMovePlayer(state, 0.0f, depthStep);
        return;
    }
}

void drawShopDemoFloor(SDL_Renderer* renderer, const AppState& state) {
    const float width = logicalWidthF(state);
    setColor(renderer, 16, 23, 32);
    fillRect(renderer, 0, 0, width, static_cast<float>(kLogicalHeight));

    setColor(renderer, 26, 32, 42);
    fillRect(renderer, 0, 0, width, 54);
    setColor(renderer, 54, 42, 50);
    fillRect(renderer, 0, 54, width, 23);
    setColor(renderer, 33, 29, 35);
    fillRect(renderer, 0, 77, width, 48);
    setColor(renderer, 48, 38, 28);
    fillRect(renderer, 0, 125, width, 115);

    const float camera = state.shopDemo.cameraX;
    for (int i = 0; i < 10; ++i) {
        const float y = 132.0f + static_cast<float>(i) * 10.5f;
        const Uint8 shade = static_cast<Uint8>(52 + (i % 2) * 18);
        setColor(renderer, shade, static_cast<Uint8>(42 + (i % 2) * 10), 30, 190);
        fillRect(renderer, 0, y, width, 1.0f);
    }
    for (int i = -8; i <= 8; ++i) {
        const float x = screenCenterX(state) + static_cast<float>(i) * 56.0f - std::fmod(camera, 56.0f);
        setColor(renderer, 76, 56, 36, 145);
        fillRect(renderer, x, 126, 1.0f, 114.0f);
    }

    setColor(renderer, 82, 62, 41);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 123, kShopRoomRight - kShopRoomLeft, 3);
    setColor(renderer, 18, 24, 32, 155);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 126, kShopRoomRight - kShopRoomLeft, 6);
}

void drawShopDemoBackdropProps(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 12, 16, 22, 230);
    shopDemoWorldRect(renderer, state, -380, 38, 142, 72);
    setColor(renderer, 58, 78, 96);
    shopDemoWorldRect(renderer, state, -372, 46, 126, 8);
    setColor(renderer, 36, 48, 61);
    shopDemoWorldRect(renderer, state, -370, 62, 38, 38);
    shopDemoWorldRect(renderer, state, -323, 62, 30, 38);
    shopDemoWorldRect(renderer, state, -284, 62, 30, 38);
    setColor(renderer, 222, 182, 82);
    shopDemoWorldRect(renderer, state, -367, 104, 116, 2);

    setColor(renderer, 16, 20, 27, 235);
    shopDemoWorldRect(renderer, state, 52, 38, 282, 78);
    setColor(renderer, 58, 128, 114);
    shopDemoWorldRect(renderer, state, 64, 48, 258, 9);
    setColor(renderer, 92, 76, 58);
    shopDemoWorldRect(renderer, state, 75, 69, 222, 8);
    setColor(renderer, 112, 88, 58);
    shopDemoWorldRect(renderer, state, 88, 86, 186, 9);
    setColor(renderer, 205, 168, 82);
    shopDemoWorldRect(renderer, state, 70, 110, 244, 2);

    setColor(renderer, 20, 18, 20, 230);
    shopDemoWorldRect(renderer, state, 66, 144, 244, 34);
    setColor(renderer, 74, 55, 38);
    shopDemoWorldRect(renderer, state, 76, 136, 224, 34);
    setColor(renderer, 112, 74, 42);
    shopDemoWorldRect(renderer, state, 82, 142, 212, 9);
    setColor(renderer, 224, 198, 102);
    shopDemoWorldRect(renderer, state, 80, 133, 218, 2);
}

void drawShopDemoPlayer(SDL_Renderer* renderer, const AppState& state) {
    const float sx = shopDemoScreenX(state, state.shopDemo.playerX);
    const float sy = shopDemoFloorY(state.shopDemo.playerDepthZ);
    const float shadowW = 31.0f;
    setColor(renderer, 4, 5, 6, 80);
    fillRect(renderer, sx - shadowW * 0.5f, sy - 3.0f, shadowW, 5.0f);
    setColor(renderer, 72, 206, 150);
    fillRect(renderer, sx - 5.0f, sy - 38.0f, 10.0f, 9.0f);
    setColor(renderer, 44, 92, 132);
    fillRect(renderer, sx - 9.0f, sy - 28.0f, 18.0f, 22.0f);
    setColor(renderer, 34, 62, 84);
    fillRect(renderer, sx - 12.0f, sy - 7.0f, 9.0f, 8.0f);
    fillRect(renderer, sx + 3.0f, sy - 7.0f, 9.0f, 8.0f);
    setColor(renderer, 230, 220, 172);
    debugTextCentered(renderer, sx, sy - 53.0f, "P1");
}

void drawShopDemoShopkeeper(SDL_Renderer* renderer, const AppState& state) {
    if (state.shopDemo.shopkeeperPose.texture) {
        drawSpriteAtAxis(
            renderer,
            state.shopDemo.shopkeeperPose,
            shopDemoScreenX(state, kShopkeeperX),
            shopDemoFloorY(kShopkeeperDepth),
            kShopkeeperScale);
        return;
    }

    const float sx = shopDemoScreenX(state, kShopkeeperX);
    const float sy = shopDemoFloorY(kShopkeeperDepth);
    setColor(renderer, 95, 54, 132);
    fillRect(renderer, sx - 11.0f, sy - 43.0f, 22.0f, 33.0f);
    setColor(renderer, 214, 198, 230);
    fillRect(renderer, sx - 6.0f, sy - 56.0f, 12.0f, 12.0f);
}

void drawShopDemoCounterFront(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 24, 18, 17, 238);
    shopDemoWorldRect(renderer, state, 62, 166, 254, 31);
    setColor(renderer, 92, 58, 34);
    shopDemoWorldRect(renderer, state, 68, 169, 242, 18);
    setColor(renderer, 44, 31, 24);
    shopDemoWorldRect(renderer, state, 68, 188, 242, 6);
    setColor(renderer, 225, 202, 112);
    shopDemoWorldRect(renderer, state, 76, 166, 224, 1);
}

void drawShopDemoItemPanel(SDL_Renderer* renderer, const AppState& state) {
    static constexpr std::array<const char*, 3> kItems{
        "POTION        100G",
        "TRAINING TAG  250G",
        "ARENA TICKET  500G",
    };

    const float width = logicalWidthF(state);
    const float panelW = std::min(214.0f, width - 36.0f);
    const float x = width - panelW - 16.0f;
    const float y = 55.0f;
    setColor(renderer, 5, 8, 13, 226);
    fillRect(renderer, x, y, panelW, 96);
    setColor(renderer, 86, 108, 138);
    drawRect(renderer, x, y, panelW, 96);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, x + 8, y + 18, panelW - 16, 1);
    setColor(renderer, 230, 220, 172);
    debugText(renderer, x + 12, y + 7, "I.CHIE SHOP");

    for (int i = 0; i < static_cast<int>(kItems.size()); ++i) {
        const float rowY = y + 31.0f + static_cast<float>(i) * 15.0f;
        if (i == state.shopDemo.selectedItem) {
            setColor(renderer, 72, 176, 138, 210);
            fillRect(renderer, x + 10.0f, rowY - 3.0f, panelW - 20.0f, 12.0f);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 196, 204, 214);
        }
        debugText(renderer, x + 16.0f, rowY, kItems[static_cast<std::size_t>(i)]);
    }

    setColor(renderer, 150, 156, 166);
    debugText(renderer, x + 12, y + 80, "UP/DN ITEM  ENTER BUY  ESC CLOSE");
    if (state.shopDemo.noticeTicks > 0) {
        setColor(renderer, 108, 244, 156);
        debugTextCentered(renderer, x + panelW * 0.5f, y + 66, "PURCHASE TEST OK");
    }
}

void drawShopDemoHud(SDL_Renderer* renderer, const AppState& state) {
    const float width = logicalWidthF(state);
    setColor(renderer, 5, 8, 13, 182);
    fillRect(renderer, 0, 0, width, 26);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 0, 26, width, 1);
    setColor(renderer, 255, 238, 120);
    debugText(renderer, 11, 8, "SHOP DEMO");
    setColor(renderer, 124, 208, 246);
    debugTextCentered(renderer, width * 0.5f, 8, "ARENA SHOP TEST ROOM");
    setColor(renderer, 190, 202, 218);
    debugText(renderer, width - 126, 8, "I.CHIE");

    const bool nearCounter = shopDemoNearCounter(state);
    setColor(renderer, 6, 8, 12, 164);
    fillRect(renderer, 10, 213, width - 20, 18);
    setColor(renderer, nearCounter ? 108 : 230, nearCounter ? 244 : 190, nearCounter ? 156 : 105);
    const std::string prompt = state.shopDemo.shopOpen
        ? "SHOP OPEN"
        : nearCounter ? "ENTER: SHOP" : "ARROWS/WASD MOVE   WALK TO COUNTER";
    debugText(renderer, 16, 218, prompt);
    setColor(renderer, 150, 156, 166);
    debugText(renderer, width - 132, 218, "ESC BACK");
    if (!nearCounter && state.shopDemo.noticeTicks > 0) {
        setColor(renderer, 255, 190, 112);
        debugTextCentered(renderer, width * 0.5f, 196, "MOVE CLOSER TO I.CHIE");
    }
}

void drawShopDemo(SDL_Renderer* renderer, AppState& state) {
    ensureShopDemoAssets(renderer, state);
    drawShopDemoFloor(renderer, state);
    drawShopDemoBackdropProps(renderer, state);

    const bool playerBehindShopkeeper = state.shopDemo.playerDepthZ <= kShopkeeperDepth;
    if (playerBehindShopkeeper) {
        drawShopDemoPlayer(renderer, state);
        drawShopDemoShopkeeper(renderer, state);
    } else {
        drawShopDemoShopkeeper(renderer, state);
        drawShopDemoPlayer(renderer, state);
    }
    drawShopDemoCounterFront(renderer, state);

    drawShopDemoHud(renderer, state);
    if (state.shopDemo.shopOpen) {
        drawShopDemoItemPanel(renderer, state);
    }

    drawFpsCounter(renderer, state);
    SDL_RenderPresent(renderer);
}

} // namespace
