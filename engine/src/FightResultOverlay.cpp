#include "FightResultOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace dragon {
namespace {

void drawRoundPips(const UiRenderContext& ui, float x, float y, FightRoundPipsView pips) {
    pips.required = std::clamp(pips.required, 1, 5);
    pips.wins = std::clamp(pips.wins, 0, pips.required);
    const float gap = 3.0f;
    const float totalWidth = static_cast<float>(pips.required) * pips.size + static_cast<float>(pips.required - 1) * gap;
    const float startX = pips.rightAligned ? x - totalWidth : x;
    for (int i = 0; i < pips.required; ++i) {
        const float pipX = startX + static_cast<float>(i) * (pips.size + gap);
        if (i < pips.wins) {
            setColor(ui.renderer, 230, 190, 105);
            fillRect(ui.renderer, pipX, y, pips.size, pips.size);
            setColor(ui.renderer, 42, 32, 12);
            drawRect(ui.renderer, pipX, y, pips.size, pips.size);
        } else {
            setColor(ui.renderer, 42, 48, 58, 210);
            fillRect(ui.renderer, pipX, y, pips.size, pips.size);
            setColor(ui.renderer, 118, 130, 148);
            drawRect(ui.renderer, pipX, y, pips.size, pips.size);
        }
    }
}

void drawRoundCalloutBand(const UiRenderContext& ui, const FightRoundCalloutView& view) {
    if (!view.visible) {
        return;
    }

    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.18f);
    const float pop = std::max(0.0f, 1.0f - static_cast<float>(view.frame) / 14.0f);
    const float bandW = 232.0f + pop * 22.0f;

    setColor(ui.renderer, 4, 6, 10, 180);
    fillRect(ui.renderer, centerX - 160.0f, 75.0f, 320.0f, 48.0f);
    setColor(ui.renderer, 6, 8, 14, 226);
    fillRect(ui.renderer, centerX - bandW * 0.5f, 82.0f, bandW, 34.0f);
    setColor(ui.renderer, 30, 38, 58, 236);
    fillRect(ui.renderer, centerX - bandW * 0.5f + 3.0f, 85.0f, bandW - 6.0f, 28.0f);
    setColor(ui.renderer, view.r, view.g, view.b, static_cast<Uint8>(176 + pulse * 64.0f));
    fillRect(ui.renderer, centerX - bandW * 0.5f + 5.0f, 84.0f, bandW - 10.0f, 2.0f);
    fillRect(ui.renderer, centerX - bandW * 0.5f + 5.0f, 114.0f, bandW - 10.0f, 2.0f);
    setColor(ui.renderer, 158, 64, 58, 190);
    fillRect(ui.renderer, centerX - 86.0f, 89.0f, 172.0f, 1.0f);
    setColor(ui.renderer, 6, 8, 12);
    debugTextCentered(ui.renderer, centerX + 1.0f, 97.0f, fitDebugText(view.text, 28));
    setColor(ui.renderer, view.r, view.g, view.b);
    debugTextCentered(ui.renderer, centerX, 96.0f, fitDebugText(view.text, 28));
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

    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.15f);
    setColor(ui.renderer, 4, 6, 10, 180);
    fillRect(ui.renderer, centerX - 130.0f, 68, 260, 86);
    setColor(ui.renderer, 6, 8, 14, 232);
    fillRect(ui.renderer, centerX - 104.0f, 74, 208, 74);
    setColor(ui.renderer, 24, 32, 48, 238);
    fillRect(ui.renderer, centerX - 100.0f, 78, 200, 24);
    setColor(ui.renderer, 230, 190, 105);
    drawRect(ui.renderer, centerX - 104.0f, 74, 208, 74);
    setColor(ui.renderer, 230, 190, 105, static_cast<Uint8>(170 + pulse * 60.0f));
    fillRect(ui.renderer, centerX - 100.0f, 77, 200, 2);
    fillRect(ui.renderer, centerX - 100.0f, 145, 200, 2);

    setColor(ui.renderer, 222, 226, 232);
    debugTextCentered(ui.renderer, centerX, 94, fitDebugText(view.resultText, 24));
    setColor(ui.renderer, 230, 220, 172);
    drawRoundPips(ui, centerX - 42.0f, 112, view.p1RoundPips);
    setColor(ui.renderer, 230, 220, 172);
    debugTextCentered(ui.renderer, centerX, 111, "-");
    drawRoundPips(ui, centerX + 42.0f, 112, view.p2RoundPips);
    setColor(ui.renderer, 174, 184, 196);
    debugTextCentered(ui.renderer, centerX, 130, view.footerText);
}

void drawMatchResultScreen(const UiRenderContext& ui, const FightMatchResultView& view) {
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float heightF = static_cast<float>(ui.logicalHeight);
    const float centerX = widthF * 0.5f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.12f);
    setColor(ui.renderer, 6, 8, 14, 238);
    fillRect(ui.renderer, 0, 0, widthF, heightF);
    setColor(ui.renderer, 24, 32, 48);
    fillRect(ui.renderer, 0, 0, widthF, 54);
    setColor(ui.renderer, 10, 14, 22, 226);
    fillRect(ui.renderer, 18, 62, widthF - 36.0f, 94);
    setColor(ui.renderer, 78, 90, 112);
    drawRect(ui.renderer, 18, 62, widthF - 36.0f, 94);
    setColor(ui.renderer, 158, 64, 58);
    fillRect(ui.renderer, 0, 52, widthF, 2);
    setColor(ui.renderer, 230, 190, 105);
    fillRect(ui.renderer, 22, 64, widthF - 44.0f, 2);

    setColor(ui.renderer, 230, 220, 172);
    debugText(ui.renderer, 22, 18, "MATCH COMPLETE");
    setColor(ui.renderer, 128, 171, 225);
    debugText(ui.renderer, 198, 18, view.modeLabel);

    setColor(ui.renderer, 222, 226, 232);
    debugTextCentered(ui.renderer, centerX, 72, fitDebugText(view.winnerText, 28));
    setColor(ui.renderer, 230, 190, 105);
    debugTextCentered(ui.renderer, centerX, 94, view.scoreText);
    setColor(ui.renderer, 174, 184, 196);
    debugTextCentered(ui.renderer, centerX, 112, view.methodText);
    if (!view.quoteText.empty()) {
        debugTextCentered(ui.renderer, centerX, 128, fitDebugText("\"" + view.quoteText + "\"", 40));
        debugTextCentered(ui.renderer, centerX, 142, fitDebugText(view.stageText, 34));
    } else {
        debugTextCentered(ui.renderer, centerX, 128, fitDebugText(view.stageText, 34));
    }

    const int rowCount = std::clamp(view.menuRowCount, 0, static_cast<int>(view.menuRows.size()));
    for (int i = 0; i < rowCount; ++i) {
        const auto& row = view.menuRows[static_cast<size_t>(i)];
        const float y = (view.quoteText.empty() ? 154.0f : 166.0f) + static_cast<float>(i * 16);
        if (row.selected) {
            setColor(ui.renderer, 74, 170, 134, static_cast<Uint8>(190 + pulse * 48.0f));
            fillRect(ui.renderer, centerX - 70.0f, y - 4.0f, 140, 14);
            setColor(ui.renderer, 230, 220, 172);
            fillRect(ui.renderer, centerX - 66.0f, y + 10.0f, 132, 1);
            setColor(ui.renderer, 8, 12, 16);
        } else {
            setColor(ui.renderer, 174, 184, 196);
        }
        debugTextCentered(ui.renderer, centerX, y, row.label);
    }

    setColor(ui.renderer, 130, 142, 156);
    debugTextCentered(ui.renderer, centerX, 224, "ENTER select");
}

} // namespace dragon
