#include "FightResultOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace dragon {
namespace {

struct FightResultCanvas {
    float width = 426.0f;
    float height = 240.0f;
    bool hd = false;
};

FightResultCanvas fightResultCanvas(const UiRenderContext& ui) {
    if (ui.logicalWidth >= 854 && ui.logicalHeight >= 480) {
        return { 640.0f, 360.0f, true };
    }
    return { ui.logicalWidth <= 340 ? 320.0f : 426.0f, 240.0f, false };
}

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

    const FightResultCanvas canvas = fightResultCanvas(ui);
    ScopedVirtualCanvas virtualCanvas(ui, canvas.width, canvas.height);

    const float centerX = canvas.width * 0.5f;
    const float scale = canvas.height / 240.0f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.18f);
    const float pop = std::max(0.0f, 1.0f - static_cast<float>(view.frame) / 14.0f);
    const float bandW = std::min(canvas.width - 48.0f, (232.0f + pop * 22.0f) * scale);
    const float bandY = 75.0f * scale;
    const float panelY = 82.0f * scale;

    setColor(ui.renderer, 4, 6, 10, 180);
    fillRect(ui.renderer, 0.0f, bandY, canvas.width, 48.0f * scale);
    setColor(ui.renderer, 6, 8, 14, 226);
    fillRect(ui.renderer, centerX - bandW * 0.5f, panelY, bandW, 34.0f * scale);
    setColor(ui.renderer, 30, 38, 58, 236);
    fillRect(ui.renderer, centerX - bandW * 0.5f + 3.0f, panelY + 3.0f * scale, bandW - 6.0f, 28.0f * scale);
    setColor(ui.renderer, view.r, view.g, view.b, static_cast<Uint8>(176 + pulse * 64.0f));
    fillRect(ui.renderer, centerX - bandW * 0.5f + 5.0f, panelY + 2.0f * scale, bandW - 10.0f, 2.0f);
    fillRect(ui.renderer, centerX - bandW * 0.5f + 5.0f, panelY + 32.0f * scale, bandW - 10.0f, 2.0f);
    setColor(ui.renderer, 158, 64, 58, 190);
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

    const FightResultCanvas canvas = fightResultCanvas(ui);
    ScopedVirtualCanvas virtualCanvas(ui, canvas.width, canvas.height);

    const float centerX = canvas.width * 0.5f;
    const float scale = canvas.height / 240.0f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.15f);
    setColor(ui.renderer, 4, 6, 10, 180);
    fillRect(ui.renderer, centerX - 130.0f * scale, 68.0f * scale, 260.0f * scale, 86.0f * scale);
    setColor(ui.renderer, 6, 8, 14, 232);
    fillRect(ui.renderer, centerX - 104.0f * scale, 74.0f * scale, 208.0f * scale, 74.0f * scale);
    setColor(ui.renderer, 24, 32, 48, 238);
    fillRect(ui.renderer, centerX - 100.0f * scale, 78.0f * scale, 200.0f * scale, 24.0f * scale);
    setColor(ui.renderer, 230, 190, 105);
    drawRect(ui.renderer, centerX - 104.0f * scale, 74.0f * scale, 208.0f * scale, 74.0f * scale);
    setColor(ui.renderer, 230, 190, 105, static_cast<Uint8>(170 + pulse * 60.0f));
    fillRect(ui.renderer, centerX - 100.0f * scale, 77.0f * scale, 200.0f * scale, 2.0f);
    fillRect(ui.renderer, centerX - 100.0f * scale, 145.0f * scale, 200.0f * scale, 2.0f);

    setColor(ui.renderer, 222, 226, 232);
    debugTextCentered(ui.renderer, centerX, 94.0f * scale, fitDebugText(view.resultText, 24));
    if (view.p1RoundPips.required > 0 || view.p2RoundPips.required > 0) {
        setColor(ui.renderer, 230, 220, 172);
        drawRoundPips(ui, centerX - 42.0f * scale, 112.0f * scale, view.p1RoundPips);
        setColor(ui.renderer, 230, 220, 172);
        debugTextCentered(ui.renderer, centerX, 111.0f * scale, "-");
        drawRoundPips(ui, centerX + 42.0f * scale, 112.0f * scale, view.p2RoundPips);
    }
    setColor(ui.renderer, 174, 184, 196);
    debugTextCentered(ui.renderer, centerX, 130.0f * scale, view.footerText);
}

void drawMatchResultScreen(const UiRenderContext& ui, const FightMatchResultView& view) {
    const FightResultCanvas canvas = fightResultCanvas(ui);
    ScopedVirtualCanvas virtualCanvas(ui, canvas.width, canvas.height);
    const float widthF = canvas.width;
    const float heightF = canvas.height;
    const float centerX = widthF * 0.5f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.12f);

    if (canvas.hd) {
        setColor(ui.renderer, 4, 6, 10, 192);
        fillRect(ui.renderer, 0, 0, widthF, heightF);
        setColor(ui.renderer, 7, 16, 25, 240);
        fillRect(ui.renderer, 0, 0, widthF, 38.0f);
        setColor(ui.renderer, 198, 79, 85, 235);
        fillRect(ui.renderer, 0, 38.0f, widthF, 2.0f);

        setColor(ui.renderer, 230, 220, 172);
        debugText(ui.renderer, 18.0f, 14.0f, "MATCH COMPLETE");
        setColor(ui.renderer, 128, 171, 225);
        debugTextCentered(ui.renderer, centerX, 14.0f, view.modeLabel);

        const float panelW = std::min(548.0f, widthF - 64.0f);
        const float panelX = centerX - panelW * 0.5f;
        const float panelY = 58.0f;
        const float panelH = 176.0f;
        setColor(ui.renderer, 7, 16, 25, 236);
        fillRect(ui.renderer, panelX, panelY, panelW, panelH);
        setColor(ui.renderer, 78, 90, 112, 235);
        drawRect(ui.renderer, panelX, panelY, panelW, panelH);
        setColor(ui.renderer, 16, 26, 39, 230);
        fillRect(ui.renderer, panelX + 1.0f, panelY + 1.0f, panelW - 2.0f, 24.0f);
        setColor(ui.renderer, 230, 190, 105, 235);
        fillRect(ui.renderer, panelX + 6.0f, panelY + 6.0f, panelW - 12.0f, 2.0f);
        setColor(ui.renderer, 198, 79, 85, 235);
        fillRect(ui.renderer, panelX + 1.0f, panelY + 25.0f, panelW - 2.0f, 2.0f);

        setColor(ui.renderer, 233, 237, 243);
        debugTextCentered(ui.renderer, centerX, panelY + 42.0f, fitDebugText(view.winnerText, 40));
        setColor(ui.renderer, 230, 190, 105);
        debugTextCentered(ui.renderer, centerX, panelY + 70.0f, view.scoreText);
        setColor(ui.renderer, 174, 184, 196);
        debugTextCentered(ui.renderer, centerX, panelY + 92.0f, view.methodText);

        float infoY = panelY + 116.0f;
        if (!view.progressionText.empty()) {
            setColor(ui.renderer, 124, 222, 170);
            debugTextCentered(ui.renderer, centerX, infoY, fitDebugText(view.progressionText, 60));
            infoY += 16.0f;
        }
        setColor(ui.renderer, 174, 184, 196);
        if (!view.quoteText.empty()) {
            debugTextCentered(ui.renderer, centerX, infoY, fitDebugText("\"" + view.quoteText + "\"", 56));
            infoY += 16.0f;
        }
        debugTextCentered(ui.renderer, centerX, infoY, fitDebugText(view.stageText, 48));

        const int rowCount = std::clamp(view.menuRowCount, 0, static_cast<int>(view.menuRows.size()));
        const float menuW = 258.0f;
        const float menuX = centerX - menuW * 0.5f;
        const float menuY = 254.0f;
        setColor(ui.renderer, 7, 16, 25, 220);
        fillRect(ui.renderer, menuX, menuY - 12.0f, menuW, std::max(42.0f, 20.0f * static_cast<float>(std::max(1, rowCount)) + 10.0f));
        setColor(ui.renderer, 26, 144, 138, 200);
        drawRect(ui.renderer, menuX, menuY - 12.0f, menuW, std::max(42.0f, 20.0f * static_cast<float>(std::max(1, rowCount)) + 10.0f));
        for (int i = 0; i < rowCount; ++i) {
            const auto& row = view.menuRows[static_cast<size_t>(i)];
            const float y = menuY + static_cast<float>(i * 20);
            if (row.selected) {
                setColor(ui.renderer, 74, 170, 134, static_cast<Uint8>(190 + pulse * 48.0f));
                fillRect(ui.renderer, centerX - 96.0f, y - 5.0f, 192.0f, 16.0f);
                setColor(ui.renderer, 230, 220, 172);
                fillRect(ui.renderer, centerX - 90.0f, y + 12.0f, 180.0f, 1.0f);
                setColor(ui.renderer, 8, 12, 16);
            } else {
                setColor(ui.renderer, 174, 184, 196);
            }
            debugTextCentered(ui.renderer, centerX, y, fitDebugText(row.label, 26));
        }

        setColor(ui.renderer, 130, 142, 156);
        debugTextCentered(ui.renderer, centerX, 340.0f, "ENTER SELECT");
        return;
    }

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
    float infoY = 128.0f;
    if (!view.progressionText.empty()) {
        setColor(ui.renderer, 124, 222, 170);
        debugTextCentered(ui.renderer, centerX, 128, fitDebugText(view.progressionText, 42));
        infoY = 142.0f;
    }
    if (!view.quoteText.empty()) {
        setColor(ui.renderer, 174, 184, 196);
        debugTextCentered(ui.renderer, centerX, infoY, fitDebugText("\"" + view.quoteText + "\"", 40));
        debugTextCentered(ui.renderer, centerX, infoY + 14.0f, fitDebugText(view.stageText, 34));
    } else {
        setColor(ui.renderer, 174, 184, 196);
        debugTextCentered(ui.renderer, centerX, infoY, fitDebugText(view.stageText, 34));
    }

    const int rowCount = std::clamp(view.menuRowCount, 0, static_cast<int>(view.menuRows.size()));
    const float menuStartY = !view.quoteText.empty()
        ? (view.progressionText.empty() ? 166.0f : 178.0f)
        : (view.progressionText.empty() ? 154.0f : 166.0f);
    for (int i = 0; i < rowCount; ++i) {
        const auto& row = view.menuRows[static_cast<size_t>(i)];
        const float y = menuStartY + static_cast<float>(i * 16);
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
