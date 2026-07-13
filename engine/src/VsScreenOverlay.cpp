#include "VsScreenOverlay.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>

namespace dragon {
namespace {

constexpr float kLoadingVirtualWidth = 640.0f;
constexpr float kLoadingVirtualHeight = 360.0f;

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
    const auto& tokens = dragonUiTokens();
    const float topH = std::max(24.0f, height * 0.12f);
    const float horizonY = height * 0.56f;

    setColor(renderer, tokens.panelBase);
    fillRect(renderer, 0.0f, 0.0f, width, height);
    setColor(renderer, tokens.panelBase);
    fillRect(renderer, 0.0f, 0.0f, width, topH);
    setColor(renderer, tokens.secondaryPanel, 230);
    fillRect(renderer, 0.0f, topH, width, horizonY - topH);
    setColor(renderer, tokens.panelBase, 242);
    fillRect(renderer, 0.0f, horizonY, width, height - horizonY);

    for (int i = 0; i < 7; ++i) {
        const float t = static_cast<float>(i) / 6.0f;
        const float y = topH + (horizonY - topH) * t;
        setColor(renderer, tokens.primaryTeal, static_cast<Uint8>(18 + i * 4));
        fillRect(renderer, width * (0.10f + t * 0.08f), y, width * (0.80f - t * 0.16f), 1.0f);
    }

    for (int i = 0; i < 6; ++i) {
        const float t = static_cast<float>(i) / 5.0f;
        const float y = horizonY + (height - horizonY) * t;
        setColor(renderer, tokens.primaryTeal, static_cast<Uint8>(16 + i * 7));
        fillRect(renderer, 0.0f, y, width, 1.0f);
    }

    const float centerX = width * 0.5f;
    for (int i = -4; i <= 4; ++i) {
        const float offset = static_cast<float>(i) * width * 0.09f;
        setColor(renderer, tokens.primaryTeal, 18);
        fillRect(renderer, centerX + offset, horizonY, 1.0f, height - horizonY);
    }

    setColor(renderer, tokens.mutedGold, 62);
    fillRect(renderer, width * 0.08f, horizonY + (height - horizonY) * 0.47f, width * 0.84f, 1.0f);
    setColor(renderer, tokens.separatorRed, 230);
    fillRect(renderer, 0.0f, topH - 2.0f, width, 2.0f);
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
    const auto& tokens = dragonUiTokens();
    setColor(renderer, 8, 10, 15, 230);
    fillRect(renderer, x, y, width, height);
    setColor(renderer, tokens.mutedText, 230);
    drawRect(renderer, x, y, width, height);

    SDL_Color fill = tokens.primaryTeal;
    if (status == VsScreenLoadStatus::Ready) {
        fill = { 118, 226, 160, 255 };
    } else if (status == VsScreenLoadStatus::Failed) {
        fill = { 230, 96, 92, 255 };
    }

    if (clamped > 0.0f) {
        setColor(renderer, fill, 220);
        fillRect(renderer, x + 2.0f, y + 2.0f, std::max(0.0f, (width - 4.0f) * clamped), height - 4.0f);
        setColor(renderer, tokens.mutedGold, 90);
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
    const auto& tokens = dragonUiTokens();
    setColor(renderer, tokens.panelBase, 238);
    fillRect(renderer, x, y, width, height);
    setColor(renderer, tokens.mutedText);
    drawRect(renderer, x, y, width, height);
    setColor(renderer, tokens.secondaryPanel, 210);
    fillRect(renderer, x + 1.0f, y + 1.0f, width - 2.0f, 14.0f);
    setColor(renderer, tokens.primaryTeal, 180);
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
            src.y = 0.0f;
            src.h = static_cast<float>(portrait.height) * 0.72f;
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

    setColor(renderer, tokens.panelBase, 230);
    fillRect(renderer, x + 1.0f, y + height - 18.0f, width - 2.0f, 17.0f);
    setColor(renderer, tokens.primaryText);
    const int maxChars = std::max(5, static_cast<int>((width - 14.0f) / 8.0f));
    debugTextCentered(renderer, x + width * 0.5f, y + height - 14.0f, fitDebugText(name, static_cast<std::size_t>(maxChars)));
    setColor(renderer, tokens.mutedText);
    debugTextCentered(renderer, x + width * 0.5f, y + 5.0f, fitDebugText(fallbackLabel, static_cast<std::size_t>(maxChars)));
}

void drawVersusScreenOverlayStable(SDL_Renderer* renderer, const VsScreenView& view) {
    const auto& tokens = dragonUiTokens();
    const float widthF = kLoadingVirtualWidth;
    const float heightF = kLoadingVirtualHeight;
    const float centerX = widthF * 0.5f;
    const float progress = std::clamp(view.loadProgress, 0.0f, 1.0f);
    const std::string progressText = view.loadProgressText.empty()
        ? std::to_string(static_cast<int>(progress * 100.0f + 0.5f)) + "%"
        : view.loadProgressText;
    const std::string phaseText = view.loadPhaseText.empty()
        ? (view.loadStatus == VsScreenLoadStatus::Ready
                ? "READY"
                : view.loadStatus == VsScreenLoadStatus::Failed ? "FAILED" : "PREPARING MATCH")
        : view.loadPhaseText;

    drawLoadingBackdrop(renderer, widthF, heightF);

    loadingText(renderer, 18.0f, 18.0f, "DRAGON MUGEN CORE", 231, 195, 90);
    loadingTextCentered(renderer, centerX, 18.0f, fitDebugText(view.modeTitle.empty() ? "LOADING" : view.modeTitle, 24), 81, 210, 198);

    const float panelW = std::min(576.0f, widthF - 48.0f);
    const float panelX = centerX - panelW * 0.5f;
    const float panelY = 55.0f;
    const float panelH = 252.0f;
    setColor(renderer, tokens.panelBase, 226);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.primaryTeal, 235);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.secondaryPanel, 230);
    fillRect(renderer, panelX + 1.0f, panelY + 1.0f, panelW - 2.0f, 26.0f);
    setColor(renderer, tokens.separatorRed, 240);
    fillRect(renderer, panelX + 1.0f, panelY + 27.0f, panelW - 2.0f, 2.0f);
    loadingText(renderer, panelX + 12.0f, panelY + 10.0f, "MATCH LOADING", 231, 195, 90);
    loadingTextRight(renderer, panelX + panelW - 12.0f, panelY + 10.0f, view.loadStatus == VsScreenLoadStatus::Ready
            ? "READY"
            : view.loadStatus == VsScreenLoadStatus::Failed ? "FAILED" : "PLEASE WAIT",
        137,
        150,
        167);

    const float cardW = 178.0f;
    const float cardH = 146.0f;
    const float cardY = panelY + 43.0f;
    drawLoadingPortraitCard(renderer, view.p1Portrait, panelX + 22.0f, cardY, cardW, cardH, view.p1Name, "P1");
    drawLoadingPortraitCard(renderer, view.opponentPortrait, panelX + panelW - 22.0f - cardW, cardY, cardW, cardH, view.opponentName, view.opponentSlotLabel);

    const float matchupW = 156.0f;
    const float matchupX = centerX - matchupW * 0.5f;
    setColor(renderer, tokens.panelBase, 225);
    fillRect(renderer, matchupX, cardY + 28.0f, matchupW, 70.0f);
    setColor(renderer, tokens.mutedGold, 230);
    fillRect(renderer, matchupX + 12.0f, cardY + 30.0f, matchupW - 24.0f, 1.0f);
    loadingTextCentered(renderer, centerX, cardY + 44.0f, "VS", 233, 237, 243);
    loadingTextCentered(renderer, centerX, cardY + 63.0f, fitDebugText(view.opponentSlotLabel, 16), 137, 150, 167);
    loadingTextCentered(renderer, centerX, cardY + 82.0f, fitDebugText(phaseText, 16), 81, 210, 198);

    const float loadX = panelX + 24.0f;
    const float loadY = panelY + panelH - 52.0f;
    const float loadW = panelW - 48.0f;
    setColor(renderer, tokens.secondaryPanel, 232);
    fillRect(renderer, loadX, loadY, loadW, 42.0f);
    setColor(renderer, tokens.primaryTeal, 210);
    drawRect(renderer, loadX, loadY, loadW, 42.0f);
    loadingText(renderer, loadX + 8.0f, loadY + 9.0f, "STAGE", 137, 150, 167);
    loadingText(renderer, loadX + 66.0f, loadY + 9.0f, fitDebugText(view.stageName, 43), 233, 237, 243);
    loadingTextRight(renderer, loadX + loadW - 8.0f, loadY + 9.0f, progressText, 231, 195, 90);
    drawLoadingProgressBar(renderer, loadX + 8.0f, loadY + 29.0f, loadW - 16.0f, 7.0f, progress, view.loadStatus);

    setColor(renderer, tokens.panelBase, 224);
    fillRect(renderer, panelX, panelY + panelH + 10.0f, panelW, 24.0f);
    setColor(renderer, tokens.primaryTeal, 210);
    drawRect(renderer, panelX, panelY + panelH + 10.0f, panelW, 24.0f);
    loadingText(renderer, panelX + 12.0f, panelY + panelH + 18.0f, "MATCH DATA", 137, 150, 167);
    loadingTextRight(renderer, panelX + panelW - 12.0f, panelY + panelH + 18.0f, "PLEASE WAIT", 137, 150, 167);
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

    ScopedVirtualCanvas virtualCanvas(ui, kLoadingVirtualWidth, kLoadingVirtualHeight);
    drawVersusScreenOverlayStable(renderer, view);
}

} // namespace dragon
