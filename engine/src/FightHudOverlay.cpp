#include "FightHudOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace dragon {
namespace {

std::string formatComboLabel(const FightComboCounterView& combo) {
    std::string label = combo.text;
    const std::string count = std::to_string(combo.displayHits);
    size_t at = 0;
    while ((at = label.find("%i", at)) != std::string::npos) {
        label.replace(at, 2, count);
        at += count.size();
    }
    return label;
}

void setFightFontPaletteColor(SDL_Renderer* renderer, int palette, bool counter) {
    switch (palette) {
    case 1:
        setColor(renderer, 128, 171, 225);
        break;
    case 2:
        setColor(renderer, 82, 190, 112);
        break;
    case 3:
        setColor(renderer, 230, 130, 120);
        break;
    case 4:
        setColor(renderer, 230, 190, 105);
        break;
    case 0:
    default:
        if (counter) {
            setColor(renderer, 238, 226, 188);
        } else {
            setColor(renderer, 222, 226, 232);
        }
        break;
    }
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

void drawComboCounter(const UiRenderContext& ui, const FightComboCounterView& combo, size_t attackerIndex) {
    if (combo.displayTicks <= 0 || combo.displayHits < 2) {
        return;
    }

    const bool labelIncludesCount = combo.text.find("%i") != std::string::npos;
    const std::string countText = labelIncludesCount ? std::string{} : std::to_string(combo.displayHits);
    const std::string label = formatComboLabel(combo);
    const float countWidth = static_cast<float>(countText.size()) * 8.0f;
    const float labelWidth = static_cast<float>(label.size()) * 8.0f;
    const float labelOffsetX = countText.empty() ? 0.0f : combo.textOffsetX;
    const float totalWidth = countWidth + labelOffsetX + labelWidth;
    const int displayTime = std::max(1, combo.displayTime);
    const int age = std::clamp(displayTime - combo.displayTicks, 0, displayTime);
    const float introT = std::clamp(static_cast<float>(age) / 8.0f, 0.0f, 1.0f);
    const float sideSign = attackerIndex == 0 ? 1.0f : -1.0f;
    const float slideOffset = combo.startX * (1.0f - introT) * sideSign;
    const float shakeOffset = combo.counterShake && age < 12
        ? static_cast<float>((combo.frame / 2) % 3 - 1)
        : 0.0f;
    float x = attackerIndex == 0
        ? combo.posX
        : static_cast<float>(ui.logicalWidth) - combo.posX - totalWidth;
    x += slideOffset + shakeOffset;
    const float y = combo.posY;

    if (!countText.empty()) {
        setFightFontPaletteColor(ui.renderer, combo.counterFontPalette, true);
        debugText(ui.renderer, x, y, countText);
    }
    setFightFontPaletteColor(ui.renderer, combo.textFontPalette, false);
    debugText(ui.renderer, x + countWidth + labelOffsetX, y + combo.textOffsetY, label);
}

void drawPowerGauge(const UiRenderContext& ui, const FightPowerGaugeView& power, bool p2) {
    const float barStart = power.anchorX + power.rangeStart;
    const float barEnd = power.anchorX + power.rangeEnd;
    const float barLeft = std::min(barStart, barEnd);
    const float barWidth = std::max(1.0f, std::abs(barEnd - barStart));
    constexpr float gaugeH = 5.0f;
    const int maxPower = std::max(1, power.maxValue);
    const float ratio = std::clamp(static_cast<float>(power.value) / static_cast<float>(maxPower), 0.0f, 1.0f);
    const float fill = barWidth * ratio;

    setColor(ui.renderer, 8, 10, 12, 230);
    fillRect(ui.renderer, barLeft - 4.0f, power.y - 2.0f, barWidth + 8.0f, gaugeH + 4.0f);
    setColor(ui.renderer, 60, 70, 88);
    drawRect(ui.renderer, barLeft - 4.0f, power.y - 2.0f, barWidth + 8.0f, gaugeH + 4.0f);
    setColor(ui.renderer, 236, 198, 74);
    if (barEnd < barStart) {
        fillRect(ui.renderer, barStart - fill, power.y, fill, gaugeH);
    } else {
        fillRect(ui.renderer, barStart, power.y, fill, gaugeH);
    }

    setColor(ui.renderer, 178, 188, 204);
    debugText(ui.renderer, p2 ? barEnd - 44.0f : barEnd + 6.0f, power.y - 3.0f, "POWER");
    const int stocks = power.value / 1000;
    for (int i = 0; i < 3; ++i) {
        if (i < stocks) {
            setColor(ui.renderer, 236, 198, 74);
        } else {
            setColor(ui.renderer, 56, 62, 76);
        }
        const float pipX = p2 ? barEnd - 7.0f - static_cast<float>(i * 8) : barEnd + 6.0f + static_cast<float>(i * 8);
        fillRect(ui.renderer, pipX, power.y + 7.0f, 5.0f, 3.0f);
    }
}

float lifeBarFillWidth(const FighterHudView& fighter, float maxWidth) {
    const int maxLife = std::max(1, fighter.maxLife);
    return maxWidth * std::clamp(static_cast<float>(fighter.life) / static_cast<float>(maxLife), 0.0f, 1.0f);
}

} // namespace

void drawFightHud(const UiRenderContext& ui, const FightHudView& view) {
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float centerX = widthF * 0.5f;

    drawComboCounter(ui, view.comboCounters[0], 0);
    drawComboCounter(ui, view.comboCounters[1], 1);
    drawPowerGauge(ui, view.p1.power, false);
    drawPowerGauge(ui, view.p2.power, true);

    setColor(ui.renderer, 8, 10, 12);
    constexpr float kLifeBarWidth = 130.0f;
    constexpr float kLifeBarFillWidth = 126.0f;
    constexpr float kLifeBarInset = 20.0f;
    const float p1BarX = 18.0f;
    const float p2BarX = widthF - p1BarX - kLifeBarWidth;
    fillRect(ui.renderer, p1BarX, 10, kLifeBarWidth, 9);
    fillRect(ui.renderer, p2BarX, 10, kLifeBarWidth, 9);
    setColor(ui.renderer, 82, 190, 112);
    const float p1LifeWidth = lifeBarFillWidth(view.p1, kLifeBarFillWidth);
    const float p2LifeWidth = lifeBarFillWidth(view.p2, kLifeBarFillWidth);
    fillRect(ui.renderer, kLifeBarInset, 12, p1LifeWidth, 5);
    fillRect(ui.renderer, p2BarX + kLifeBarWidth - 2.0f - p2LifeWidth, 12, p2LifeWidth, 5);

    setColor(ui.renderer, 222, 226, 232);
    debugText(ui.renderer, 20, 24, view.p1.name);
    debugText(ui.renderer, widthF - 106.0f, 24, view.p2.name);

    if (view.showMatchTimer) {
        setColor(ui.renderer, 8, 10, 12);
        fillRect(ui.renderer, centerX - 28.0f, 22, 56, 13);
        setColor(ui.renderer, 230, 220, 172);
        debugText(ui.renderer, centerX - 20.0f, 25, std::to_string(view.timerSeconds));
        setColor(ui.renderer, 155, 164, 174);
        debugText(ui.renderer, 82, 24, "R" + std::to_string(view.currentRound));
        drawRoundPips(ui, 108, 27, view.p1.roundPips);
        drawRoundPips(ui, widthF - 108.0f, 27, view.p2.roundPips);
    }

    setColor(ui.renderer, 155, 164, 174);
    debugText(ui.renderer, 20, 206, view.versusLine);
    if (!view.bottomLine.empty()) {
        setColor(
            ui.renderer,
            view.bottomLineHighlighted ? 230 : 155,
            view.bottomLineHighlighted ? 190 : 164,
            view.bottomLineHighlighted ? 105 : 174);
        debugText(ui.renderer, 20, 218, view.bottomLine);
    }
}

} // namespace dragon
