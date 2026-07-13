#include "ArenaSetupOverlay.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace dragon {
namespace {

std::string setupLabel(int row, const ArenaSetupView& view) {
    switch (row) {
    case 0:
        return "CPU COUNT  " + std::to_string(view.cpuCount);
    case 1:
        return "CPU 1  " + view.cpuNames[0];
    case 2:
        return view.cpuCount >= 2 ? "CPU 2  " + view.cpuNames[1] : "CPU 2  -";
    case 3:
        return view.cpuCount >= 3 ? "CPU 3  " + view.cpuNames[2] : "CPU 3  -";
    case 4:
        return "STAGE  " + view.stageName;
    case 5:
        return "TIMER  " + view.timerLabel;
    case 6:
        return std::string("Z AXIS  ") + (view.zAxisEnabled ? "ON" : "OFF");
    case 7:
        return std::string("CAMERA ROTATE  ") + (view.cameraRotationEnabled ? "ON" : "OFF");
    case 8:
        return "START MATCH";
    case 9:
    default:
        return "BACK";
    }
}

} // namespace

void drawArenaSetupOverlay(const UiRenderContext& ui, const ArenaSetupView& view) {
    SDL_Renderer* renderer = ui.renderer;
    constexpr float widthF = 640.0f;
    constexpr float heightF = 360.0f;
    ScopedVirtualCanvas virtualCanvas(ui, widthF, heightF);
    const DragonUiTokens tokens = dragonUiTokens();
    const float centerX = widthF * 0.5f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.14f);
    constexpr float topBarH = 42.0f;
    constexpr float panelW = 456.0f;
    constexpr float panelH = 276.0f;
    const float panelX = centerX - panelW * 0.5f;
    constexpr float panelY = 58.0f;
    constexpr float pad = 16.0f;
    constexpr float rowH = 16.0f;
    constexpr float rowStart = panelY + 96.0f;
    constexpr int rowFit = 48;

    setColor(renderer, 6, 8, 12, 142);
    fillRect(renderer, 0, 0, widthF, heightF);
    setColor(renderer, tokens.panelBase.r, tokens.panelBase.g, tokens.panelBase.b, 232);
    fillRect(renderer, 0, 0, widthF, topBarH);
    setColor(renderer, tokens.separatorRed.r, tokens.separatorRed.g, tokens.separatorRed.b);
    fillRect(renderer, 0, topBarH, widthF, 2.0f);

    setColor(renderer, tokens.mutedGold.r, tokens.mutedGold.g, tokens.mutedGold.b);
    debugText(renderer, 28.0f, 13.0f, view.title);
    setColor(renderer, tokens.mutedText.r, tokens.mutedText.g, tokens.mutedText.b);
    const std::string description = fitDebugText(view.description, 42);
    debugTextCentered(renderer, centerX, 13.0f, description);

    setColor(renderer, tokens.panelBase.r, tokens.panelBase.g, tokens.panelBase.b, 226);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.primaryTeal.r, tokens.primaryTeal.g, tokens.primaryTeal.b);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.mutedGold.r, tokens.mutedGold.g, tokens.mutedGold.b);
    fillRect(renderer, panelX + pad, panelY + pad, panelW - pad * 2.0f, 2.0f);
    setColor(renderer, tokens.separatorRed.r, tokens.separatorRed.g, tokens.separatorRed.b);
    fillRect(renderer, panelX + pad, panelY + 58.0f, panelW - pad * 2.0f, 2.0f);

    setColor(renderer, tokens.primaryText.r, tokens.primaryText.g, tokens.primaryText.b);
    debugText(renderer, panelX + pad * 1.5f, panelY + 22.0f, "FIGHTER");
    debugText(renderer, panelX + panelW * 0.38f, panelY + 22.0f, fitDebugText(view.fighterName, 28));
    setColor(renderer, tokens.mutedText.r, tokens.mutedText.g, tokens.mutedText.b);
    debugText(renderer, panelX + pad * 1.5f, panelY + 40.0f, "MODE");
    debugText(renderer, panelX + panelW * 0.38f, panelY + 40.0f, fitDebugText(view.modeLabel, 28));
    debugText(renderer, panelX + pad * 1.5f, panelY + 78.0f, "DEPTH");
    debugText(renderer,
              panelX + panelW * 0.38f,
              panelY + 78.0f,
              fitDebugText(view.zAxisEnabled ? "SHIFT+UP/DOWN" : "OFF", 30));

    constexpr int rowCount = 10;
    const int selected = std::clamp(view.selectedOption, 0, rowCount - 1);
    for (int i = 0; i < rowCount; ++i) {
        const float y = rowStart + static_cast<float>(i) * rowH;
        if (i == selected) {
            setColor(renderer,
                     tokens.primaryTeal.r,
                     tokens.primaryTeal.g,
                     tokens.primaryTeal.b,
                     static_cast<Uint8>(188 + pulse * 52.0f));
            fillRect(renderer, panelX + pad, y - 4.0f, panelW - pad * 2.0f, rowH);
            setColor(renderer, 6, 10, 14);
        } else {
            const bool inactiveCpu = (i == 2 && view.cpuCount < 2) || (i == 3 && view.cpuCount < 3);
            if (inactiveCpu) {
                setColor(renderer, tokens.mutedText.r, tokens.mutedText.g, tokens.mutedText.b, 150);
            } else {
                setColor(renderer, tokens.primaryText.r, tokens.primaryText.g, tokens.primaryText.b);
            }
        }
        debugTextCentered(renderer, centerX, y, fitDebugText(setupLabel(i, view), rowFit));
    }

    const float footerY = panelY + panelH - 26.0f;
    setColor(renderer, tokens.secondaryPanel.r, tokens.secondaryPanel.g, tokens.secondaryPanel.b, 230);
    fillRect(renderer, panelX + pad, footerY - 5.0f, panelW - pad * 2.0f, 20.0f);
    setColor(renderer, tokens.mutedText.r, tokens.mutedText.g, tokens.mutedText.b);
    debugTextCentered(renderer,
                      centerX,
                      footerY,
                      "UP/DOWN ROW   LEFT/RIGHT CHANGE   ENTER SELECT   ESC BACK");
}

} // namespace dragon
