#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp-local AppState, TextureSprite, and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

void drawTitleBackground(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 0, 0, 0);
    SDL_RenderClear(renderer);

    const int width = logicalWidth(state);
    const float widthF = static_cast<float>(width);
    const float centerX = screenCenterX(state);
    const float motifX = motifOriginX(state);
    const auto& system = state.systemScreens;
    if (!system.titleTop.texture) {
        setColor(renderer, 14, 16, 20);
        fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
        return;
    }

    const float topScroll = static_cast<float>(state.frame % std::max(1, system.titleTop.width));
    const float topX = centerX - static_cast<float>(system.titleTop.axisX) - topScroll;
    drawTiledSpriteCoverX(renderer, system.titleTop, topX, 10, width, 2);

    if (system.titleFloor.texture) {
        drawParallaxFloorSprite(renderer, system.titleFloor, motifX, 145, 400.0f, 1200.0f, width, state.frame);
    }
    if (system.titleShade.texture) {
        drawSpriteTopLeftWithBlend(renderer, system.titleShade, motifX - 160.0f, 145.0f, SDL_BLENDMODE_MUL, 180);
    }

    setColor(renderer, 0, 0, 0);
    fillRect(renderer, 0, 0, widthF, 12);
    fillRect(renderer, 0, 220, widthF, 20);
}

void drawSelectBackground(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 0, 0, 0);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    const auto& system = state.systemScreens;
    if (system.selectBackdrop.texture) {
        const float backdropOffset = -static_cast<float>((state.frame / 3) % std::max(1, system.selectBackdrop.width));
        drawTiledSprite(renderer, system.selectBackdrop, backdropOffset, 0, 3, 2);
    } else {
        setColor(renderer, 18, 22, 30);
        fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    }

    if (system.selectShadow.texture) {
        drawSpriteTopLeft(renderer, system.selectShadow, 0, 128);
    }

    if (system.selectTitleA.texture) {
        const float stripOffset = -static_cast<float>(state.frame % std::max(1, system.selectTitleA.width));
        drawTiledSprite(renderer, system.selectTitleA, stripOffset, 0, 4, 1);
    }
    if (system.selectTitleB.texture) {
        const float stripOffset = -static_cast<float>((state.frame / 2) % std::max(1, system.selectTitleB.width));
        drawTiledSprite(renderer, system.selectTitleB, stripOffset, 14, 4, 1);
    }
    if (system.selectTitleC.texture) {
        const float stripOffset = -static_cast<float>((state.frame / 2) % std::max(1, system.selectTitleC.width));
        drawTiledSprite(renderer, system.selectTitleC, stripOffset, 224, 2, 1);
    }

    setColor(renderer, 0, 0, 0, 150);
    fillRect(renderer, 0, 160, widthF, 60);
}

void drawFixedOpponentSlot(
    SDL_Renderer* renderer,
    float x,
    float y,
    float width,
    float height,
    std::string_view label) {
    setColor(renderer, 12, 14, 18);
    fillRect(renderer, x, y, width, height);
    setColor(renderer, 54, 62, 76);
    drawRect(renderer, x, y, width, height);

    setColor(renderer, 74, 82, 98);
    fillRect(renderer, x + width * 0.38f, y + height * 0.18f, width * 0.24f, height * 0.22f);
    fillRect(renderer, x + width * 0.28f, y + height * 0.44f, width * 0.44f, height * 0.38f);
    setColor(renderer, 34, 38, 46);
    fillRect(renderer, x + width * 0.34f, y + height * 0.50f, width * 0.32f, height * 0.30f);

    setColor(renderer, 150, 160, 176);
    debugTextCentered(renderer, x + width * 0.5f, y + height * 0.86f, std::string(label));
}
