#include "CharacterSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
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
    float inset = 0.0f,
    float size = 29.0f) {
    const int pulse = confirmed ? 255 : 180 + ((frame / 6) % 40);
    setColor(renderer, r, static_cast<Uint8>(std::min<int>(pulse, g)), b);
    drawRect(renderer, x + inset, y + inset, size - inset * 2.0f, size - inset * 2.0f);
    if (confirmed) {
        drawRect(renderer, x + 1.0f + inset, y + 1.0f + inset, size - 2.0f - inset * 2.0f, size - 2.0f - inset * 2.0f);
    }
}

float textWidth(const std::string& text) {
    return static_cast<float>(text.size() * 8);
}

void drawSelectText(SDL_Renderer* renderer, float x, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    setColor(renderer, 2, 4, 8, 230);
    debugText(renderer, x + 1.0f, y + 1.0f, text);
    setColor(renderer, r, g, b);
    debugText(renderer, x, y, text);
}

void drawSelectTextCentered(SDL_Renderer* renderer, float centerX, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    drawSelectText(renderer, centerX - textWidth(text) * 0.5f, y, text, r, g, b);
}

void drawSelectFrame(SDL_Renderer* renderer, float x, float y, float w, float h, bool selected = false) {
    setColor(renderer, 4, 8, 13, 224);
    fillRect(renderer, x + 4.0f, y + 5.0f, w, h);
    setColor(renderer, 7, 16, 25, 232);
    fillRect(renderer, x, y, w, h);
    setColor(renderer, 45, 151, 148, 230);
    drawRect(renderer, x, y, w, h);
    if (selected) {
        setColor(renderer, 231, 195, 90, 235);
        drawRect(renderer, x + 2.0f, y + 2.0f, w - 4.0f, h - 4.0f);
    }
}

void drawPortraitBox(SDL_Renderer* renderer, const UiSpriteView& sprite, float x, float y, float w, float h) {
    setColor(renderer, 7, 10, 14, 224);
    fillRect(renderer, x, y, w, h);
    setColor(renderer, 54, 62, 76);
    drawRect(renderer, x, y, w, h);

    if (!hasTexture(sprite) || sprite.width <= 0 || sprite.height <= 0) {
        setColor(renderer, 34, 38, 46, 210);
        fillRect(renderer, x + 2.0f, y + 2.0f, w - 4.0f, h - 4.0f);
        return;
    }

    SDL_FRect src{
        0.0f,
        0.0f,
        static_cast<float>(sprite.width),
        static_cast<float>(sprite.height),
    };
    if (sprite.height > sprite.width * 1.25f) {
        src.x = static_cast<float>(sprite.width) * 0.30f;
        src.w = static_cast<float>(sprite.width) * 0.40f;
        src.y = static_cast<float>(sprite.height) * 0.03f;
        src.h = static_cast<float>(sprite.height) * 0.24f;
    }

    const float scale = std::min((w - 6.0f) / src.w, (h - 6.0f) / src.h);
    SDL_FRect dst{
        x + (w - src.w * scale) * 0.5f,
        y + (h - src.h * scale) * 0.5f,
        src.w * scale,
        src.h * scale,
    };
    SDL_RenderTextureRotated(renderer, sprite.texture, &src, &dst, 0.0, nullptr, SDL_FLIP_NONE);
}

void drawWaveOpponentSlot(SDL_Renderer* renderer, float x, float y, float w, float h) {
    setColor(renderer, 12, 14, 18);
    fillRect(renderer, x, y, w, h);
    setColor(renderer, 54, 62, 76);
    drawRect(renderer, x, y, w, h);
    const float laneX = x + w * 0.18f;
    const float laneW = w * 0.64f;
    setColor(renderer, 20, 26, 34);
    fillRect(renderer, laneX, y + h * 0.70f, laneW, h * 0.10f);
    setColor(renderer, 81, 210, 198, 210);
    fillRect(renderer, laneX, y + h * 0.30f, laneW * 0.28f, h * 0.08f);
    fillRect(renderer, laneX + laneW * 0.36f, y + h * 0.46f, laneW * 0.28f, h * 0.08f);
    setColor(renderer, 231, 195, 90, 225);
    fillRect(renderer, laneX + laneW * 0.72f, y + h * 0.32f, laneW * 0.16f, h * 0.16f);
    setColor(renderer, 198, 79, 85, 210);
    fillRect(renderer, laneX + laneW * 0.50f, y + h * 0.54f, laneW * 0.34f, h * 0.04f);
}

} // namespace

void drawCharacterSelectOverlay(const UiRenderContext& ui, const CharacterSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    constexpr float widthF = 640.0f;
    constexpr float heightF = 360.0f;
    ScopedVirtualCanvas virtualCanvas(ui, widthF, heightF);
    const float centerX = widthF * 0.5f;
    constexpr float topBarH = 54.0f;
    constexpr float panelX = 24.0f;
    constexpr float panelY = 66.0f;
    const float panelW = widthF - panelX * 2.0f;
    constexpr float panelH = heightF - panelY - 12.0f;
    constexpr float cardGap = 34.0f;
    constexpr float cardW = 214.0f;
    constexpr float cardH = 148.0f;
    constexpr float cardY = 84.0f;
    const float leftCardX = centerX - cardGap * 0.5f - cardW;
    const float rightCardX = centerX + cardGap * 0.5f;
    constexpr float portraitPad = 12.0f;
    constexpr float portraitBoxY = cardY + 32.0f;
    constexpr float portraitBoxH = 82.0f;
    constexpr float gridCellSize = 30.0f;
    constexpr float gridCellSpacing = 5.0f;
    constexpr float gridY = 242.0f;
    constexpr float footerStatusY = heightF - 60.0f;
    constexpr float footerStageY = heightF - 42.0f;
    constexpr float footerControlsY = heightF - 24.0f;
    constexpr int titleMaxChars = 58;

    setColor(renderer, 4, 9, 16, 232);
    fillRect(renderer, 0.0f, 0.0f, widthF, topBarH);
    setColor(renderer, 198, 79, 85, 235);
    fillRect(renderer, 0.0f, topBarH - 2.0f, widthF, 2.0f);

    drawSelectText(renderer, 20.0f, 14.0f, fitDebugText(view.modeTitle, 22), 231, 195, 90);
    drawSelectTextCentered(renderer, centerX, 14.0f, "CHARACTER SELECT", 81, 210, 198);
    const std::string selectLine = view.showP2Cursor ? "P1 / P2 SELECT YOUR FIGHTERS" : view.activePlayerLabel + " SELECT YOUR FIGHTER";
    drawSelectTextCentered(renderer, centerX, 31.0f, fitDebugText(selectLine, titleMaxChars), 246, 226, 112);
    if (!view.profileName.empty()) {
        std::string profileLine = "P1 " + view.profileName;
        if (!view.opponentProfileName.empty()) {
            profileLine += "   P2 " + view.opponentProfileName;
        }
        drawSelectTextCentered(renderer, centerX, 44.0f, fitDebugText("PROFILE " + profileLine, titleMaxChars), 128, 171, 225);
    }

    drawSelectFrame(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 198, 79, 85, 220);
    fillRect(renderer, panelX + 4.0f, panelY + 20.0f, panelW - 8.0f, 2.0f);

    if (view.cells.empty()) {
        drawSelectTextCentered(renderer, centerX, heightF * 0.45f, "NO CHARACTERS IN SELECT.DEF", 235, 110, 100);
        drawSelectTextCentered(renderer, centerX, footerControlsY, "ESC mode select", 156, 166, 180);
        return;
    }

    const bool opponentHasPortrait = hasTexture(view.opponentPortrait)
        && view.opponentPortrait.width > 0
        && view.opponentPortrait.height > 0;

    drawSelectFrame(renderer, leftCardX, cardY, cardW, cardH, true);
    drawSelectFrame(renderer, rightCardX, cardY, cardW, cardH, view.showP2Cursor);
    drawSelectText(renderer, leftCardX + 8.0f, cardY + 8.0f, fitDebugText(view.showP2Cursor ? "PLAYER 1" : view.activePlayerLabel, 14), 231, 195, 90);
    drawSelectText(renderer, rightCardX + 8.0f, cardY + 8.0f, fitDebugText(view.showP2Cursor ? "PLAYER 2" : (view.opponentName == "ENEMY WAVES" ? "STORY ROUTE" : "OPPONENT"), 14), 231, 195, 90);

    drawPortraitBox(
        renderer,
        view.selectedPortrait,
        leftCardX + portraitPad,
        portraitBoxY,
        cardW - portraitPad * 2.0f,
        portraitBoxH);
    if (opponentHasPortrait) {
        drawPortraitBox(
            renderer,
            view.opponentPortrait,
            rightCardX + portraitPad,
            portraitBoxY,
            cardW - portraitPad * 2.0f,
            portraitBoxH);
    } else if (view.opponentName == "ENEMY WAVES") {
        drawWaveOpponentSlot(
            renderer,
            rightCardX + portraitPad,
            portraitBoxY,
            cardW - portraitPad * 2.0f,
            portraitBoxH);
    } else {
        drawFixedOpponentSlot(
            renderer,
            rightCardX + portraitPad,
            portraitBoxY,
            cardW - portraitPad * 2.0f,
            portraitBoxH,
            view.opponentIsDummy);
    }

    constexpr int cardNameChars = 18;
    drawSelectText(renderer, leftCardX + 8.0f, cardY + cardH - 26.0f, fitDebugText(view.selectedName, cardNameChars), 235, 240, 248);
    if (!view.selectedProgressionLabel.empty()) {
        drawSelectText(renderer, leftCardX + 8.0f, cardY + cardH - 13.0f, fitDebugText(view.selectedProgressionLabel, cardNameChars), 120, 230, 170);
    }
    drawSelectText(renderer, rightCardX + 8.0f, cardY + cardH - 26.0f, fitDebugText(view.opponentName, cardNameChars), 235, 240, 248);
    if (!view.opponentProgressionLabel.empty()) {
        drawSelectText(renderer, rightCardX + 8.0f, cardY + cardH - 13.0f, fitDebugText(view.opponentProgressionLabel, cardNameChars), 120, 230, 170);
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

    setColor(renderer, 10, 17, 26, 212);
    fillRect(renderer, panelX + 10.0f, footerStatusY - 8.0f, panelW - 20.0f, 48.0f);
    setColor(renderer, 81, 210, 198, 150);
    drawRect(renderer, panelX + 10.0f, footerStatusY - 8.0f, panelW - 20.0f, 48.0f);
    setColor(renderer, 198, 79, 85, 140);
    fillRect(renderer, panelX + 18.0f, footerStatusY + 14.0f, panelW - 36.0f, 1.0f);

    if (view.showP2Cursor) {
        drawSelectTextCentered(
            renderer,
            centerX,
            footerStatusY,
            std::string("P1 ") + (view.p1Confirmed ? "OK" : "choose") + "   P2 " + (view.p2Confirmed ? "OK" : "choose"),
            238,
            210,
            94);
    } else {
        drawSelectTextCentered(renderer, centerX, footerStatusY, view.activePlayerLabel, 238, 210, 94);
    }
    drawSelectTextCentered(renderer, centerX, footerStageY, fitDebugText("STAGE: " + view.preferredStageLabel, 54), 210, 218, 230);

    drawSelectTextCentered(
        renderer,
        centerX,
        footerControlsY,
        view.showP2Cursor ? "P1 arrows/ENTER  P2 IJKL/;  ESC back" : "ARROWS choose  ENTER stage  ESC back",
        156,
        166,
        180);
}

} // namespace dragon
