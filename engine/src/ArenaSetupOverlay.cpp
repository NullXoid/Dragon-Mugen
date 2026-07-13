#include "ArenaSetupOverlay.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace dragon {
namespace {

struct ArenaSetupCanvas {
    float width = 640.0f;
    float height = 360.0f;
    bool hd = true;
};

ArenaSetupCanvas arenaSetupCanvas(const UiRenderContext& ui) {
    (void)ui;
    return { 640.0f, 360.0f, true };
}

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
    const ArenaSetupCanvas canvas = arenaSetupCanvas(ui);
    ScopedVirtualCanvas virtualCanvas(ui, canvas.width, canvas.height);
    const DragonUiTokens tokens = dragonUiTokens();
    const float widthF = canvas.width;
    const float heightF = canvas.height;
    const float centerX = widthF * 0.5f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.14f);
    const float topBarH = canvas.hd ? 42.0f : 24.0f;
    const float panelW = canvas.hd ? 456.0f : std::min(286.0f, widthF - 24.0f);
    const float panelH = canvas.hd ? 276.0f : 168.0f;
    const float panelX = centerX - panelW * 0.5f;
    const float panelY = canvas.hd ? 58.0f : 44.0f;
    const float pad = canvas.hd ? 16.0f : 6.0f;
    const float rowH = canvas.hd ? 16.0f : 10.0f;
    const float rowStart = canvas.hd ? panelY + 96.0f : panelY + 76.0f;
    const int rowFit = canvas.hd ? 48 : 34;

    setColor(renderer, 6, 8, 12, 142);
    fillRect(renderer, 0, 0, widthF, heightF);
    setColor(renderer, tokens.panelBase.r, tokens.panelBase.g, tokens.panelBase.b, 232);
    fillRect(renderer, 0, 0, widthF, topBarH);
    setColor(renderer, tokens.separatorRed.r, tokens.separatorRed.g, tokens.separatorRed.b);
    fillRect(renderer, 0, topBarH, widthF, canvas.hd ? 2.0f : 1.0f);

    setColor(renderer, tokens.mutedGold.r, tokens.mutedGold.g, tokens.mutedGold.b);
    debugText(renderer, canvas.hd ? 28.0f : 10.0f, canvas.hd ? 13.0f : 7.0f, view.title);
    setColor(renderer, tokens.mutedText.r, tokens.mutedText.g, tokens.mutedText.b);
    const std::string description = fitDebugText(view.description, canvas.hd ? 42 : 18);
    debugTextCentered(renderer, centerX, canvas.hd ? 13.0f : 7.0f, description);

    setColor(renderer, tokens.panelBase.r, tokens.panelBase.g, tokens.panelBase.b, 226);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.primaryTeal.r, tokens.primaryTeal.g, tokens.primaryTeal.b);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.mutedGold.r, tokens.mutedGold.g, tokens.mutedGold.b);
    fillRect(renderer, panelX + pad, panelY + pad, panelW - pad * 2.0f, canvas.hd ? 2.0f : 1.0f);
    setColor(renderer, tokens.separatorRed.r, tokens.separatorRed.g, tokens.separatorRed.b);
    fillRect(renderer, panelX + pad, panelY + (canvas.hd ? 58.0f : 36.0f), panelW - pad * 2.0f, canvas.hd ? 2.0f : 1.0f);

    setColor(renderer, tokens.primaryText.r, tokens.primaryText.g, tokens.primaryText.b);
    debugText(renderer, panelX + pad * 1.5f, panelY + (canvas.hd ? 22.0f : 14.0f), "FIGHTER");
    debugText(renderer, panelX + panelW * 0.38f, panelY + (canvas.hd ? 22.0f : 14.0f), fitDebugText(view.fighterName, canvas.hd ? 28 : 12));
    setColor(renderer, tokens.mutedText.r, tokens.mutedText.g, tokens.mutedText.b);
    debugText(renderer, panelX + pad * 1.5f, panelY + (canvas.hd ? 40.0f : 26.0f), "MODE");
    debugText(renderer, panelX + panelW * 0.38f, panelY + (canvas.hd ? 40.0f : 26.0f), fitDebugText(view.modeLabel, canvas.hd ? 28 : 14));
    debugText(renderer, panelX + pad * 1.5f, panelY + (canvas.hd ? 78.0f : 48.0f), "DEPTH");
    debugText(renderer,
              panelX + panelW * 0.38f,
              panelY + (canvas.hd ? 78.0f : 48.0f),
              fitDebugText(view.zAxisEnabled ? "SHIFT+UP/DOWN" : "OFF", canvas.hd ? 30 : 15));

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

    const float footerY = panelY + panelH - (canvas.hd ? 26.0f : 14.0f);
    setColor(renderer, tokens.secondaryPanel.r, tokens.secondaryPanel.g, tokens.secondaryPanel.b, 230);
    fillRect(renderer, panelX + pad, footerY - 5.0f, panelW - pad * 2.0f, canvas.hd ? 20.0f : 12.0f);
    setColor(renderer, tokens.mutedText.r, tokens.mutedText.g, tokens.mutedText.b);
    debugTextCentered(renderer,
                      centerX,
                      footerY,
                      canvas.hd ? "UP/DOWN ROW   LEFT/RIGHT CHANGE   ENTER SELECT   ESC BACK"
                                : "UP/DOWN ROW  L/R CHANGE  ENT  ESC");
}

} // namespace dragon
