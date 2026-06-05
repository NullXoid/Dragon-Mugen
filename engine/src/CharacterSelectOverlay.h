#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

void drawCharacterSelect(SDL_Renderer* renderer, const AppState& state) {
    drawSelectBackground(renderer, state);

    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);
    setColor(renderer, 235, 240, 248);
    debugTextCentered(renderer, centerX, 8, std::string(pendingModeTitle(state.pendingMode)));
    setColor(renderer, 246, 214, 92);
    debugTextCentered(renderer, centerX, 20, "SELECT YOUR FIGHTER");

    if (state.selection.characters.empty()) {
        setColor(renderer, 235, 110, 100);
        debugTextCentered(renderer, centerX, 96, "NO CHARACTERS IN SELECT.DEF");
        setColor(renderer, 156, 166, 180);
        debugTextCentered(renderer, centerX, 226, "ESC mode select");
        SDL_RenderPresent(renderer);
        return;
    }

    const int selected = std::clamp(state.selection.selectedCharacter, 0, static_cast<int>(state.selection.characters.size()) - 1);
    const auto& character = state.selection.characters[static_cast<size_t>(selected)];
    const TextureSprite* face = spriteAt(state.characterFaceSprites, selected);
    if (face) {
        const float portraitScale = std::min({ 1.0f, 120.0f / static_cast<float>(face->width), 140.0f / static_cast<float>(face->height) });
        drawSpriteTopLeft(renderer, *face, 18, 30, portraitScale);
    } else {
        setColor(renderer, 34, 38, 46, 210);
        fillRect(renderer, 18, 30, 120, 140);
        setColor(renderer, 94, 108, 130);
        drawRect(renderer, 18, 30, 120, 140);
    }
    const float opponentPortraitX = widthF - 138.0f;
    drawFixedOpponentSlot(renderer, opponentPortraitX, 30, 120, 140, opponentSlotLabel(state.pendingMode));

    const std::string p1Name = compactSettingText(character.displayName, 15);
    const std::string p2Name(opponentSlotLabel(state.pendingMode));
    setColor(renderer, 235, 240, 248);
    debugText(renderer, 10, 154, p1Name);
    debugText(renderer, widthF - 10.0f - static_cast<float>(p2Name.size() * 8), 154, p2Name);

    static constexpr float kCellSize = 27.0f;
    static constexpr float kCellSpacing = 2.0f;
    static constexpr float kGridY = 170.0f;
    constexpr float kGridWidth = static_cast<float>(kCharacterSelectColumns) * kCellSize
        + static_cast<float>(kCharacterSelectColumns - 1) * kCellSpacing;
    const float gridX = centerX - kGridWidth * 0.5f;

    const int page = selected / kCharacterSelectPageSize;
    const int firstIndex = page * kCharacterSelectPageSize;
    const int lastIndex = std::min(firstIndex + kCharacterSelectPageSize, static_cast<int>(state.selection.characters.size()));

    for (int i = firstIndex; i < lastIndex; ++i) {
        const int local = i - firstIndex;
        const int column = local % kCharacterSelectColumns;
        const int row = local / kCharacterSelectColumns;
        const float cellX = gridX + static_cast<float>(column) * (kCellSize + kCellSpacing);
        const float cellY = kGridY + static_cast<float>(row) * (kCellSize + kCellSpacing);

        if (state.systemScreens.selectCell.texture) {
            drawSpriteTopLeft(renderer, state.systemScreens.selectCell, cellX, cellY);
        } else {
            setColor(renderer, 22, 26, 32, 230);
            fillRect(renderer, cellX, cellY, kCellSize, kCellSize);
            setColor(renderer, 92, 110, 136);
            drawRect(renderer, cellX, cellY, kCellSize, kCellSize);
        }

        if (const TextureSprite* icon = spriteAt(state.characterIconSprites, i)) {
            const float scale = std::min({ 1.0f, 25.0f / static_cast<float>(icon->width), 25.0f / static_cast<float>(icon->height) });
            const float iconX = cellX + (kCellSize - static_cast<float>(icon->width) * scale) * 0.5f;
            const float iconY = cellY + (kCellSize - static_cast<float>(icon->height) * scale) * 0.5f;
            drawSpriteTopLeft(renderer, *icon, iconX, iconY, scale);
        }
    }

    const int cursorLocal = selected - firstIndex;
    const float cursorX = gridX + static_cast<float>(cursorLocal % kCharacterSelectColumns) * (kCellSize + kCellSpacing) - 1.0f;
    const float cursorY = kGridY + static_cast<float>(cursorLocal / kCharacterSelectColumns) * (kCellSize + kCellSpacing) - 1.0f;
    if (state.systemScreens.selectP1Cursor.texture) {
        drawSpriteTopLeft(renderer, state.systemScreens.selectP1Cursor, cursorX, cursorY);
    } else {
        const int pulse = 180 + ((state.frame / 6) % 40);
        setColor(renderer, 240, static_cast<Uint8>(pulse), 70);
        drawRect(renderer, cursorX, cursorY, 29, 29);
    }

    setColor(renderer, 238, 210, 94);
    debugTextCentered(renderer, centerX, 208, "P1");
    setColor(renderer, 210, 218, 230);
    debugTextCentered(renderer, centerX, 220, "STAGE: " + compactSettingText(characterPreferredStageName(state.selection, selected), 22));

    setColor(renderer, 156, 166, 180);
    debugTextCentered(renderer, centerX, 232, "ARROWS choose  ENTER stage  ESC back");
    SDL_RenderPresent(renderer);
}
