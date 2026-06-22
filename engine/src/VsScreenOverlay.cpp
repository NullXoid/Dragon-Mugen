#include "VsScreenOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>

namespace dragon {
namespace {

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

} // namespace

void drawVersusScreenOverlay(const UiRenderContext& ui, const VsScreenView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float centerX = widthF * 0.5f;

    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    setColor(renderer, 28, 26, 36);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(ui.logicalHeight));
    setColor(renderer, 42, 35, 52);
    fillRect(renderer, 0, 100, widthF, 42);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 18, 16, "DRAGON MUGEN CORE");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 20, 30, view.modeTitle + " VS");

    drawPanel(renderer, 22, 62, 108, 96);
    drawPanel(renderer, widthF - 130.0f, 62, 108, 96);

    if (hasTexture(view.p1Portrait) && view.p1Portrait.width > 0 && view.p1Portrait.height > 0) {
        const float scale = std::min({
            1.0f,
            92.0f / static_cast<float>(view.p1Portrait.width),
            96.0f / static_cast<float>(view.p1Portrait.height),
        });
        drawSpriteTopLeft(
            renderer,
            view.p1Portrait,
            76.0f - static_cast<float>(view.p1Portrait.width) * scale * 0.5f,
            76,
            scale);
    } else {
        setColor(renderer, 70, 132, 190);
        fillRect(renderer, 50, 86, 52, 48);
    }
    if (hasTexture(view.opponentPortrait) && view.opponentPortrait.width > 0 && view.opponentPortrait.height > 0) {
        const float scale = std::min({
            1.0f,
            76.0f / static_cast<float>(view.opponentPortrait.width),
            76.0f / static_cast<float>(view.opponentPortrait.height),
        });
        drawSpriteTopLeft(
            renderer,
            view.opponentPortrait,
            widthF - 76.0f - static_cast<float>(view.opponentPortrait.width) * scale * 0.5f,
            76,
            scale);
    } else {
        drawFixedOpponentSlot(renderer, widthF - 122.0f, 72, 76, 76, view.opponentSlotLabel);
    }

    setColor(renderer, 222, 226, 232);
    debugText(renderer, 34, 144, view.p1Name);
    debugText(renderer, widthF - 100.0f, 144, view.opponentName);

    setColor(renderer, 230, 130, 120);
    debugTextCentered(renderer, centerX, 98, "VS");

    const float progress = std::clamp(view.loadProgress, 0.0f, 1.0f);
    const std::string phaseText = view.loadPhaseText.empty() ? "Preparing" : view.loadPhaseText;
    const std::string progressText = view.loadProgressText.empty()
        ? std::to_string(static_cast<int>(progress * 100.0f + 0.5f)) + "%"
        : view.loadProgressText;

    setColor(renderer, 155, 164, 174);
    debugText(renderer, 20, 184, "Stage: " + view.stageName);
    setColor(renderer, 190, 204, 218);
    debugText(renderer, 20, 202, phaseText);
    setColor(renderer, 224, 232, 238);
    debugText(renderer, widthF - 54.0f, 202, progressText);
    drawLoadingProgressBar(renderer, 20.0f, 216.0f, widthF - 40.0f, 10.0f, progress, view.loadStatus);

    switch (view.loadStatus) {
    case VsScreenLoadStatus::Failed:
        setColor(renderer, 230, 130, 120);
        debugText(renderer, 20, 230, "Load failed. ESC stage select");
        break;
    case VsScreenLoadStatus::Ready:
        debugText(renderer, 20, 230, "Ready. ENTER start now");
        break;
    case VsScreenLoadStatus::Loading:
    default:
        setColor(renderer, 155, 164, 174);
        debugText(renderer, 20, 230, "Please wait");
        break;
    }
}

} // namespace dragon
