#pragma once

// Internal shop hub scene/projection helpers.
// Include only from ShopDemoRuntime.h after shop room constants are defined.

using ShopDemoLayoutRects = shop_demo::ShopDemoLayoutRects;

ShopDemoLayoutRects shopDemoLayoutRects(const AppState& state) {
    const DragonUiMetrics metrics = dragonUiMetricsForCanvas(CanvasDimensions{ logicalWidth(state), logicalHeight(state) }, uiScale(state));
    const float width = logicalWidthF(state);
    const float height = logicalHeightF(state);
    const float topH = metrics.topBarH;
    const float helpH = metrics.helpBarH;
    return shop_demo::makeShopDemoLayoutRects(width, height, topH, helpH);
}

float shopDemoSceneScaleY(const AppState& state) {
    return shopDemoLayoutRects(state).world.h / 194.0f;
}

float shopDemoWorldZoom(const AppState& state) {
    return shop_demo::clampShopDemoWorldZoom(state.shopDemo.worldZoom);
}

shop_demo::ShopPerspectiveCamera shopDemoPerspectiveCamera(const AppState& state) {
    const SDL_FRect world = shopDemoLayoutRects(state).world;
    constexpr float kFocalLength = 620.0f;
    return {
        world.x + world.w * 0.5f,
        world.y - world.h * (0.16f + state.shopDemo.cinematicBlend * 0.34f),
        state.shopDemo.cameraX,
        kFocalLength / shopDemoWorldZoom(state),
        kFocalLength,
        world.h * 0.86f,
        -0.025f * state.shopDemo.cinematicBlend,
    };
}

shop_demo::ShopPerspectivePoint shopDemoProjectGround(const AppState& state, float worldX, float depthZ) {
    return shop_demo::projectShopGroundPoint(shopDemoPerspectiveCamera(state), worldX, depthZ);
}

float shopDemoScreenXAtDepth(const AppState& state, float worldX, float depthZ) {
    if (state.shopDemo.perspectiveCameraEnabled) {
        return shopDemoProjectGround(state, worldX, depthZ).x;
    }
    return screenCenterX(state) + (worldX - state.shopDemo.cameraX) * shopDemoWorldZoom(state);
}

float shopDemoScreenX(const AppState& state, float worldX) {
    return shopDemoScreenXAtDepth(state, worldX, 0.0f);
}

float shopDemoSceneY(const AppState& state, float y240) {
    const SDL_FRect world = shopDemoLayoutRects(state).world;
    return shop_demo::projectShopDemoSceneY(world, shopDemoWorldZoom(state), y240);
}

float shopDemoSceneH(const AppState& state, float h240) {
    return shop_demo::projectShopDemoSceneHeight(shopDemoLayoutRects(state).world, shopDemoWorldZoom(state), h240);
}

float shopDemoCounterFrontBottomY(const AppState& state) {
    return shopDemoSceneY(state, 182.4f);
}

float shopDemoCounterVisualBottomY(const AppState& state) {
    return shopDemoSceneY(state, 201.6f);
}

float shopDemoFloorBaseY(const AppState& state) {
    return shopDemoCounterFrontBottomY(state);
}

float shopDemoCameraPan01(const AppState& state) {
    const float halfView = logicalWidthF(state) * 0.5f / shopDemoWorldZoom(state);
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

void setShopSpriteRenderStyle(TextureSprite& sprite, TextureFilter filter = TextureFilter::Linear) {
    if (!sprite.texture) {
        return;
    }
    SDL_SetTextureBlendMode(sprite.texture, SDL_BLENDMODE_BLEND);
    setTextureSpriteFilterIntent(sprite, filter);
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

float shopDemoFloorY(const AppState& state, float depthZ) {
    if (state.shopDemo.perspectiveCameraEnabled) {
        return shopDemoProjectGround(state, state.shopDemo.cameraX, depthZ).floorY;
    }
    return shopDemoFloorBaseY(state) + depthZ * 0.38f * shopDemoSceneScaleY(state) * shopDemoWorldZoom(state);
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
    fillRect(renderer, shopDemoScreenX(state, worldX), shopDemoSceneY(state, y), w * shopDemoWorldZoom(state), shopDemoSceneH(state, h));
}

SDL_Rect shopDemoWorldClipRect(const AppState& state) {
    const SDL_FRect world = shopDemoLayoutRects(state).world;
    return SDL_Rect{
        static_cast<int>(std::floor(world.x)),
        static_cast<int>(std::floor(world.y)),
        static_cast<int>(std::ceil(world.w)),
        static_cast<int>(std::ceil(world.h)),
    };
}

void shopDemoWorldTextCentered(
    SDL_Renderer* renderer,
    const AppState& state,
    float worldCenterX,
    float y,
    const std::string& text) {
    debugTextCentered(renderer, shopDemoScreenX(state, worldCenterX), shopDemoSceneY(state, y), text);
}

void drawShopDemoContactShadow(
    SDL_Renderer* renderer,
    float centerX,
    float centerY,
    float width,
    float height,
    Uint8 alpha) {
    if (width <= 0.0f || height <= 0.0f || alpha == 0) {
        return;
    }
    const int rows = std::max(1, static_cast<int>(std::ceil(height)));
    const float radiusY = height * 0.5f;
    for (int row = 0; row < rows; ++row) {
        const float y = centerY - radiusY + static_cast<float>(row);
        const float dy = (static_cast<float>(row) + 0.5f - radiusY) / std::max(1.0f, radiusY);
        const float rowW = width * std::sqrt(std::max(0.0f, 1.0f - dy * dy));
        const Uint8 rowAlpha = static_cast<Uint8>(std::clamp(static_cast<int>(alpha * (1.0f - std::fabs(dy) * 0.45f)), 0, 255));
        setColor(renderer, 0, 0, 0, rowAlpha);
        fillRect(renderer, centerX - rowW * 0.5f, y, rowW, 1.0f);
    }
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

void drawShopDemoFloor(SDL_Renderer* renderer, const AppState& state) {
    const float width = logicalWidthF(state);
    const float height = logicalHeightF(state);
    if (state.shopDemo.shopBackdrop.texture) {
        const SDL_FRect world = shopDemoLayoutRects(state).world;
        const float zoom = shopDemoWorldZoom(state);
        SDL_FRect dst{
            world.x + (world.w - world.w * zoom) * 0.5f,
            world.y + (world.h - world.h * zoom) * 0.5f,
            world.w * zoom,
            world.h * zoom,
        };
        drawShopTextureCover(renderer, state.shopDemo.shopBackdrop, dst, shopDemoBackdropPan01(state));
        const TextureSprite& focusBackdrop = state.shopDemo.layeredSceneV2Enabled
                && state.shopDemo.shopFocusBackdropV2.texture
            ? state.shopDemo.shopFocusBackdropV2
            : state.shopDemo.shopFocusBackdrop;
        if (focusBackdrop.texture && state.shopDemo.cinematicBlend > 0.01f) {
            const Uint8 focusAlpha = static_cast<Uint8>(std::clamp(
                static_cast<int>(std::lround(state.shopDemo.cinematicBlend * 255.0f)),
                0,
                255));
            SDL_SetTextureAlphaMod(focusBackdrop.texture, focusAlpha);
            drawShopTextureCover(renderer, focusBackdrop, dst, 0.5f);
            SDL_SetTextureAlphaMod(focusBackdrop.texture, 255);
        }
        return;
    }

    setColor(renderer, 8, 10, 15);
    fillRect(renderer, 0, 0, width, height);

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
        const float y = shopDemoSceneY(state, 131.0f + static_cast<float>(i) * 10.0f);
        const Uint8 shade = static_cast<Uint8>(46 + (i % 2) * 12);
        setColor(renderer, shade, static_cast<Uint8>(34 + (i % 2) * 8), 24, 188);
        fillRect(renderer, 0, y, width, 1.0f);
    }
    for (int i = -12; i <= 12; ++i) {
        const float x = screenCenterX(state) + static_cast<float>(i) * 56.0f - std::fmod(camera, 56.0f);
        setColor(renderer, 72, 52, 32, 120);
        fillRect(renderer, x, shopDemoSceneY(state, 126.0f), 1.0f, shopDemoSceneH(state, 114.0f));
    }
    for (int i = -12; i <= 12; ++i) {
        const float x = screenCenterX(state) + static_cast<float>(i) * 112.0f - std::fmod(camera * 0.55f, 112.0f);
        setColor(renderer, 216, 176, 92, 28);
        fillRect(renderer, x, shopDemoSceneY(state, 126.0f), 42.0f, shopDemoSceneH(state, 114.0f));
    }

    setColor(renderer, 112, 80, 45);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 122, kShopRoomRight - kShopRoomLeft, 3);
    setColor(renderer, 15, 20, 27, 175);
    shopDemoWorldRect(renderer, state, kShopRoomLeft, 126, kShopRoomRight - kShopRoomLeft, 6);
}

void drawShopDemoBackdropProps(SDL_Renderer* renderer, const AppState& state) {
    if (state.shopDemo.shopBackdrop.texture) {
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
}

void drawShopDemoV2Prop(
    SDL_Renderer* renderer,
    const AppState& state,
    std::size_t propIndex,
    float worldX,
    float depthZ,
    float worldHeight,
    float opacity = 1.0f) {
    if (!state.shopDemo.layeredSceneV2Enabled
        || propIndex >= state.shopDemo.shopV2Props.size()) {
        return;
    }
    const TextureSprite& prop = state.shopDemo.shopV2Props[propIndex];
    if (!prop.texture || prop.height <= 0) {
        return;
    }
    const auto projected = shopDemoProjectGround(state, worldX, depthZ);
    const float targetHeight = worldHeight * projected.scale;
    const Uint8 alpha = static_cast<Uint8>(std::clamp(
        static_cast<int>(std::lround(opacity * 255.0f)),
        0,
        255));
    SDL_SetTextureAlphaMod(prop.texture, alpha);
    drawSpriteAtAxis(renderer, prop, projected.x, projected.floorY, shopSpriteScaleForHeight(prop, targetHeight));
    SDL_SetTextureAlphaMod(prop.texture, 255);
}

void drawShopDemoLayeredWallPropsV2(SDL_Renderer* renderer, const AppState& state) {
    if (!state.shopDemo.layeredSceneV2Enabled || state.shopDemo.cinematicBlend <= 0.01f) {
        return;
    }
    const float opacity = std::clamp(state.shopDemo.cinematicBlend, 0.0f, 1.0f);
    drawShopDemoV2Prop(renderer, state, 0, 510.0f, -84.0f, 184.0f, opacity * 0.92f);
    drawShopDemoV2Prop(renderer, state, 4, 345.0f, -84.0f, 37.0f, opacity * 0.68f);
    drawShopDemoV2Prop(renderer, state, 3, 315.0f, -64.0f, 55.0f, opacity * 0.92f);
}

float shopDemoRenderedPlayerDepth(const AppState& state);

float shopDemoPlayerTargetHeight(const AppState& state) {
    if (state.shopDemo.perspectiveCameraEnabled) {
        const float depthScale = shopDemoProjectGround(
            state,
            state.shopDemo.playerX,
            shopDemoRenderedPlayerDepth(state)).scale;
        return shopDemoLayoutRects(state).world.h * 0.33f * depthScale;
    }
    const float openShotScale = state.shopDemo.shopOpen ? 1.34f : 1.0f;
    return shop_demo::shopDemoPlayerTargetHeight(shopDemoLayoutRects(state).world, shopDemoWorldZoom(state), openShotScale);
}

float shopDemoShopkeeperTargetHeight(const AppState& state) {
    const float focusScale = 1.0f + state.shopDemo.cinematicBlend * 0.28f;
    if (state.shopDemo.perspectiveCameraEnabled) {
        const float depthScale = shopDemoProjectGround(state, kShopkeeperX, kShopkeeperDepth).scale;
        return shopDemoLayoutRects(state).world.h * 0.27f * depthScale * focusScale;
    }
    return shop_demo::shopDemoShopkeeperTargetHeight(shopDemoLayoutRects(state).world, shopDemoWorldZoom(state)) * focusScale;
}

float shopDemoCounterTargetHeight(const AppState& state) {
    const float baseHeight = std::clamp(shopDemoLayoutRects(state).world.h * 0.18f, 68.0f, 88.0f);
    if (state.shopDemo.perspectiveCameraEnabled) {
        const float visualDepth = 70.0f + (20.0f - 70.0f) * state.shopDemo.cinematicBlend;
        const float depthScale = shopDemoProjectGround(state, kShopCounterVisualCenterX, visualDepth).scale;
        const float focusedCounterGain = state.shopDemo.shopOpen ? 0.49f : 0.10f;
        return baseHeight * depthScale * (1.0f + state.shopDemo.cinematicBlend * focusedCounterGain);
    }
    return baseHeight * shopDemoWorldZoom(state) * (1.0f + state.shopDemo.cinematicBlend * 1.02f);
}

float shopDemoShopkeeperVisualY(const AppState& state) {
    return shopDemoFloorY(state, kShopkeeperDepth) + kShopkeeperVisualYOffset * shopDemoSceneScaleY(state) * shopDemoWorldZoom(state);
}

float shopDemoRenderedPlayerDepth(const AppState& state) {
    return state.shopDemo.shopOpen
        ? kShopOpenPlayerPresentationDepth
        : state.shopDemo.playerDepthZ;
}

float shopDemoRenderedPlayerScreenX(const AppState& state) {
    const float freeRoamX = shopDemoScreenXAtDepth(state, state.shopDemo.playerX, shopDemoRenderedPlayerDepth(state));
    if (state.shopDemo.cinematicBlend <= 0.0f) {
        return freeRoamX;
    }
    const SDL_FRect world = shopDemoLayoutRects(state).world;
    const float stagedX = world.x + world.w * (state.shopDemo.shopOpen ? 0.27f : 0.34f);
    return freeRoamX + (stagedX - freeRoamX) * state.shopDemo.cinematicBlend;
}

float shopDemoRenderedPlayerBaselineY(const AppState& state) {
    if (!state.shopDemo.shopOpen) {
        const SDL_FRect world = shopDemoLayoutRects(state).world;
        const float projectedY = shopDemoFloorY(state, shopDemoRenderedPlayerDepth(state))
            + 6.0f * shopDemoSceneScaleY(state) * state.shopDemo.cinematicBlend;
        return std::min(projectedY, world.y + world.h - 8.0f);
    }
    const SDL_FRect world = shopDemoLayoutRects(state).world;
    return world.y + world.h * 1.02f;
}

void drawShopDemoPlayerShadow(SDL_Renderer* renderer, const AppState& state) {
    const float playerDepth = shopDemoRenderedPlayerDepth(state);
    const float sx = shopDemoRenderedPlayerScreenX(state);
    float sy = shopDemoRenderedPlayerBaselineY(state);
    const float targetH = shopDemoPlayerTargetHeight(state);
    const float depthNarrow = std::clamp(1.0f - (playerDepth - kShopPlayerMinDepth) / (kShopPlayerMaxDepth - kShopPlayerMinDepth) * 0.18f, 0.74f, 1.0f);
    if (state.shopDemo.shopOpen) {
        const SDL_FRect world = shopDemoLayoutRects(state).world;
        sy = std::min(sy, world.y + world.h - 10.0f * dragonUiMetricsForCanvas(CanvasDimensions{ logicalWidth(state), logicalHeight(state) }, uiScale(state)).pixelScale);
    }
    drawShopDemoContactShadow(renderer, sx, sy - 1.0f * shopDemoSceneScaleY(state) * shopDemoWorldZoom(state), targetH * 0.45f * depthNarrow, std::max(3.0f, targetH * 0.055f), 102);
}

void drawShopDemoShopkeeperShadow(SDL_Renderer* renderer, const AppState& state) {
    const float sx = shopDemoScreenXAtDepth(state, kShopkeeperX, kShopkeeperDepth);
    const float sy = shopDemoShopkeeperVisualY(state);
    const float targetH = shopDemoShopkeeperTargetHeight(state);
    drawShopDemoContactShadow(renderer, sx, sy - 1.0f * shopDemoSceneScaleY(state) * shopDemoWorldZoom(state), targetH * 0.45f, std::max(2.0f, targetH * 0.05f), 78);
}

void drawShopDemoPlayer(SDL_Renderer* renderer, const AppState& state) {
    const float sx = shopDemoRenderedPlayerScreenX(state);
    const float sy = shopDemoRenderedPlayerBaselineY(state);
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
        const float spriteScale = shopSpriteScaleForHeight(*playerSprite, shopDemoPlayerTargetHeight(state));
        drawSpriteAtAxis(renderer, *playerSprite, sx, sy, spriteScale, playerFlip);
        return;
    }

    const float fallbackH = shopDemoPlayerTargetHeight(state);
    const float fallbackScale = fallbackH / 52.0f;
    setColor(renderer, 72, 206, 150);
    fillRect(renderer, sx - 4.0f * fallbackScale, sy - 35.0f * fallbackScale, 8.0f * fallbackScale, 8.0f * fallbackScale);
    setColor(renderer, 44, 92, 132);
    fillRect(renderer, sx - 8.0f * fallbackScale, sy - 26.0f * fallbackScale, 16.0f * fallbackScale, 20.0f * fallbackScale);
    setColor(renderer, 34, 62, 84);
    fillRect(renderer, sx - 11.0f * fallbackScale, sy - 7.0f * fallbackScale, 8.0f * fallbackScale, 8.0f * fallbackScale);
    fillRect(renderer, sx + 3.0f * fallbackScale, sy - 7.0f * fallbackScale, 8.0f * fallbackScale, 8.0f * fallbackScale);
}

void drawShopDemoShopkeeper(SDL_Renderer* renderer, const AppState& state) {
    const TextureSprite& shopkeeper = state.shopDemo.layeredSceneV2Enabled && state.shopDemo.shopkeeperPoseV2.texture
        ? state.shopDemo.shopkeeperPoseV2
        : state.shopDemo.shopkeeperPose;
    if (shopkeeper.texture) {
        const float spriteScale = shopSpriteScaleForHeight(shopkeeper, shopDemoShopkeeperTargetHeight(state));
        drawSpriteAtAxis(
            renderer,
            shopkeeper,
            shopDemoScreenXAtDepth(state, kShopkeeperX, kShopkeeperDepth),
            shopDemoShopkeeperVisualY(state),
            spriteScale);
        return;
    }

    const float sx = shopDemoScreenXAtDepth(state, kShopkeeperX, kShopkeeperDepth);
    const float sy = shopDemoShopkeeperVisualY(state);
    const float fallbackScale = shopDemoShopkeeperTargetHeight(state) / 54.0f;
    setColor(renderer, 95, 54, 132);
    fillRect(renderer, sx - 10.0f * fallbackScale, sy - 40.0f * fallbackScale, 20.0f * fallbackScale, 30.0f * fallbackScale);
    setColor(renderer, 214, 198, 230);
    fillRect(renderer, sx - 5.0f * fallbackScale, sy - 52.0f * fallbackScale, 10.0f * fallbackScale, 11.0f * fallbackScale);
}

void drawShopDemoCounterFront(SDL_Renderer* renderer, const AppState& state) {
    const float visualDepth = 70.0f + (20.0f - 70.0f) * state.shopDemo.cinematicBlend;
    const float frontBottomY = state.shopDemo.perspectiveCameraEnabled
        ? shopDemoProjectGround(state, kShopCounterVisualCenterX, visualDepth).floorY
        : shopDemoCounterVisualBottomY(state);
    const auto counterAspect = [&]() {
        if (state.shopDemo.shopCounterFront.width > 0 && state.shopDemo.shopCounterFront.height > 0) {
            return static_cast<float>(state.shopDemo.shopCounterFront.width)
                / static_cast<float>(state.shopDemo.shopCounterFront.height);
        }
        return kShopCounterVisualDefaultAspect;
    };
    const float frontH = shopDemoCounterTargetHeight(state);
    const float visualW = frontH * counterAspect();
    const SDL_FRect dst{
        shopDemoScreenXAtDepth(state, kShopCounterVisualCenterX, visualDepth) - visualW * 0.5f,
        frontBottomY - frontH,
        visualW,
        frontH,
    };
    if (state.shopDemo.shopCounterFront.texture) {
        if (state.shopDemo.perspectiveCameraEnabled) {
            const float baseHeight = std::clamp(shopDemoLayoutRects(state).world.h * 0.18f, 68.0f, 88.0f);
            const float focusedCounterGain = state.shopDemo.shopOpen ? 0.49f : 0.10f;
            const float worldHeight = baseHeight * (1.0f + state.shopDemo.cinematicBlend * focusedCounterGain);
            const float worldWidth = worldHeight * counterAspect();
            const auto left = shopDemoProjectGround(
                state,
                kShopCounterVisualCenterX - worldWidth * 0.5f,
                visualDepth);
            const auto right = shopDemoProjectGround(
                state,
                kShopCounterVisualCenterX + worldWidth * 0.5f,
                visualDepth);
            const float leftHeight = worldHeight * left.scale;
            const float rightHeight = worldHeight * right.scale;
            const SDL_FColor white{ 1.0f, 1.0f, 1.0f, 1.0f };
            const SDL_Vertex vertices[4]{
                SDL_Vertex{ SDL_FPoint{ left.x, left.floorY - leftHeight }, white, SDL_FPoint{ 0.0f, 0.0f } },
                SDL_Vertex{ SDL_FPoint{ right.x, right.floorY - rightHeight }, white, SDL_FPoint{ 1.0f, 0.0f } },
                SDL_Vertex{ SDL_FPoint{ right.x, right.floorY }, white, SDL_FPoint{ 1.0f, 1.0f } },
                SDL_Vertex{ SDL_FPoint{ left.x, left.floorY }, white, SDL_FPoint{ 0.0f, 1.0f } },
            };
            constexpr int indices[6]{ 0, 1, 2, 0, 2, 3 };
            SDL_RenderGeometry(renderer, state.shopDemo.shopCounterFront.texture, vertices, 4, indices, 6);
            return;
        }
        SDL_RenderTexture(renderer, state.shopDemo.shopCounterFront.texture, nullptr, &dst);
        return;
    }

    setColor(renderer, 14, 10, 10, 242);
    fillRect(renderer, dst.x, dst.y, dst.w, dst.h);
    setColor(renderer, 68, 42, 29, 238);
    fillRect(renderer, dst.x + dst.w * 0.04f, dst.y + frontH * 0.12f, dst.w * 0.92f, frontH * 0.70f);
    setColor(renderer, 112, 70, 39, 235);
    fillRect(renderer, dst.x + dst.w * 0.08f, dst.y + frontH * 0.22f, dst.w * 0.84f, frontH * 0.22f);
    setColor(renderer, 48, 30, 22, 238);
    fillRect(renderer, dst.x + dst.w * 0.04f, dst.y + frontH * 0.72f, dst.w * 0.92f, frontH * 0.18f);
    setColor(renderer, 224, 198, 102, 235);
    fillRect(renderer, dst.x, dst.y, dst.w, std::max(1.0f, shopDemoSceneScaleY(state) * 2.0f));
    fillRect(renderer, dst.x + dst.w * 0.06f, dst.y + frontH * 0.55f, dst.w * 0.88f, std::max(1.0f, shopDemoSceneScaleY(state) * 2.0f));
    setColor(renderer, 92, 48, 126, 120);
    fillRect(renderer, dst.x + dst.w * 0.5f - 24.0f, dst.y + frontH * 0.28f, 48.0f, frontH * 0.36f);
    setColor(renderer, 132, 82, 210, 145);
    fillRect(renderer, dst.x + dst.w * 0.5f - 17.0f, dst.y + frontH * 0.42f, 34.0f, std::max(1.0f, shopDemoSceneScaleY(state) * 3.0f));
}
