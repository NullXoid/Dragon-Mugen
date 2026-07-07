#include "CharacterSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cstddef>

namespace dragon {
namespace {

float selectVirtualWidth(const UiRenderContext& ui) {
    if (ui.logicalWidth >= 1000 || ui.logicalHeight >= 620) {
        return 640.0f;
    }
    return ui.logicalWidth <= 340 ? 320.0f : 426.0f;
}

float selectVirtualHeight(float virtualWidth) {
    return virtualWidth >= 600.0f ? 360.0f : 240.0f;
}

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
    float inset = 0.0f,
    float size = 29.0f) {
    const int pulse = confirmed ? 255 : 180 + ((frame / 6) % 40);
    setColor(renderer, r, static_cast<Uint8>(std::min<int>(pulse, g)), b);
    drawRect(renderer, x + inset, y + inset, size - inset * 2.0f, size - inset * 2.0f);
    if (confirmed) {
        drawRect(renderer, x + 1.0f + inset, y + 1.0f + inset, size - 2.0f - inset * 2.0f, size - 2.0f - inset * 2.0f);
    }
}

} // namespace

void drawCharacterSelectOverlay(const UiRenderContext& ui, const CharacterSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float virtualWidth = selectVirtualWidth(ui);
    const float virtualHeight = selectVirtualHeight(virtualWidth);
    ScopedVirtualCanvas virtualCanvas(ui, virtualWidth, virtualHeight);
    const float widthF = virtualWidth;
    const float heightF = virtualHeight;
    const float centerX = widthF * 0.5f;
    const bool classic = widthF <= 340.0f;
    const bool expanded = widthF >= 600.0f;
    const float titleY = expanded ? 12.0f : 8.0f;
    const float subtitleY = expanded ? 27.0f : 20.0f;
    const float profileY = expanded ? 42.0f : 32.0f;
    const float portraitW = expanded ? 142.0f : (classic ? 86.0f : 118.0f);
    const float portraitH = expanded ? 124.0f : (classic ? 92.0f : 116.0f);
    const float portraitY = expanded ? 66.0f : (classic ? 48.0f : 44.0f);
    const float leftPortraitX = expanded ? 28.0f : (classic ? 10.0f : 16.0f);
    const float rightPortraitX = widthF - leftPortraitX - portraitW;
    const float gridCellSize = expanded ? 24.0f : (classic ? 20.0f : 22.0f);
    const float gridCellSpacing = expanded ? 5.0f : (classic ? 2.0f : 3.0f);
    const float gridY = expanded ? 202.0f : (classic ? 159.0f : 157.0f);
    const float footerStatusY = expanded ? heightF - 58.0f : (classic ? 202.0f : 204.0f);
    const float footerStageY = expanded ? heightF - 42.0f : (classic ? 214.0f : 216.0f);
    const float footerControlsY = expanded ? heightF - 24.0f : (classic ? 226.0f : 228.0f);

    setColor(renderer, 235, 240, 248);
    debugTextCentered(renderer, centerX, titleY, view.modeTitle);
    setColor(renderer, 246, 214, 92);
    debugTextCentered(renderer, centerX, subtitleY, view.showP2Cursor ? "P1 / P2 SELECT YOUR FIGHTERS" : view.activePlayerLabel + " SELECT YOUR FIGHTER");
    if (!view.profileName.empty()) {
        setColor(renderer, 128, 171, 225);
        std::string profileLine = "P1 " + view.profileName;
        if (!view.opponentProfileName.empty()) {
            profileLine += "   P2 " + view.opponentProfileName;
        }
        debugTextCentered(renderer, centerX, profileY, fitDebugText("PROFILE " + profileLine, classic ? 30 : (expanded ? 54 : 42)));
    }

    if (view.cells.empty()) {
        setColor(renderer, 235, 110, 100);
        debugTextCentered(renderer, centerX, heightF * 0.45f, "NO CHARACTERS IN SELECT.DEF");
        setColor(renderer, 156, 166, 180);
        debugTextCentered(renderer, centerX, footerControlsY, "ESC mode select");
        return;
    }

    setColor(renderer, 7, 10, 14, 224);
    fillRect(renderer, leftPortraitX - 2.0f, portraitY - 2.0f, portraitW + 4.0f, portraitH + 4.0f);
    setColor(renderer, 86, 96, 116);
    drawRect(renderer, leftPortraitX - 2.0f, portraitY - 2.0f, portraitW + 4.0f, portraitH + 4.0f);
    if (hasTexture(view.selectedPortrait) && view.selectedPortrait.width > 0 && view.selectedPortrait.height > 0) {
        const float portraitScale = std::min({
            1.0f,
            portraitW / static_cast<float>(view.selectedPortrait.width),
            portraitH / static_cast<float>(view.selectedPortrait.height),
        });
        const float portraitX = leftPortraitX + (portraitW - static_cast<float>(view.selectedPortrait.width) * portraitScale) * 0.5f;
        const float portraitDrawY = portraitY + (portraitH - static_cast<float>(view.selectedPortrait.height) * portraitScale) * 0.5f;
        drawSpriteTopLeft(renderer, view.selectedPortrait, portraitX, portraitDrawY, portraitScale);
    } else {
        setColor(renderer, 34, 38, 46, 210);
        fillRect(renderer, leftPortraitX, portraitY, portraitW, portraitH);
        setColor(renderer, 94, 108, 130);
        drawRect(renderer, leftPortraitX, portraitY, portraitW, portraitH);
    }
    const bool opponentHasPortrait = hasTexture(view.opponentPortrait)
        && view.opponentPortrait.width > 0
        && view.opponentPortrait.height > 0;
    setColor(renderer, 7, 10, 14, 224);
    fillRect(renderer, rightPortraitX - 2.0f, portraitY - 2.0f, portraitW + 4.0f, portraitH + 4.0f);
    setColor(renderer, 86, 96, 116);
    drawRect(renderer, rightPortraitX - 2.0f, portraitY - 2.0f, portraitW + 4.0f, portraitH + 4.0f);
    if (opponentHasPortrait) {
        const float portraitScale = std::min({
            1.0f,
            portraitW / static_cast<float>(view.opponentPortrait.width),
            portraitH / static_cast<float>(view.opponentPortrait.height),
        });
        const float portraitX = rightPortraitX + (portraitW - static_cast<float>(view.opponentPortrait.width) * portraitScale) * 0.5f;
        const float portraitDrawY = portraitY + (portraitH - static_cast<float>(view.opponentPortrait.height) * portraitScale) * 0.5f;
        drawSpriteTopLeft(renderer, view.opponentPortrait, portraitX, portraitDrawY, portraitScale);
    } else {
        drawFixedOpponentSlot(renderer, rightPortraitX, portraitY, portraitW, portraitH, view.opponentIsDummy);
    }

    setColor(renderer, 235, 240, 248);
    debugText(renderer, leftPortraitX, portraitY + portraitH + 6.0f, fitDebugText(view.selectedName, classic ? 12 : (expanded ? 18 : 16)));
    if (!view.selectedProgressionLabel.empty()) {
        setColor(renderer, 120, 230, 170);
        debugText(renderer, leftPortraitX, portraitY + portraitH + 18.0f, fitDebugText(view.selectedProgressionLabel, classic ? 14 : (expanded ? 22 : 18)));
    }
    setColor(renderer, 235, 240, 248);
    if (opponentHasPortrait) {
        const std::string opponentName = fitDebugText(view.opponentName, classic ? 12 : (expanded ? 18 : 16));
        debugText(
            renderer,
            rightPortraitX + portraitW - static_cast<float>(opponentName.size() * 8),
            portraitY + portraitH + 6.0f,
            opponentName);
    } else {
        debugTextCentered(renderer, rightPortraitX + portraitW * 0.5f, portraitY + portraitH + 6.0f, fitDebugText(view.opponentName, classic ? 12 : (expanded ? 18 : 16)));
    }
    if (!view.opponentProgressionLabel.empty()) {
        setColor(renderer, 120, 230, 170);
        const std::string progression = fitDebugText(view.opponentProgressionLabel, classic ? 14 : (expanded ? 22 : 18));
        debugText(renderer, rightPortraitX + portraitW - static_cast<float>(progression.size() * 8), portraitY + portraitH + 18.0f, progression);
    }

    const int columns = std::max(1, view.columns);
    const float gridWidth = static_cast<float>(columns) * gridCellSize
        + static_cast<float>(columns - 1) * gridCellSpacing;
    const float gridX = centerX - gridWidth * 0.5f;

    for (int i = 0; i < static_cast<int>(view.cells.size()); ++i) {
        const int column = i % columns;
        const int row = i / columns;
        const float cellX = gridX + static_cast<float>(column) * (gridCellSize + gridCellSpacing);
        const float cellY = gridY + static_cast<float>(row) * (gridCellSize + gridCellSpacing);

        setColor(renderer, 22, 26, 32, 230);
        fillRect(renderer, cellX, cellY, gridCellSize, gridCellSize);
        setColor(renderer, 92, 110, 136);
        drawRect(renderer, cellX, cellY, gridCellSize, gridCellSize);
        if (hasTexture(view.cellSprite) && view.cellSprite.width > 0 && view.cellSprite.height > 0) {
            const float cellScale = std::min({
                gridCellSize / static_cast<float>(view.cellSprite.width),
                gridCellSize / static_cast<float>(view.cellSprite.height),
            });
            drawSpriteTopLeft(renderer, view.cellSprite, cellX, cellY, cellScale);
        }

        const auto& cell = view.cells[static_cast<std::size_t>(i)];
        if (cell.occupied && hasTexture(cell.icon) && cell.icon.width > 0 && cell.icon.height > 0) {
            const float scale = std::min({
                1.0f,
                (gridCellSize - 2.0f) / static_cast<float>(cell.icon.width),
                (gridCellSize - 2.0f) / static_cast<float>(cell.icon.height),
            });
            const float iconX = cellX + (gridCellSize - static_cast<float>(cell.icon.width) * scale) * 0.5f;
            const float iconY = cellY + (gridCellSize - static_cast<float>(cell.icon.height) * scale) * 0.5f;
            drawSpriteTopLeft(renderer, cell.icon, iconX, iconY, scale);
        }
    }

    const int safeSelectedCell = std::clamp(view.selectedCell, 0, static_cast<int>(view.cells.size()) - 1);
    const float cursorX = gridX + static_cast<float>(safeSelectedCell % columns) * (gridCellSize + gridCellSpacing) - 1.0f;
    const float cursorY = gridY + static_cast<float>(safeSelectedCell / columns) * (gridCellSize + gridCellSpacing) - 1.0f;
    drawCellCursor(renderer, cursorX, cursorY, 240, 220, 70, view.p1Confirmed, view.frame, 0.0f, gridCellSize + 2.0f);

    if (view.showP2Cursor) {
        const int safeP2Cell = std::clamp(view.p2SelectedCell, 0, static_cast<int>(view.cells.size()) - 1);
        const float p2CursorX = gridX + static_cast<float>(safeP2Cell % columns) * (gridCellSize + gridCellSpacing) - 1.0f;
        const float p2CursorY = gridY + static_cast<float>(safeP2Cell / columns) * (gridCellSize + gridCellSpacing) - 1.0f;
        const float inset = safeP2Cell == safeSelectedCell ? 3.0f : 0.0f;
        drawCellCursor(renderer, p2CursorX, p2CursorY, 80, 175, 255, view.p2Confirmed, view.frame, inset, gridCellSize + 2.0f);
    }

    setColor(renderer, 238, 210, 94);
    if (view.showP2Cursor) {
        debugTextCentered(
            renderer,
            centerX,
            footerStatusY,
            std::string("P1 ") + (view.p1Confirmed ? "OK" : "choose") + "   P2 " + (view.p2Confirmed ? "OK" : "choose"));
    } else {
        debugTextCentered(renderer, centerX, footerStatusY, view.activePlayerLabel);
    }
    setColor(renderer, 210, 218, 230);
    debugTextCentered(renderer, centerX, footerStageY, fitDebugText("STAGE: " + view.preferredStageLabel, classic ? 32 : 46));

    setColor(renderer, 156, 166, 180);
    debugTextCentered(
        renderer,
        centerX,
        footerControlsY,
        view.showP2Cursor ? "P1 arrows/ENTER  P2 IJKL/;  ESC back" : "ARROWS choose  ENTER stage  ESC back");
}

} // namespace dragon
