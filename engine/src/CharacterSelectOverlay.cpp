#include "CharacterSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cstddef>

namespace dragon {
namespace {

void drawFixedOpponentSlot(
    SDL_Renderer* renderer,
    float x,
    float y,
    float width,
    float height,
    bool dummySlot) {
    setColor(renderer, 12, 14, 18);
    fillRect(renderer, x, y, width, height);
    setColor(renderer, 54, 62, 76);
    drawRect(renderer, x, y, width, height);

    if (dummySlot) {
        setColor(renderer, 30, 24, 20);
        fillRect(renderer, x + width * 0.22f, y + height * 0.70f, width * 0.56f, height * 0.08f);
        setColor(renderer, 128, 90, 54);
        fillRect(renderer, x + width * 0.48f, y + height * 0.34f, width * 0.05f, height * 0.40f);
        setColor(renderer, 168, 118, 66);
        fillRect(renderer, x + width * 0.40f, y + height * 0.38f, width * 0.20f, height * 0.28f);
        setColor(renderer, 222, 180, 102);
        drawRect(renderer, x + width * 0.40f, y + height * 0.38f, width * 0.20f, height * 0.28f);
        setColor(renderer, 96, 62, 42);
        fillRect(renderer, x + width * 0.30f, y + height * 0.43f, width * 0.16f, height * 0.07f);
        fillRect(renderer, x + width * 0.54f, y + height * 0.43f, width * 0.16f, height * 0.07f);
        setColor(renderer, 186, 134, 78);
        fillRect(renderer, x + width * 0.43f, y + height * 0.20f, width * 0.14f, height * 0.14f);
        setColor(renderer, 234, 200, 116);
        drawRect(renderer, x + width * 0.43f, y + height * 0.20f, width * 0.14f, height * 0.14f);
        setColor(renderer, 72, 42, 34);
        fillRect(renderer, x + width * 0.455f, y + height * 0.245f, width * 0.07f, height * 0.025f);
        setColor(renderer, 72, 84, 102);
        fillRect(renderer, x + width * 0.31f, y + height * 0.82f, width * 0.38f, height * 0.05f);
        return;
    }

    setColor(renderer, 74, 82, 98);
    fillRect(renderer, x + width * 0.38f, y + height * 0.18f, width * 0.24f, height * 0.22f);
    fillRect(renderer, x + width * 0.28f, y + height * 0.44f, width * 0.44f, height * 0.38f);
    setColor(renderer, 34, 38, 46);
    fillRect(renderer, x + width * 0.34f, y + height * 0.50f, width * 0.32f, height * 0.30f);
}

void drawCellCursor(
    SDL_Renderer* renderer,
    float x,
    float y,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    bool confirmed,
    int frame,
    float inset = 0.0f) {
    const int pulse = confirmed ? 255 : 180 + ((frame / 6) % 40);
    setColor(renderer, r, static_cast<Uint8>(std::min<int>(pulse, g)), b);
    drawRect(renderer, x + inset, y + inset, 29.0f - inset * 2.0f, 29.0f - inset * 2.0f);
    if (confirmed) {
        drawRect(renderer, x + 1.0f + inset, y + 1.0f + inset, 27.0f - inset * 2.0f, 27.0f - inset * 2.0f);
    }
}

} // namespace

void drawCharacterSelectOverlay(const UiRenderContext& ui, const CharacterSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float centerX = widthF * 0.5f;

    setColor(renderer, 235, 240, 248);
    debugTextCentered(renderer, centerX, 8, view.modeTitle);
    setColor(renderer, 246, 214, 92);
    debugTextCentered(renderer, centerX, 20, view.showP2Cursor ? "P1 / P2 SELECT YOUR FIGHTERS" : view.activePlayerLabel + " SELECT YOUR FIGHTER");

    if (view.cells.empty()) {
        setColor(renderer, 235, 110, 100);
        debugTextCentered(renderer, centerX, 96, "NO CHARACTERS IN SELECT.DEF");
        setColor(renderer, 156, 166, 180);
        debugTextCentered(renderer, centerX, 226, "ESC mode select");
        return;
    }

    if (hasTexture(view.selectedPortrait) && view.selectedPortrait.width > 0 && view.selectedPortrait.height > 0) {
        const float portraitScale = std::min({
            1.0f,
            120.0f / static_cast<float>(view.selectedPortrait.width),
            140.0f / static_cast<float>(view.selectedPortrait.height),
        });
        drawSpriteTopLeft(renderer, view.selectedPortrait, 18, 30, portraitScale);
    } else {
        setColor(renderer, 34, 38, 46, 210);
        fillRect(renderer, 18, 30, 120, 140);
        setColor(renderer, 94, 108, 130);
        drawRect(renderer, 18, 30, 120, 140);
    }
    const float opponentPortraitX = widthF - 138.0f;
    const bool opponentHasPortrait = hasTexture(view.opponentPortrait)
        && view.opponentPortrait.width > 0
        && view.opponentPortrait.height > 0;
    if (opponentHasPortrait) {
        const float portraitScale = std::min({
            1.0f,
            120.0f / static_cast<float>(view.opponentPortrait.width),
            140.0f / static_cast<float>(view.opponentPortrait.height),
        });
        drawSpriteTopLeft(renderer, view.opponentPortrait, opponentPortraitX, 30, portraitScale);
    } else {
        drawFixedOpponentSlot(renderer, opponentPortraitX, 30, 120, 140, view.opponentIsDummy);
    }

    setColor(renderer, 235, 240, 248);
    debugText(renderer, 10, 154, view.selectedName);
    if (opponentHasPortrait) {
        debugText(renderer, widthF - 10.0f - static_cast<float>(view.opponentName.size() * 8), 154, view.opponentName);
    } else {
        debugTextCentered(renderer, opponentPortraitX + 60.0f, 154, view.opponentName);
    }

    static constexpr float kCellSize = 27.0f;
    static constexpr float kCellSpacing = 2.0f;
    static constexpr float kGridY = 170.0f;
    const int columns = std::max(1, view.columns);
    const float gridWidth = static_cast<float>(columns) * kCellSize
        + static_cast<float>(columns - 1) * kCellSpacing;
    const float gridX = centerX - gridWidth * 0.5f;

    for (int i = 0; i < static_cast<int>(view.cells.size()); ++i) {
        const int column = i % columns;
        const int row = i / columns;
        const float cellX = gridX + static_cast<float>(column) * (kCellSize + kCellSpacing);
        const float cellY = kGridY + static_cast<float>(row) * (kCellSize + kCellSpacing);

        if (hasTexture(view.cellSprite)) {
            drawSpriteTopLeft(renderer, view.cellSprite, cellX, cellY);
        } else {
            setColor(renderer, 22, 26, 32, 230);
            fillRect(renderer, cellX, cellY, kCellSize, kCellSize);
            setColor(renderer, 92, 110, 136);
            drawRect(renderer, cellX, cellY, kCellSize, kCellSize);
        }

        const auto& cell = view.cells[static_cast<std::size_t>(i)];
        if (cell.occupied && hasTexture(cell.icon) && cell.icon.width > 0 && cell.icon.height > 0) {
            const float scale = std::min({
                1.0f,
                25.0f / static_cast<float>(cell.icon.width),
                25.0f / static_cast<float>(cell.icon.height),
            });
            const float iconX = cellX + (kCellSize - static_cast<float>(cell.icon.width) * scale) * 0.5f;
            const float iconY = cellY + (kCellSize - static_cast<float>(cell.icon.height) * scale) * 0.5f;
            drawSpriteTopLeft(renderer, cell.icon, iconX, iconY, scale);
        }
    }

    const int safeSelectedCell = std::clamp(view.selectedCell, 0, static_cast<int>(view.cells.size()) - 1);
    const float cursorX = gridX + static_cast<float>(safeSelectedCell % columns) * (kCellSize + kCellSpacing) - 1.0f;
    const float cursorY = kGridY + static_cast<float>(safeSelectedCell / columns) * (kCellSize + kCellSpacing) - 1.0f;
    if (hasTexture(view.cursorSprite)) {
        drawSpriteTopLeft(renderer, view.cursorSprite, cursorX, cursorY);
    } else {
        drawCellCursor(renderer, cursorX, cursorY, 240, 220, 70, view.p1Confirmed, view.frame);
    }

    if (view.showP2Cursor) {
        const int safeP2Cell = std::clamp(view.p2SelectedCell, 0, static_cast<int>(view.cells.size()) - 1);
        const float p2CursorX = gridX + static_cast<float>(safeP2Cell % columns) * (kCellSize + kCellSpacing) - 1.0f;
        const float p2CursorY = kGridY + static_cast<float>(safeP2Cell / columns) * (kCellSize + kCellSpacing) - 1.0f;
        const float inset = safeP2Cell == safeSelectedCell ? 3.0f : 0.0f;
        drawCellCursor(renderer, p2CursorX, p2CursorY, 80, 175, 255, view.p2Confirmed, view.frame, inset);
    }

    setColor(renderer, 238, 210, 94);
    if (view.showP2Cursor) {
        debugTextCentered(
            renderer,
            centerX,
            208,
            std::string("P1 ") + (view.p1Confirmed ? "OK" : "choose") + "   P2 " + (view.p2Confirmed ? "OK" : "choose"));
    } else {
        debugTextCentered(renderer, centerX, 208, view.activePlayerLabel);
    }
    setColor(renderer, 210, 218, 230);
    debugTextCentered(renderer, centerX, 220, "STAGE: " + view.preferredStageLabel);

    setColor(renderer, 156, 166, 180);
    debugTextCentered(
        renderer,
        centerX,
        232,
        view.showP2Cursor ? "P1 arrows/ENTER  P2 IJKL/;  ESC back" : "ARROWS choose  ENTER stage  ESC back");
}

} // namespace dragon
