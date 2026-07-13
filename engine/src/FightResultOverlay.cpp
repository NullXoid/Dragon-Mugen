#include "FightResultOverlay.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace dragon {
namespace {

constexpr float kFightResultWidth = 640.0f;
constexpr float kFightResultHeight = 360.0f;

void drawRoundPips(const UiRenderContext& ui, float x, float y, FightRoundPipsView pips) {
    const auto& tokens = dragonUiTokens();
    pips.required = std::clamp(pips.required, 1, 5);
    pips.wins = std::clamp(pips.wins, 0, pips.required);
    const float gap = 3.0f;
    const float totalWidth = static_cast<float>(pips.required) * pips.size + static_cast<float>(pips.required - 1) * gap;
    const float startX = pips.rightAligned ? x - totalWidth : x;
    for (int i = 0; i < pips.required; ++i) {
        const float pipX = startX + static_cast<float>(i) * (pips.size + gap);
        if (i < pips.wins) {
            setColor(ui.renderer, tokens.mutedGold);
            fillRect(ui.renderer, pipX, y, pips.size, pips.size);
            setColor(ui.renderer, 42, 32, 12);
            drawRect(ui.renderer, pipX, y, pips.size, pips.size);
        } else {
            setColor(ui.renderer, tokens.secondaryPanel, 210);
            fillRect(ui.renderer, pipX, y, pips.size, pips.size);
            setColor(ui.renderer, tokens.mutedText);
            drawRect(ui.renderer, pipX, y, pips.size, pips.size);
        }
    }
}

void drawRoundCalloutBand(const UiRenderContext& ui, const FightRoundCalloutView& view) {
    if (!view.visible) {
        return;
    }

    ScopedVirtualCanvas virtualCanvas(ui, kFightResultWidth, kFightResultHeight);

    constexpr float centerX = kFightResultWidth * 0.5f;
    constexpr float scale = kFightResultHeight / 240.0f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.18f);
    const float pop = std::max(0.0f, 1.0f - static_cast<float>(view.frame) / 14.0f);
    const float bandW = std::min(kFightResultWidth - 48.0f, (232.0f + pop * 22.0f) * scale);
    const float bandY = 75.0f * scale;
    const float panelY = 82.0f * scale;
    const auto& tokens = dragonUiTokens();

    setColor(ui.renderer, tokens.panelBase, 180);
    fillRect(ui.renderer, 0.0f, bandY, kFightResultWidth, 48.0f * scale);
    setColor(ui.renderer, tokens.panelBase, 226);
    fillRect(ui.renderer, centerX - bandW * 0.5f, panelY, bandW, 34.0f * scale);
    setColor(ui.renderer, tokens.secondaryPanel, 236);
    fillRect(ui.renderer, centerX - bandW * 0.5f + 3.0f, panelY + 3.0f * scale, bandW - 6.0f, 28.0f * scale);
    setColor(ui.renderer, view.r, view.g, view.b, static_cast<Uint8>(176 + pulse * 64.0f));
    fillRect(ui.renderer, centerX - bandW * 0.5f + 5.0f, panelY + 2.0f * scale, bandW - 10.0f, 2.0f);
    fillRect(ui.renderer, centerX - bandW * 0.5f + 5.0f, panelY + 32.0f * scale, bandW - 10.0f, 2.0f);
    setColor(ui.renderer, tokens.separatorRed, 190);
    fillRect(ui.renderer, centerX - 86.0f * scale, panelY + 7.0f * scale, 172.0f * scale, 1.0f);
    setColor(ui.renderer, 6, 8, 12);
    debugTextCentered(ui.renderer, centerX + 1.0f, panelY + 15.0f * scale, fitDebugText(view.text, 28));
    setColor(ui.renderer, view.r, view.g, view.b);
    debugTextCentered(ui.renderer, centerX, panelY + 14.0f * scale, fitDebugText(view.text, 28));
}

} // namespace

void drawRoundStartOverlay(const UiRenderContext& ui, const FightRoundCalloutView& view) {
    drawRoundCalloutBand(ui, view);
}

void drawRoundFinishOverlay(const UiRenderContext& ui, const FightRoundCalloutView& view) {
    drawRoundCalloutBand(ui, view);
}

void drawRoundResultOverlay(const UiRenderContext& ui, const FightRoundResultView& view) {
    if (!view.visible) {
        return;
    }

    ScopedVirtualCanvas virtualCanvas(ui, kFightResultWidth, kFightResultHeight);

    constexpr float centerX = kFightResultWidth * 0.5f;
    constexpr float scale = kFightResultHeight / 240.0f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.15f);
    const auto& tokens = dragonUiTokens();
    setColor(ui.renderer, tokens.panelBase, 180);
    fillRect(ui.renderer, centerX - 130.0f * scale, 68.0f * scale, 260.0f * scale, 86.0f * scale);
    setColor(ui.renderer, tokens.panelBase, 232);
    fillRect(ui.renderer, centerX - 104.0f * scale, 74.0f * scale, 208.0f * scale, 74.0f * scale);
    setColor(ui.renderer, tokens.secondaryPanel, 238);
    fillRect(ui.renderer, centerX - 100.0f * scale, 78.0f * scale, 200.0f * scale, 24.0f * scale);
    setColor(ui.renderer, tokens.mutedGold);
    drawRect(ui.renderer, centerX - 104.0f * scale, 74.0f * scale, 208.0f * scale, 74.0f * scale);
    setColor(ui.renderer, tokens.mutedGold, static_cast<Uint8>(170 + pulse * 60.0f));
    fillRect(ui.renderer, centerX - 100.0f * scale, 77.0f * scale, 200.0f * scale, 2.0f);
    fillRect(ui.renderer, centerX - 100.0f * scale, 145.0f * scale, 200.0f * scale, 2.0f);

    setColor(ui.renderer, tokens.primaryText);
    debugTextCentered(ui.renderer, centerX, 94.0f * scale, fitDebugText(view.resultText, 24));
    if (view.p1RoundPips.required > 0 || view.p2RoundPips.required > 0) {
        setColor(ui.renderer, tokens.mutedGold);
        drawRoundPips(ui, centerX - 42.0f * scale, 112.0f * scale, view.p1RoundPips);
        setColor(ui.renderer, tokens.mutedGold);
        debugTextCentered(ui.renderer, centerX, 111.0f * scale, "-");
        drawRoundPips(ui, centerX + 42.0f * scale, 112.0f * scale, view.p2RoundPips);
    }
    setColor(ui.renderer, tokens.mutedText);
    debugTextCentered(ui.renderer, centerX, 130.0f * scale, view.footerText);
}

void drawMatchResultScreen(const UiRenderContext& ui, const FightMatchResultView& view) {
    ScopedVirtualCanvas virtualCanvas(ui, kFightResultWidth, kFightResultHeight);
    constexpr float widthF = kFightResultWidth;
    constexpr float heightF = kFightResultHeight;
    const float centerX = widthF * 0.5f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.12f);
    const auto& tokens = dragonUiTokens();

    {
        setColor(ui.renderer, tokens.panelBase, 192);
        fillRect(ui.renderer, 0, 0, widthF, heightF);
        setColor(ui.renderer, tokens.panelBase, 240);
        fillRect(ui.renderer, 0, 0, widthF, 38.0f);
        setColor(ui.renderer, tokens.separatorRed, 235);
        fillRect(ui.renderer, 0, 38.0f, widthF, 2.0f);

        setColor(ui.renderer, tokens.mutedGold);
        debugText(ui.renderer, 18.0f, 14.0f, "MATCH COMPLETE");
        setColor(ui.renderer, tokens.primaryTeal);
        debugTextCentered(ui.renderer, centerX, 14.0f, view.modeLabel);

        const float panelW = std::min(548.0f, widthF - 64.0f);
        const float panelX = centerX - panelW * 0.5f;
        const float panelY = 58.0f;
        const float panelH = 176.0f;
        setColor(ui.renderer, tokens.panelBase, 236);
        fillRect(ui.renderer, panelX, panelY, panelW, panelH);
        setColor(ui.renderer, tokens.mutedText, 235);
        drawRect(ui.renderer, panelX, panelY, panelW, panelH);
        setColor(ui.renderer, tokens.secondaryPanel, 230);
        fillRect(ui.renderer, panelX + 1.0f, panelY + 1.0f, panelW - 2.0f, 24.0f);
        setColor(ui.renderer, tokens.mutedGold, 235);
        fillRect(ui.renderer, panelX + 6.0f, panelY + 6.0f, panelW - 12.0f, 2.0f);
        setColor(ui.renderer, tokens.separatorRed, 235);
        fillRect(ui.renderer, panelX + 1.0f, panelY + 25.0f, panelW - 2.0f, 2.0f);

        setColor(ui.renderer, tokens.primaryText);
        debugTextCentered(ui.renderer, centerX, panelY + 42.0f, fitDebugText(view.winnerText, 40));
        setColor(ui.renderer, tokens.mutedGold);
        debugTextCentered(ui.renderer, centerX, panelY + 70.0f, view.scoreText);
        setColor(ui.renderer, tokens.mutedText);
        debugTextCentered(ui.renderer, centerX, panelY + 92.0f, view.methodText);

        float infoY = panelY + 116.0f;
        if (!view.progressionText.empty()) {
            setColor(ui.renderer, SDL_Color{ 124, 222, 170, 255 });
            debugTextCentered(ui.renderer, centerX, infoY, fitDebugText(view.progressionText, 60));
            infoY += 16.0f;
        }
        setColor(ui.renderer, tokens.mutedText);
        if (!view.quoteText.empty()) {
            debugTextCentered(ui.renderer, centerX, infoY, fitDebugText("\"" + view.quoteText + "\"", 56));
            infoY += 16.0f;
        }
        debugTextCentered(ui.renderer, centerX, infoY, fitDebugText(view.stageText, 48));

        const int rowCount = std::clamp(view.menuRowCount, 0, static_cast<int>(view.menuRows.size()));
        const float menuW = 258.0f;
        const float menuX = centerX - menuW * 0.5f;
        const float menuY = 246.0f;
        const float menuH = std::max(72.0f, 20.0f * static_cast<float>(std::max(1, rowCount)) + 34.0f);
        setColor(ui.renderer, tokens.panelBase, 220);
        fillRect(ui.renderer, menuX, menuY - 12.0f, menuW, menuH);
        setColor(ui.renderer, tokens.primaryTeal, 200);
        drawRect(ui.renderer, menuX, menuY - 12.0f, menuW, menuH);
        for (int i = 0; i < rowCount; ++i) {
            const auto& row = view.menuRows[static_cast<size_t>(i)];
            const float y = menuY + static_cast<float>(i * 20);
            if (row.selected) {
                setColor(ui.renderer, tokens.primaryTeal, static_cast<Uint8>(190 + pulse * 48.0f));
                fillRect(ui.renderer, centerX - 96.0f, y - 5.0f, 192.0f, 16.0f);
                setColor(ui.renderer, tokens.mutedGold);
                fillRect(ui.renderer, centerX - 90.0f, y + 12.0f, 180.0f, 1.0f);
                setColor(ui.renderer, 8, 12, 16);
            } else {
                setColor(ui.renderer, tokens.mutedText);
            }
            debugTextCentered(ui.renderer, centerX, y, fitDebugText(row.label, 26));
        }

        setColor(ui.renderer, tokens.mutedText);
        debugTextCentered(ui.renderer, centerX, menuY + 20.0f * static_cast<float>(rowCount) + 12.0f, "ENTER SELECT");
    }
}

} // namespace dragon
