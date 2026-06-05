#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

void drawVersusScreen(SDL_Renderer* renderer, const AppState& state) {
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    setColor(renderer, 28, 26, 36);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(kLogicalHeight));
    setColor(renderer, 42, 35, 52);
    fillRect(renderer, 0, 100, widthF, 42);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 18, 16, "DRAGON MUGEN CORE");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 20, 30, std::string(pendingModeTitle(state.pendingMode)) + " VS");

    drawPanel(renderer, 22, 62, 108, 96);
    drawPanel(renderer, widthF - 130.0f, 62, 108, 96);

    const TextureSprite* p1Portrait = state.characterLargePortrait.texture
        ? &state.characterLargePortrait
        : spriteAt(state.characterFaceSprites, sessionP1CharacterIndex(state));

    if (p1Portrait) {
        const float scale = std::min({ 1.0f, 92.0f / static_cast<float>(p1Portrait->width), 96.0f / static_cast<float>(p1Portrait->height) });
        drawSpriteTopLeft(
            renderer,
            *p1Portrait,
            76.0f - static_cast<float>(p1Portrait->width) * scale * 0.5f,
            76,
            scale);
    } else {
        setColor(renderer, 70, 132, 190);
        fillRect(renderer, 50, 86, 52, 48);
    }
    drawFixedOpponentSlot(renderer, widthF - 122.0f, 72, 76, 76, opponentSlotLabel(state));

    setColor(renderer, 222, 226, 232);
    debugText(renderer, 34, 144, compactSettingText(selectedCharacterName(state), 13));
    debugText(renderer, widthF - 100.0f, 144, compactSettingText(opponentDisplayName(state), 10));

    setColor(renderer, 230, 130, 120);
    debugTextCentered(renderer, centerX, 98, "VS");

    setColor(renderer, 155, 164, 174);
    debugText(renderer, 20, 184, "Stage: " + compactSettingText(selectedStageName(state), 26));
    if (state.fightSessionLoadFailed) {
        setColor(renderer, 230, 130, 120);
        debugText(renderer, 20, 204, "Load failed. ESC stage select");
    } else if (state.fightSessionPrepared) {
        debugText(renderer, 20, 204, "Ready. ENTER start now");
        debugText(renderer, 20, 216, "Auto-start after a short pause");
    } else {
        debugText(renderer, 20, 204, "Loading fighter and stage...");
        debugText(renderer, 20, 216, "Please wait");
    }

    SDL_RenderPresent(renderer);
}
