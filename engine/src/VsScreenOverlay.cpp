#include "VsScreenOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>

namespace dragon {
namespace {

struct LoadingCanvas {
    float width = 426.0f;
    float height = 240.0f;
    bool hd = false;
};

LoadingCanvas loadingCanvas(const UiRenderContext& ui) {
    if (ui.logicalWidth >= 854 && ui.logicalHeight >= 480) {
        return { 640.0f, 360.0f, true };
    }
    return { ui.logicalWidth <= 340 ? 320.0f : 426.0f, 240.0f, false };
}

float textWidth(const std::string& text) {
    return static_cast<float>(text.size() * 8);
}

void loadingText(SDL_Renderer* renderer, float x, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    setColor(renderer, 3, 5, 8, 220);
    debugText(renderer, x + 1.0f, y + 1.0f, text);
    setColor(renderer, r, g, b);
    debugText(renderer, x, y, text);
}

void loadingTextCentered(SDL_Renderer* renderer, float centerX, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    loadingText(renderer, centerX - textWidth(text) * 0.5f, y, text, r, g, b);
}

void loadingTextRight(SDL_Renderer* renderer, float rightX, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    loadingText(renderer, rightX - textWidth(text), y, text, r, g, b);
}

void drawLoadingBackdrop(SDL_Renderer* renderer, float width, float height) {
    const float topH = height * 0.13f;
    const float heroH = height * 0.29f;
    const float floorY = topH + heroH;

    setColor(renderer, 5, 9, 14);
    fillRect(renderer, 0.0f, 0.0f, width, height);

    setColor(renderer, 7, 16, 25);
    fillRect(renderer, 0.0f, 0.0f, width, topH);
    setColor(renderer, 12, 22, 34, 235);
    fillRect(renderer, 0.0f, topH, width, heroH);
    setColor(renderer, 5, 8, 13, 230);
    fillRect(renderer, 0.0f, floorY, width, height - floorY);

    setColor(renderer, 81, 210, 198, 42);
    fillRect(renderer, width * 0.04f, topH + heroH * 0.58f, width * 0.92f, 1.0f);
    setColor(renderer, 231, 195, 90, 36);
    fillRect(renderer, width * 0.08f, floorY + height * 0.18f, width * 0.84f, 1.0f);
    setColor(renderer, 198, 79, 85, 220);
    fillRect(renderer, 0.0f, topH - 2.0f, width, 2.0f);

    setColor(renderer, 12, 18, 26, 165);
    fillRect(renderer, width * 0.08f, floorY + height * 0.08f, width * 0.22f, height * 0.19f);
    fillRect(renderer, width * 0.70f, floorY + height * 0.05f, width * 0.20f, height * 0.22f);
}

void drawFixedOpponentSlot(
    SDL_Renderer* renderer,
    float x,
    float y,
    float width,
    float height,
    const std::string& label) {
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
    debugTextCentered(renderer, x + width * 0.5f, y + height * 0.86f, label);
}

void drawLoadingProgressBar(
    SDL_Renderer* renderer,
    float x,
    float y,
    float width,
    float height,
    float progress,
    VsScreenLoadStatus status) {
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    setColor(renderer, 8, 10, 15, 230);
    fillRect(renderer, x, y, width, height);
    setColor(renderer, 72, 80, 98, 230);
    drawRect(renderer, x, y, width, height);

    Uint8 r = 96;
    Uint8 g = 168;
    Uint8 b = 230;
    if (status == VsScreenLoadStatus::Ready) {
        r = 118;
        g = 226;
        b = 160;
    } else if (status == VsScreenLoadStatus::Failed) {
        r = 230;
        g = 96;
        b = 92;
    }

    if (clamped > 0.0f) {
        setColor(renderer, r, g, b, 220);
        fillRect(renderer, x + 2.0f, y + 2.0f, std::max(0.0f, (width - 4.0f) * clamped), height - 4.0f);
        setColor(renderer, 245, 235, 150, 90);
        fillRect(renderer, x + 2.0f, y + 2.0f, std::max(0.0f, (width - 4.0f) * clamped), 2.0f);
    }
}

void drawLoadingPortraitCard(
    SDL_Renderer* renderer,
    const UiSpriteView& portrait,
    float x,
    float y,
    float width,
    float height,
    const std::string& name,
    const std::string& fallbackLabel) {
    setColor(renderer, 7, 16, 25, 238);
    fillRect(renderer, x, y, width, height);
    setColor(renderer, 64, 78, 98);
    drawRect(renderer, x, y, width, height);
    setColor(renderer, 16, 26, 39, 210);
    fillRect(renderer, x + 1.0f, y + 1.0f, width - 2.0f, 14.0f);
    setColor(renderer, 81, 210, 198, 180);
    fillRect(renderer, x + 1.0f, y + 1.0f, width - 2.0f, 1.0f);

    const float imageY = y + 17.0f;
    const float imageH = height - 35.0f;
    if (hasTexture(portrait) && portrait.width > 0 && portrait.height > 0) {
        SDL_FRect src{
            0.0f,
            0.0f,
            static_cast<float>(portrait.width),
            static_cast<float>(portrait.height),
        };
        if (portrait.height > portrait.width * 1.25f) {
            src.x = static_cast<float>(portrait.width) * 0.33f;
            src.w = static_cast<float>(portrait.width) * 0.34f;
            src.y = static_cast<float>(portrait.height) * 0.04f;
            src.h = static_cast<float>(portrait.height) * 0.22f;
        }
        const float scale = std::min({
            2.0f,
            (width - 16.0f) / src.w,
            imageH / src.h,
        });
        SDL_FRect dst{
            x + (width - src.w * scale) * 0.5f,
            imageY + (imageH - src.h * scale) * 0.5f,
            src.w * scale,
            src.h * scale,
        };
        SDL_RenderTextureRotated(renderer, portrait.texture, &src, &dst, 0.0, nullptr, SDL_FLIP_NONE);
    } else {
        drawFixedOpponentSlot(renderer, x + width * 0.21f, imageY + 4.0f, width * 0.58f, imageH - 8.0f, fallbackLabel);
    }

    setColor(renderer, 12, 14, 18, 230);
    fillRect(renderer, x + 1.0f, y + height - 18.0f, width - 2.0f, 17.0f);
    setColor(renderer, 233, 237, 243);
    const int maxChars = std::max(5, static_cast<int>((width - 14.0f) / 8.0f));
    debugTextCentered(renderer, x + width * 0.5f, y + height - 14.0f, fitDebugText(name, static_cast<std::size_t>(maxChars)));
    setColor(renderer, 137, 150, 167);
    debugTextCentered(renderer, x + width * 0.5f, y + 5.0f, fitDebugText(fallbackLabel, static_cast<std::size_t>(maxChars)));
}

void drawVersusScreenOverlayHd(SDL_Renderer* renderer, const VsScreenView& view, float widthF, float heightF) {
    const float centerX = widthF * 0.5f;
    const float progress = std::clamp(view.loadProgress, 0.0f, 1.0f);
    const std::string phaseText = view.loadPhaseText.empty() ? "Preparing" : view.loadPhaseText;
    const std::string progressText = view.loadProgressText.empty()
        ? std::to_string(static_cast<int>(progress * 100.0f + 0.5f)) + "%"
        : view.loadProgressText;

    drawLoadingBackdrop(renderer, widthF, heightF);

    loadingText(renderer, 18.0f, 18.0f, "DRAGON MUGEN CORE", 231, 195, 90);
    loadingTextCentered(renderer, centerX, 18.0f, fitDebugText(view.modeTitle.empty() ? "LOADING" : view.modeTitle, 24), 81, 210, 198);
    loadingTextRight(renderer, widthF - 18.0f, 18.0f, progressText, 137, 150, 167);

    const float panelX = 46.0f;
    const float panelY = 70.0f;
    const float panelW = widthF - panelX * 2.0f;
    const float panelH = 218.0f;
    setColor(renderer, 7, 16, 25, 226);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 26, 144, 138, 235);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 16, 26, 39, 230);
    fillRect(renderer, panelX + 1.0f, panelY + 1.0f, panelW - 2.0f, 26.0f);
    setColor(renderer, 198, 79, 85, 240);
    fillRect(renderer, panelX + 1.0f, panelY + 27.0f, panelW - 2.0f, 2.0f);
    loadingText(renderer, panelX + 12.0f, panelY + 10.0f, "LOADING", 231, 195, 90);
    loadingTextRight(renderer, panelX + panelW - 12.0f, panelY + 10.0f, view.loadStatus == VsScreenLoadStatus::Ready
            ? "READY"
            : view.loadStatus == VsScreenLoadStatus::Failed ? "FAILED" : "PLEASE WAIT",
        137,
        150,
        167);

    const float cardW = 150.0f;
    const float cardH = 112.0f;
    const float cardY = panelY + 44.0f;
    drawLoadingPortraitCard(renderer, view.p1Portrait, panelX + 24.0f, cardY, cardW, cardH, view.p1Name, "P1");
    drawLoadingPortraitCard(renderer, view.opponentPortrait, panelX + panelW - 24.0f - cardW, cardY, cardW, cardH, view.opponentName, view.opponentSlotLabel);

    const float matchupW = 144.0f;
    const float matchupX = centerX - matchupW * 0.5f;
    setColor(renderer, 4, 7, 12, 225);
    fillRect(renderer, matchupX, cardY + 24.0f, matchupW, 62.0f);
    setColor(renderer, 231, 195, 90, 230);
    fillRect(renderer, matchupX + 12.0f, cardY + 26.0f, matchupW - 24.0f, 1.0f);
    loadingTextCentered(renderer, centerX, cardY + 39.0f, "VS", 233, 237, 243);
    loadingTextCentered(renderer, centerX, cardY + 58.0f, fitDebugText(view.opponentSlotLabel, 14), 137, 150, 167);

    const float loadX = panelX + 24.0f;
    const float loadY = panelY + panelH - 50.0f;
    const float loadW = panelW - 48.0f;
    setColor(renderer, 10, 16, 25, 232);
    fillRect(renderer, loadX, loadY, loadW, 40.0f);
    setColor(renderer, 26, 144, 138, 210);
    drawRect(renderer, loadX, loadY, loadW, 40.0f);
    loadingText(renderer, loadX + 8.0f, loadY + 9.0f, "STAGE", 137, 150, 167);
    loadingText(renderer, loadX + 66.0f, loadY + 9.0f, fitDebugText(view.stageName, 43), 233, 237, 243);
    loadingTextRight(renderer, loadX + loadW - 8.0f, loadY + 9.0f, progressText, 231, 195, 90);
    drawLoadingProgressBar(renderer, loadX + 8.0f, loadY + 27.0f, loadW - 16.0f, 7.0f, progress, view.loadStatus);

    const float phaseY = 306.0f;
    setColor(renderer, 7, 16, 25, 230);
    fillRect(renderer, panelX, phaseY, panelW, 30.0f);
    setColor(renderer, 26, 144, 138, 210);
    drawRect(renderer, panelX, phaseY, panelW, 30.0f);
    loadingText(renderer, panelX + 12.0f, phaseY + 9.0f, fitDebugText(phaseText, 44), 137, 150, 167);

    const char* statusText = "PLEASE WAIT";
    Uint8 statusR = 155;
    Uint8 statusG = 164;
    Uint8 statusB = 174;
    if (view.loadStatus == VsScreenLoadStatus::Ready) {
        statusText = "ENTER START";
        statusR = 81;
        statusG = 210;
        statusB = 198;
    } else if (view.loadStatus == VsScreenLoadStatus::Failed) {
        statusText = "LOAD FAILED";
        statusR = 230;
        statusG = 130;
        statusB = 120;
    }
    loadingTextRight(renderer, panelX + panelW - 12.0f, phaseY + 9.0f, statusText, statusR, statusG, statusB);
}

} // namespace

void drawVersusScreenOverlay(const UiRenderContext& ui, const VsScreenView& view) {
    SDL_Renderer* renderer = ui.renderer;

    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    setColor(renderer, 7, 16, 25);
    fillRect(renderer, 0, 0, static_cast<float>(ui.logicalWidth), static_cast<float>(ui.logicalHeight));
    setColor(renderer, 16, 26, 39);
    fillRect(renderer, 0, static_cast<float>(ui.logicalHeight) * 0.42f, static_cast<float>(ui.logicalWidth), static_cast<float>(ui.logicalHeight) * 0.16f);

    const LoadingCanvas canvas = loadingCanvas(ui);
    ScopedVirtualCanvas virtualCanvas(ui, canvas.width, canvas.height);
    const float widthF = canvas.width;
    const float centerX = widthF * 0.5f;
    const bool classic = widthF <= 340.0f;

    const float progress = std::clamp(view.loadProgress, 0.0f, 1.0f);
    const std::string phaseText = view.loadPhaseText.empty() ? "Preparing" : view.loadPhaseText;
    const std::string progressText = view.loadProgressText.empty()
        ? std::to_string(static_cast<int>(progress * 100.0f + 0.5f)) + "%"
        : view.loadProgressText;

    if (canvas.hd) {
        drawVersusScreenOverlayHd(renderer, view, canvas.width, canvas.height);
        return;
    }

    drawLoadingBackdrop(renderer, widthF, 240.0f);

    loadingText(renderer, classic ? 8.0f : 12.0f, 9.0f, fitDebugText("DRAGON MUGEN CORE", classic ? 15 : 18), 231, 195, 90);
    loadingTextCentered(renderer, centerX, 9.0f, fitDebugText(view.modeTitle.empty() ? "LOADING" : view.modeTitle, classic ? 12 : 18), 81, 210, 198);
    loadingTextRight(renderer, widthF - (classic ? 8.0f : 12.0f), 9.0f, progressText, 137, 150, 167);

    const float panelX = classic ? 10.0f : 18.0f;
    const float panelY = 42.0f;
    const float panelW = widthF - panelX * 2.0f;
    const float panelH = 150.0f;
    setColor(renderer, 7, 16, 25, 226);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 26, 144, 138, 235);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 16, 26, 39, 230);
    fillRect(renderer, panelX + 1.0f, panelY + 1.0f, panelW - 2.0f, 20.0f);
    setColor(renderer, 198, 79, 85, 240);
    fillRect(renderer, panelX + 1.0f, panelY + 21.0f, panelW - 2.0f, 2.0f);
    loadingText(renderer, panelX + 8.0f, panelY + 7.0f, "LOADING", 231, 195, 90);
    loadingTextRight(renderer, panelX + panelW - 8.0f, panelY + 7.0f, view.loadStatus == VsScreenLoadStatus::Ready
            ? "READY"
            : view.loadStatus == VsScreenLoadStatus::Failed ? "FAILED" : "PLEASE WAIT",
        137,
        150,
        167);

    const float cardW = classic ? 82.0f : 114.0f;
    const float cardH = classic ? 72.0f : 78.0f;
    const float cardY = panelY + 32.0f;
    drawLoadingPortraitCard(renderer, view.p1Portrait, panelX + 10.0f, cardY, cardW, cardH, view.p1Name, "P1");
    drawLoadingPortraitCard(renderer, view.opponentPortrait, panelX + panelW - 10.0f - cardW, cardY, cardW, cardH, view.opponentName, view.opponentSlotLabel);

    const float matchupX = centerX - (classic ? 39.0f : 48.0f);
    const float matchupW = classic ? 78.0f : 96.0f;
    setColor(renderer, 4, 7, 12, 225);
    fillRect(renderer, matchupX, cardY + 18.0f, matchupW, 46.0f);
    setColor(renderer, 231, 195, 90, 230);
    fillRect(renderer, matchupX + 8.0f, cardY + 20.0f, matchupW - 16.0f, 1.0f);
    loadingTextCentered(renderer, centerX, cardY + 30.0f, "VS", 233, 237, 243);
    loadingTextCentered(renderer, centerX, cardY + 46.0f, fitDebugText(view.opponentSlotLabel, classic ? 8 : 10), 137, 150, 167);

    const float loadX = panelX + 8.0f;
    const float loadY = panelY + panelH - 39.0f;
    const float loadW = panelW - 16.0f;
    setColor(renderer, 10, 16, 25, 232);
    fillRect(renderer, loadX, loadY, loadW, 31.0f);
    setColor(renderer, 26, 144, 138, 210);
    drawRect(renderer, loadX, loadY, loadW, 31.0f);
    loadingText(renderer, loadX + 6.0f, loadY + 6.0f, "STAGE", 137, 150, 167);
    loadingText(
        renderer,
        loadX + (classic ? 52.0f : 58.0f),
        loadY + 6.0f,
        fitDebugText(view.stageName, static_cast<std::size_t>(std::max(10.0f, (loadW - (classic ? 62.0f : 112.0f)) / 8.0f))),
        233,
        237,
        243);
    if (!classic) {
        loadingTextRight(renderer, loadX + loadW - 6.0f, loadY + 6.0f, progressText, 231, 195, 90);
    }
    drawLoadingProgressBar(renderer, loadX + 6.0f, loadY + 21.0f, loadW - 12.0f, 6.0f, progress, view.loadStatus);

    const float phaseY = 200.0f;
    setColor(renderer, 7, 16, 25, 230);
    fillRect(renderer, panelX, phaseY, panelW, 27.0f);
    setColor(renderer, 26, 144, 138, 210);
    drawRect(renderer, panelX, phaseY, panelW, 27.0f);
    loadingText(renderer, panelX + 9.0f, phaseY + 7.0f, fitDebugText(phaseText, static_cast<std::size_t>(std::max(10.0f, (panelW - 112.0f) / 8.0f))), 137, 150, 167);

    const char* statusText = "PLEASE WAIT";
    Uint8 statusR = 155;
    Uint8 statusG = 164;
    Uint8 statusB = 174;
    if (view.loadStatus == VsScreenLoadStatus::Ready) {
        statusText = "ENTER START";
        statusR = 81;
        statusG = 210;
        statusB = 198;
    } else if (view.loadStatus == VsScreenLoadStatus::Failed) {
        statusText = "LOAD FAILED";
        statusR = 230;
        statusG = 130;
        statusB = 120;
    }
    loadingTextRight(renderer, panelX + panelW - 9.0f, phaseY + 7.0f, statusText, statusR, statusG, statusB);
}

} // namespace dragon
