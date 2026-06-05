#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

std::string formatComboLabel(const FightComboSettings& settings, int hits) {
    std::string label = settings.text;
    const std::string count = std::to_string(hits);
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

void drawComboCounter(SDL_Renderer* renderer, const AppState& state, size_t attackerIndex) {
    if (attackerIndex >= state.comboCounters.size()) {
        return;
    }

    const auto& combo = state.comboCounters[attackerIndex];
    if (combo.displayTicks <= 0 || combo.displayHits < 2) {
        return;
    }

    const auto& settings = state.fightRoundSettings.combo;
    const bool labelIncludesCount = settings.text.find("%i") != std::string::npos;
    const std::string countText = labelIncludesCount ? std::string{} : std::to_string(combo.displayHits);
    const std::string label = formatComboLabel(settings, combo.displayHits);
    const float countWidth = static_cast<float>(countText.size()) * 8.0f;
    const float labelWidth = static_cast<float>(label.size()) * 8.0f;
    const float labelOffsetX = countText.empty() ? 0.0f : settings.textOffsetX;
    const float totalWidth = countWidth + labelOffsetX + labelWidth;
    const int displayTime = std::max(1, settings.displayTime);
    const int age = std::clamp(displayTime - combo.displayTicks, 0, displayTime);
    const float introT = std::clamp(static_cast<float>(age) / 8.0f, 0.0f, 1.0f);
    const float sideSign = attackerIndex == 0 ? 1.0f : -1.0f;
    const float slideOffset = settings.startX * (1.0f - introT) * sideSign;
    const float shakeOffset = settings.counterShake && age < 12
        ? static_cast<float>((state.frame / 2) % 3 - 1)
        : 0.0f;
    float x = attackerIndex == 0
        ? settings.posX
        : logicalWidthF(state) - settings.posX - totalWidth;
    x += slideOffset + shakeOffset;
    const float y = settings.posY;

    if (!countText.empty()) {
        setFightFontPaletteColor(renderer, settings.counterFontPalette, true);
        debugText(renderer, x, y, countText);
    }
    setFightFontPaletteColor(renderer, settings.textFontPalette, false);
    debugText(renderer, x + countWidth + labelOffsetX, y + settings.textOffsetY, label);
}

void drawPowerGauge(SDL_Renderer* renderer, const AppState& state, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }

    const bool p2 = fighterIndex == 1;
    const auto& settings = state.fightRoundSettings.powerbar;
    const float anchorX = motifOriginX(state) + (p2 ? settings.p2PosX : settings.p1PosX);
    const float y = p2 ? settings.p2PosY : settings.p1PosY;
    const float rangeStart = p2 ? settings.p2RangeStart : settings.p1RangeStart;
    const float rangeEnd = p2 ? settings.p2RangeEnd : settings.p1RangeEnd;
    const float barStart = anchorX + rangeStart;
    const float barEnd = anchorX + rangeEnd;
    const float barLeft = std::min(barStart, barEnd);
    const float barWidth = std::max(1.0f, std::abs(barEnd - barStart));
    constexpr float gaugeH = 5.0f;
    const int maxPower = std::max(1, state.characterConstants.maxPower);
    const float ratio = std::clamp(static_cast<float>(state.fighters[fighterIndex].power) / static_cast<float>(maxPower), 0.0f, 1.0f);
    const float fill = barWidth * ratio;

    setColor(renderer, 8, 10, 12, 230);
    fillRect(renderer, barLeft - 4.0f, y - 2.0f, barWidth + 8.0f, gaugeH + 4.0f);
    setColor(renderer, 60, 70, 88);
    drawRect(renderer, barLeft - 4.0f, y - 2.0f, barWidth + 8.0f, gaugeH + 4.0f);
    setColor(renderer, 236, 198, 74);
    if (barEnd < barStart) {
        fillRect(renderer, barStart - fill, y, fill, gaugeH);
    } else {
        fillRect(renderer, barStart, y, fill, gaugeH);
    }

    setColor(renderer, 178, 188, 204);
    debugText(renderer, p2 ? barEnd - 44.0f : barEnd + 6.0f, y - 3.0f, "POWER");
    const int stocks = state.fighters[fighterIndex].power / 1000;
    for (int i = 0; i < 3; ++i) {
        if (i < stocks) {
            setColor(renderer, 236, 198, 74);
        } else {
            setColor(renderer, 56, 62, 76);
        }
        const float pipX = p2 ? barEnd - 7.0f - static_cast<float>(i * 8) : barEnd + 6.0f + static_cast<float>(i * 8);
        fillRect(renderer, pipX, y + 7.0f, 5.0f, 3.0f);
    }
}

void drawFightHud(SDL_Renderer* renderer, const AppState& state) {
    const float widthF = logicalWidthF(state);
    const float centerX = screenCenterX(state);

    drawComboCounter(renderer, state, 0);
    drawComboCounter(renderer, state, 1);
    drawPowerGauge(renderer, state, 0);
    drawPowerGauge(renderer, state, 1);
    setColor(renderer, 8, 10, 12);
    constexpr float kLifeBarWidth = 130.0f;
    constexpr float kLifeBarFillWidth = 126.0f;
    constexpr float kLifeBarInset = 20.0f;
    const float p1BarX = 18.0f;
    const float p2BarX = widthF - p1BarX - kLifeBarWidth;
    fillRect(renderer, p1BarX, 10, kLifeBarWidth, 9);
    fillRect(renderer, p2BarX, 10, kLifeBarWidth, 9);
    setColor(renderer, 82, 190, 112);
    const float p1LifeWidth = kLifeBarFillWidth * std::clamp(static_cast<float>(state.fighters[0].life) / 1000.0f, 0.0f, 1.0f);
    const float p2LifeWidth = kLifeBarFillWidth * std::clamp(static_cast<float>(state.fighters[1].life) / 1000.0f, 0.0f, 1.0f);
    fillRect(renderer, kLifeBarInset, 12, p1LifeWidth, 5);
    fillRect(renderer, p2BarX + kLifeBarWidth - 2.0f - p2LifeWidth, 12, p2LifeWidth, 5);

    setColor(renderer, 222, 226, 232);
    debugText(renderer, 20, 24, selectedCharacterName(state));
    debugText(renderer, widthF - 106.0f, 24, compactSettingText(opponentDisplayName(state), 12));

    if (isMatchMode(state)) {
        const int seconds = std::max(0, (state.matchTimerTicks + 59) / 60);
        setColor(renderer, 8, 10, 12);
        fillRect(renderer, centerX - 28.0f, 22, 56, 13);
        setColor(renderer, 230, 220, 172);
        debugText(renderer, centerX - 20.0f, 25, std::to_string(seconds));
        setColor(renderer, 155, 164, 174);
        debugText(renderer, 82, 24, "R" + std::to_string(state.currentRound));
        drawRoundWinPips(renderer, 108, 27, state.roundWins[0], matchWinsRequired(state));
        drawRoundWinPips(renderer, widthF - 108.0f, 27, state.roundWins[1], matchWinsRequired(state), true);
    }

    setColor(renderer, 155, 164, 174);
    debugText(renderer, 20, 206,
        "P1 " + compactSettingText(selectedCharacterName(state), 11)
        + " vs " + compactSettingText(opponentDisplayName(state), 9));
    if (state.pendingMode == PendingMode::Training
        && state.trainingOptions.showHitLog
        && state.lastHitTextTicks > 0
        && !state.lastHitText.empty()) {
        setColor(renderer, 230, 190, 105);
        debugText(renderer, 20, 218, state.lastHitText);
    } else if (isMatchMode(state)) {
        const bool resultActive = isSingleFightResultPhase(state);
        setColor(renderer, resultActive ? 230 : 155, resultActive ? 190 : 164, resultActive ? 105 : 174);
        debugText(renderer, 20, 218, singleFightStatusLine(state));
    } else {
        if (!state.gamepads.empty()) {
            debugText(renderer, 20, 218, "Keys A/S/D Z/X/C  Pad " + gamepadActionLayoutText(state, 0));
        } else if (state.pendingMode == PendingMode::Training && state.trainingOptions.p2Controlled) {
            debugText(renderer, 20, 218, "P1 arrows A/S/D Z/X/C  P2 I/J/K/L U/O/P N/M/,");
        } else {
            debugText(renderer, 20, 218, "A/S/D Z/X/C  R reset  F1 boxes  F2 options");
        }
    }
}
