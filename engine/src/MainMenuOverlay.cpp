#include "MainMenuOverlay.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"
#include "UiSpriteView.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

namespace dragon {
namespace {

constexpr std::array<std::string_view, kMainMenuOptionCount> kDefaultModeLabels{ {
    "TRAINING",
    "SINGLE PLAYER",
    "VS MODE",
    "ARENA MODE",
    "STORY MODE",
    "SHOP DEMO",
    "OPTIONS",
    "EXIT",
} };

std::string_view modeLabel(const MainMenuView& view, std::size_t index) {
    if (index < view.labels.size() && !view.labels[index].empty()) {
        return view.labels[index];
    }
    return kDefaultModeLabels[index];
}

struct MainMenuLayout {
    SDL_FRect panel{};
    float rowH = 18.0f;
    float headerH = 22.0f;
    float footerH = 3.0f;
    float scale = 1.0f;
    float border = 1.0f;
};

MainMenuLayout mainMenuLayout(const UiRenderContext& ui) {
    constexpr float designPanelW = 176.0f;
    constexpr float designPanelH = 110.0f;
    constexpr float designHeaderH = 22.0f;
    constexpr float designFooterH = 3.0f;
    constexpr float designRowH = 10.625f;

    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const float s = metrics.pixelScale;
    const float border = metrics.border;
    const float height = static_cast<float>(ui.logicalHeight);
    const float panelW = designPanelW * s;
    const float panelH = designPanelH * s;
    const float panelX = std::floor((static_cast<float>(ui.logicalWidth) - panelW) * 0.5f);
    const float desiredY = std::floor(height * 0.56f);
    const float minY = metrics.topBarH + 14.0f * s;
    const float maxY = std::max(minY, height - panelH - 6.0f * s);
    const float panelY = std::clamp(desiredY, minY, maxY);

    return MainMenuLayout{
        SDL_FRect{ panelX, panelY, panelW, panelH },
        designRowH * s,
        designHeaderH * s,
        designFooterH * s,
        s,
        border,
    };
}

} // namespace

void drawMainMenuCoverImage(
    SDL_Renderer* renderer,
    const UiSpriteView& sprite,
    float width,
    float height,
    float panX) {
    if (!hasTexture(sprite) || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }

    const float scale = std::max(width / static_cast<float>(sprite.width), height / static_cast<float>(sprite.height));
    const float drawW = static_cast<float>(sprite.width) * scale;
    const float drawH = static_cast<float>(sprite.height) * scale;
    const float overflowX = std::max(0.0f, drawW - width);
    const float x = -overflowX * std::clamp(panX, 0.0f, 1.0f);
    const float y = (height - drawH) * 0.5f;
    SDL_FRect dst{ x, y, drawW, drawH };
    SDL_RenderTexture(renderer, sprite.texture, nullptr, &dst);
}

void drawMainMenuTitleText(const UiRenderContext& ui) {
    SDL_Renderer* renderer = ui.renderer;
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const auto& tokens = dragonUiTokens();
    const float s = metrics.pixelScale;
    const float width = static_cast<float>(ui.logicalWidth);

    setColor(renderer, tokens.panelBase, 220);
    fillRect(renderer, 0.0f, 0.0f, width, metrics.topBarH);
    setColor(renderer, tokens.separatorRed);
    fillRect(renderer, 0.0f, metrics.topBarH - metrics.border, width, metrics.border);
    setColor(renderer, tokens.mutedGold);
    scaledDebugText(renderer, s, 10.0f * s, 8.0f * s, "DRAGON MUGEN CORE");
    setColor(renderer, tokens.primaryTeal);
    scaledDebugText(renderer, s, width * 0.5f - 32.0f * s, 8.0f * s, "MAIN MENU");
}

void drawDragonMainMenuBackdrop(const UiRenderContext& ui, const MainMenuBackdropView& backdrop) {
    SDL_Renderer* renderer = ui.renderer;
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const auto& tokens = dragonUiTokens();
    const float width = static_cast<float>(ui.logicalWidth);
    const float height = static_cast<float>(ui.logicalHeight);
    const float s = metrics.pixelScale;
    const float top = metrics.topBarH;
    const float horizonY = std::floor(top + (height - top) * 0.43f);

    if (hasTexture(backdrop.background)) {
        drawMainMenuCoverImage(renderer, backdrop.background, width, height, backdrop.panX);
        if (backdrop.dimAlpha > 0) {
            setColor(renderer, tokens.panelBase, static_cast<Uint8>(std::clamp(backdrop.dimAlpha, 0, 255)));
            fillRect(renderer, 0.0f, 0.0f, width, height);
        }
        return;
    }

    if (!backdrop.fallbackGrid) {
        setColor(renderer, 3, 7, 12);
        fillRect(renderer, 0.0f, 0.0f, width, height);
        return;
    }

    setColor(renderer, 3, 7, 12);
    fillRect(renderer, 0.0f, 0.0f, width, height);

    for (int i = 0; i < 7; ++i) {
        const float y = top + static_cast<float>(i) * 9.0f * s;
        const Uint8 alpha = static_cast<Uint8>(70 - i * 6);
        setColor(renderer, tokens.secondaryPanel, alpha);
        fillRect(renderer, 0.0f, y, width, 9.0f * s);
    }

    setColor(renderer, tokens.primaryTeal, 58);
    fillRect(renderer, 0.0f, horizonY - 9.0f * s, width, 1.0f * s);
    setColor(renderer, tokens.mutedGold, 74);
    fillRect(renderer, 0.0f, horizonY, width, 1.0f * s);
    setColor(renderer, tokens.separatorRed, 54);
    fillRect(renderer, 0.0f, horizonY + 9.0f * s, width, 1.0f * s);

    setColor(renderer, tokens.secondaryPanel, 206);
    fillRect(renderer, 0.0f, horizonY + 1.0f * s, width, height - horizonY);

    setColor(renderer, tokens.primaryTeal, 32);
    for (int i = 0; i < 6; ++i) {
        const float y = horizonY + 14.0f * s + static_cast<float>(i * i + i) * 2.0f * s;
        if (y < height) {
            fillRect(renderer, 0.0f, y, width, 1.0f * s);
        }
    }

    for (int i = -4; i <= 4; ++i) {
        const float startX = width * 0.5f + static_cast<float>(i) * 10.0f * s;
        const float endX = width * 0.5f + static_cast<float>(i) * 45.0f * s;
        SDL_RenderLine(renderer, startX, horizonY + 1.0f * s, endX, height);
    }

    setColor(renderer, tokens.mutedGold, 54);
    fillRect(renderer, 14.0f * s, horizonY + 26.0f * s, width - 28.0f * s, 1.0f * s);
    setColor(renderer, tokens.characterPurple, 44);
    fillRect(renderer, width * 0.5f - 42.0f * s, top + 27.0f * s, 84.0f * s, 1.0f * s);
}

SDL_FRect mainMenuPanelRect(const UiRenderContext& ui) {
    return mainMenuLayout(ui).panel;
}

void drawMainMenuOverlay(const UiRenderContext& ui, const MainMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const MainMenuLayout layout = mainMenuLayout(ui);
    const auto& tokens = dragonUiTokens();
    const float s = layout.scale;
    const float border = layout.border;
    const float rowH = layout.rowH;
    const float panelW = layout.panel.w;
    const float panelH = layout.panel.h;
    const float menuX = layout.panel.x;
    const float menuY = layout.panel.y;
    const float centerX = menuX + panelW * 0.5f;
    const int selectedMode = std::clamp(view.selectedMode, 0, static_cast<int>(kDefaultModeLabels.size()) - 1);
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.16f);
    const int pulseAlpha = 96 + static_cast<int>(pulse * 70.0f);

    setColor(renderer, tokens.panelBase, 228);
    fillRect(renderer, menuX, menuY, panelW, panelH);
    setColor(renderer, tokens.primaryTeal, 160);
    drawRect(renderer, menuX, menuY, panelW, panelH);
    setColor(renderer, tokens.secondaryPanel, 238);
    fillRect(renderer, menuX + 2.0f * s, menuY + 2.0f * s, panelW - 4.0f * s, 18.0f * s);
    setColor(renderer, tokens.separatorRed);
    fillRect(renderer, menuX + 2.0f * s, menuY + 20.0f * s, panelW - 4.0f * s, border);
    setColor(renderer, tokens.characterPurple);
    scaledDebugText(renderer, s, menuX + 10.0f * s, menuY + 8.0f * s, "M.U.G.E.N");
    setColor(renderer, tokens.mutedText);
    scaledDebugText(renderer, s, menuX + panelW - 58.0f * s, menuY + 8.0f * s, "CORE");

    for (int i = 0; i < static_cast<int>(kDefaultModeLabels.size()); ++i) {
        const float y = menuY + layout.headerH + 2.0f * s + static_cast<float>(i) * rowH;
        const std::string label(modeLabel(view, static_cast<std::size_t>(i)));
        const float textX = centerX - static_cast<float>(label.size()) * 4.0f * s;
        const float selectionH = std::max(6.0f * s, rowH - 1.0f * s);
        const float fillH = std::max(5.0f * s, rowH - 3.0f * s);
        const float textY = y + std::max(0.0f, (rowH - 7.0f * s) * 0.5f);

        if (i == selectedMode) {
            setColor(renderer, tokens.mutedGold, static_cast<Uint8>(pulseAlpha));
            drawRect(renderer, menuX + 12.0f * s, y - 2.0f * s, panelW - 24.0f * s, selectionH);
            setColor(renderer, tokens.secondaryPanel, 238);
            fillRect(renderer, menuX + 15.0f * s, y, panelW - 30.0f * s, fillH);
            setColor(renderer, tokens.primaryTeal);
            fillRect(renderer, menuX + 18.0f * s, y + selectionH - 2.0f * s, panelW - 36.0f * s, border);
            setColor(renderer, tokens.mutedGold);
            scaledDebugText(renderer, s, textX, textY, label);
        } else {
            setColor(renderer, tokens.primaryText);
            scaledDebugText(renderer, s, textX, textY, label);
        }
    }

    if (view.exitConfirmOpen) {
        const float confirmW = std::min(184.0f * s, static_cast<float>(ui.logicalWidth) - 20.0f * s);
        const float confirmH = 24.0f * s;
        const float confirmX = centerX - confirmW * 0.5f;
        const float confirmY = std::max(
            28.0f * s,
            static_cast<float>(ui.logicalHeight) - 22.0f * s - confirmH - 5.0f * s);
        setColor(renderer, tokens.panelBase, 238);
        fillRect(renderer, confirmX, confirmY, confirmW, confirmH);
        setColor(renderer, tokens.mutedGold);
        drawRect(renderer, confirmX, confirmY, confirmW, confirmH);
        setColor(renderer, tokens.primaryText);
        scaledDebugText(renderer, s, centerX - 52.0f * s, confirmY + 5.0f * s, "ARE YOU SURE?");
        setColor(renderer, tokens.mutedText);
        scaledDebugText(renderer, s, centerX - 68.0f * s, confirmY + 15.0f * s, "ENTER YES  ESC NO");
    }
}

} // namespace dragon
