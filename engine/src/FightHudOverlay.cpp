#include "FightHudOverlay.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"

#include <algorithm>
#include <array>
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
    const float pulse = std::max(0.0f, 1.0f - static_cast<float>(age) / 12.0f);
    const float backplatePad = 5.0f + pulse * 3.0f;
    const float backplateX = x - backplatePad;
    const float backplateY = y - 5.0f - pulse * 1.0f;
    const float backplateW = std::max(52.0f, totalWidth + backplatePad * 2.0f);

    setColor(ui.renderer, 6, 8, 12, 190);
    fillRect(ui.renderer, backplateX, backplateY, backplateW, 19.0f + pulse * 2.0f);
    setColor(ui.renderer, 158, 64, 58, static_cast<Uint8>(130 + pulse * 70.0f));
    fillRect(ui.renderer, backplateX + 2.0f, backplateY + 2.0f, backplateW - 4.0f, 2.0f);
    setColor(ui.renderer, 230, 190, 105, static_cast<Uint8>(110 + pulse * 90.0f));
    drawRect(ui.renderer, backplateX, backplateY, backplateW, 19.0f + pulse * 2.0f);

    if (!countText.empty()) {
        setColor(ui.renderer, 8, 10, 12, 220);
        debugText(ui.renderer, x + 1.0f, y + 1.0f, countText);
        if (pulse > 0.0f) {
            setColor(ui.renderer, 255, 238, 150, static_cast<Uint8>(pulse * 120.0f));
            debugText(ui.renderer, x - 1.0f, y - 1.0f, countText);
        }
        setFightFontPaletteColor(ui.renderer, combo.counterFontPalette, true);
        debugText(ui.renderer, x, y, countText);
    }
    setColor(ui.renderer, 8, 10, 12, 220);
    debugText(ui.renderer, x + countWidth + labelOffsetX + 1.0f, y + combo.textOffsetY + 1.0f, label);
    setFightFontPaletteColor(ui.renderer, combo.textFontPalette, false);
    debugText(ui.renderer, x + countWidth + labelOffsetX, y + combo.textOffsetY, label);
}

float lifeBarFillWidth(const FighterHudView& fighter, float maxWidth) {
    const int maxLife = std::max(1, fighter.maxLife);
    return maxWidth * std::clamp(static_cast<float>(fighter.life) / static_cast<float>(maxLife), 0.0f, 1.0f);
}

float hudTextWidth(const std::string& text, float scale) {
    return static_cast<float>(text.size()) * 8.0f * scale;
}

void drawHudText(SDL_Renderer* renderer, float scale, float x, float y, const std::string& text, SDL_Color color) {
    setColor(renderer, color);
    scaledDebugText(renderer, scale, x, y, text);
}

void drawHudStatusPanel(const UiRenderContext& ui, const FightHudView& view) {
    if (view.versusLine.empty() && view.bottomLine.empty()) {
        return;
    }

    const auto& tokens = dragonUiTokens();
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float heightF = static_cast<float>(ui.logicalHeight);
    const float scale = std::max(1.0f, std::floor(dragonUiMetricsForContext(ui).pixelScale * 0.70f));
    const float panelX = std::max(24.0f, widthF * 0.025f);
    const float panelW = widthF - panelX * 2.0f;
    const float panelH = view.bottomLine.empty() ? 34.0f : 54.0f;
    const float panelY = heightF - panelH - 26.0f;
    const std::size_t maxChars = static_cast<std::size_t>(std::max(8.0f, (panelW - 28.0f) / (8.0f * scale)));

    setColor(ui.renderer, tokens.panelBase, 202);
    fillRect(ui.renderer, panelX, panelY, panelW, panelH);
    setColor(ui.renderer, tokens.primaryTeal, 214);
    drawRect(ui.renderer, panelX, panelY, panelW, panelH);
    setColor(ui.renderer, tokens.separatorRed, 185);
    fillRect(ui.renderer, panelX + 2.0f, panelY + 2.0f, panelW - 4.0f, 2.0f);

    if (!view.versusLine.empty()) {
        drawHudText(ui.renderer, scale, panelX + 14.0f, panelY + 13.0f, fitDebugText(view.versusLine, maxChars), tokens.mutedText);
    }
    if (!view.bottomLine.empty()) {
        const SDL_Color color = view.bottomLineHighlighted ? tokens.mutedGold : tokens.primaryText;
        const float y = view.versusLine.empty() ? panelY + 14.0f : panelY + 32.0f;
        drawHudText(ui.renderer, scale, panelX + 14.0f, y, fitDebugText(view.bottomLine, maxChars), color);
    }
}

void drawFighterBar(
    const UiRenderContext& ui,
    const FighterHudView& fighter,
    float x,
    float y,
    float width,
    SDL_Color fill,
    bool rightAligned) {
    const auto& tokens = dragonUiTokens();
    constexpr float barH = 20.0f;
    const float lifeW = lifeBarFillWidth(fighter, std::max(1.0f, width - 6.0f));

    setColor(ui.renderer, tokens.panelBase, 228);
    fillRect(ui.renderer, x, y, width, barH);
    setColor(ui.renderer, 76, 88, 110, 230);
    drawRect(ui.renderer, x, y, width, barH);
    setColor(ui.renderer, fill);
    if (rightAligned) {
        fillRect(ui.renderer, x + width - 3.0f - lifeW, y + 4.0f, lifeW, barH - 8.0f);
    } else {
        fillRect(ui.renderer, x + 3.0f, y + 4.0f, lifeW, barH - 8.0f);
    }

    const int maxPower = std::max(1, fighter.power.maxValue);
    const float powerW = std::clamp(static_cast<float>(fighter.power.value) / static_cast<float>(maxPower), 0.0f, 1.0f)
        * std::max(1.0f, width - 6.0f);
    setColor(ui.renderer, tokens.panelBase, 220);
    fillRect(ui.renderer, x, y + barH + 5.0f, width, 7.0f);
    setColor(ui.renderer, tokens.mutedGold, 230);
    if (rightAligned) {
        fillRect(ui.renderer, x + width - 3.0f - powerW, y + barH + 7.0f, powerW, 3.0f);
    } else {
        fillRect(ui.renderer, x + 3.0f, y + barH + 7.0f, powerW, 3.0f);
    }
}

void drawArenaHealthBars(const UiRenderContext& ui, const FightHudView& view) {
    const int count = std::clamp(view.arenaFighterCount, 1, static_cast<int>(view.arenaFighters.size()));
    const float widthF = static_cast<float>(ui.logicalWidth);
    const auto& tokens = dragonUiTokens();
    const float margin = std::max(32.0f, widthF * 0.045f);
    const float gap = 18.0f;
    const float reservedRight = 330.0f;
    const float usableW = std::max(360.0f, widthF - margin * 2.0f - (widthF >= 854.0f ? reservedRight : 0.0f));
    const float barW = std::max(126.0f, (usableW - gap * static_cast<float>(count - 1)) / static_cast<float>(count));
    const float y = widthF >= 854.0f ? 34.0f : 18.0f;
    const float textScale = widthF >= 854.0f ? 2.0f : 1.0f;
    const std::array<std::array<Uint8, 3>, 4> fills{{
        { 82, 190, 112 },
        { 236, 198, 74 },
        { 128, 171, 225 },
        { 230, 130, 120 },
    }};

    for (int i = 0; i < count; ++i) {
        const auto& fighter = view.arenaFighters[static_cast<size_t>(i)];
        const float x = margin + static_cast<float>(i) * (barW + gap);
        const auto& fill = fills[static_cast<size_t>(i % static_cast<int>(fills.size()))];
        drawFighterBar(ui, fighter, x, y, barW, SDL_Color{ fill[0], fill[1], fill[2], 255 }, false);
        const std::size_t nameChars = static_cast<std::size_t>(std::max(6.0f, barW / (8.0f * textScale)));
        drawHudText(ui.renderer, textScale, x, y + 36.0f, fitDebugText(fighter.name, nameChars), tokens.primaryText);
    }
}

void drawFightHudContent(const UiRenderContext& ui, const FightHudView& view) {
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float centerX = widthF * 0.5f;
    const auto& tokens = dragonUiTokens();

    drawComboCounter(ui, view.comboCounters[0], 0);
    drawComboCounter(ui, view.comboCounters[1], 1);
    if (view.arenaMode) {
        drawArenaHealthBars(ui, view);
        if (view.showMatchTimer) {
            setColor(ui.renderer, 8, 10, 12);
            fillRect(ui.renderer, centerX - 28.0f, 35, 56, 13);
            setColor(ui.renderer, 230, 220, 172);
            debugText(ui.renderer, centerX - static_cast<float>(view.timerText.size() * 4), 38, view.timerText);
        }
        drawHudStatusPanel(ui, view);
        return;
    }

    const float margin = std::max(36.0f, widthF * 0.045f);
    const float barW = std::clamp(widthF * 0.31f, 260.0f, 410.0f);
    const float barY = ui.logicalWidth >= 854 ? 34.0f : 18.0f;
    const float p1BarX = margin;
    const float p2BarX = widthF - margin - barW;
    drawFighterBar(ui, view.p1, p1BarX, barY, barW, tokens.primaryTeal, false);
    drawFighterBar(ui, view.p2, p2BarX, barY, barW, tokens.mutedGold, true);

    const float nameScale = ui.logicalWidth >= 854 ? 2.0f : 1.0f;
    drawHudText(ui.renderer, nameScale, p1BarX, barY + 36.0f, fitDebugText(view.p1.name, 18), tokens.primaryText);
    const std::string p2Name = fitDebugText(view.p2.name, 18);
    drawHudText(ui.renderer, nameScale, p2BarX + barW - hudTextWidth(p2Name, nameScale), barY + 36.0f, p2Name, tokens.primaryText);
    if (!view.p1.progressionLabel.empty()) {
        drawHudText(ui.renderer, nameScale, p1BarX, barY + 56.0f, fitDebugText(view.p1.progressionLabel, 18), tokens.primaryTeal);
    }
    if (!view.p2.progressionLabel.empty()) {
        const std::string progress = fitDebugText(view.p2.progressionLabel, 18);
        drawHudText(ui.renderer, nameScale, p2BarX + barW - hudTextWidth(progress, nameScale), barY + 56.0f, progress, tokens.primaryTeal);
    }

    if (view.showMatchTimer) {
        const float timerScale = ui.logicalWidth >= 854 ? 2.0f : 1.0f;
        const float timerW = std::max(72.0f, hudTextWidth(view.timerText, timerScale) + 34.0f);
        setColor(ui.renderer, tokens.panelBase, 230);
        fillRect(ui.renderer, centerX - timerW * 0.5f, barY + 5.0f, timerW, 30.0f);
        setColor(ui.renderer, tokens.mutedGold, 240);
        debugTextCentered(ui.renderer, centerX, barY + 12.0f, view.timerText);
        if (timerScale > 1.0f) {
            drawHudText(ui.renderer, timerScale, centerX - hudTextWidth(view.timerText, timerScale) * 0.5f, barY + 8.0f, view.timerText, tokens.mutedGold);
        }
        if (view.p1.roundPips.required > 0) {
            drawRoundPips(ui, p1BarX + barW + 18.0f, barY + 42.0f, view.p1.roundPips);
        }
        if (view.p2.roundPips.required > 0) {
            drawRoundPips(ui, p2BarX - 18.0f, barY + 42.0f, view.p2.roundPips);
        }
    }

    drawHudStatusPanel(ui, view);
}

} // namespace

void drawFightHud(const UiRenderContext& ui, const FightHudView& view) {
    drawFightHudContent(ui, view);
}

} // namespace dragon
