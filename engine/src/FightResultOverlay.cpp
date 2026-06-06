#include "FightResultOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
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
    setColor(ui.renderer, 6, 8, 14, 210);
    fillRect(ui.renderer, centerX - 116.0f, 82, 232, 34);
    setColor(ui.renderer, view.r, view.g, view.b);
    fillRect(ui.renderer, centerX - 114.0f, 84, 228, 2);
    fillRect(ui.renderer, centerX - 114.0f, 114, 228, 2);
    debugTextCentered(ui.renderer, centerX, 96, fitDebugText(view.text, 28));
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
    setColor(ui.renderer, 6, 8, 14, 224);
    fillRect(ui.renderer, centerX - 98.0f, 76, 196, 70);
    setColor(ui.renderer, 230, 190, 105);
    drawRect(ui.renderer, centerX - 98.0f, 76, 196, 70);
    setColor(ui.renderer, 230, 190, 105);
    fillRect(ui.renderer, centerX - 96.0f, 78, 192, 2);

    setColor(ui.renderer, 222, 226, 232);
    debugTextCentered(ui.renderer, centerX, 94, fitDebugText(view.resultText, 24));
    setColor(ui.renderer, 230, 220, 172);
    drawRoundPips(ui, centerX - 42.0f, 112, view.p1RoundPips);
    setColor(ui.renderer, 230, 220, 172);
    debugTextCentered(ui.renderer, centerX, 111, "-");
    drawRoundPips(ui, centerX + 42.0f, 112, view.p2RoundPips);
    setColor(ui.renderer, 130, 142, 156);
    debugTextCentered(ui.renderer, centerX, 130, view.footerText);
}

void drawMatchResultScreen(const UiRenderContext& ui, const FightMatchResultView& view) {
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float heightF = static_cast<float>(ui.logicalHeight);
    const float centerX = widthF * 0.5f;
    setColor(ui.renderer, 6, 8, 14, 238);
    fillRect(ui.renderer, 0, 0, widthF, heightF);
    setColor(ui.renderer, 34, 40, 52);
    fillRect(ui.renderer, 0, 0, widthF, 52);
    setColor(ui.renderer, 158, 64, 58);
    fillRect(ui.renderer, 0, 52, widthF, 2);

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
            setColor(ui.renderer, 74, 170, 134);
            fillRect(ui.renderer, centerX - 64.0f, y - 3.0f, 128, 13);
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
