#pragma once

#include "AppTypes.h"

#include <array>
#include <cstdint>
#include <string>

namespace dragon {

struct FightRoundPipsView {
    int wins = 0;
    int required = 1;
    bool rightAligned = false;
    float size = 5.0f;
};

struct FightPowerGaugeView {
    int value = 0;
    int maxValue = 1;
    float anchorX = 0.0f;
    float y = 0.0f;
    float rangeStart = 0.0f;
    float rangeEnd = 0.0f;
};

struct FightComboCounterView {
    int displayHits = 0;
    int displayTicks = 0;
    int displayTime = 1;
    int frame = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    float startX = 0.0f;
    int counterFontPalette = 0;
    bool counterShake = false;
    std::string text;
    int textFontPalette = 0;
    float textOffsetX = 0.0f;
    float textOffsetY = 0.0f;
};

struct FighterHudView {
    std::string name;
    int life = 0;
    int maxLife = 1000;
    FightPowerGaugeView power;
    FightRoundPipsView roundPips;
};

struct FightHudView {
    FighterHudView p1;
    FighterHudView p2;
    std::array<FightComboCounterView, 2> comboCounters;
    bool showMatchTimer = false;
    int timerSeconds = 0;
    int currentRound = 1;
    std::string versusLine;
    std::string bottomLine;
    bool bottomLineHighlighted = false;
};

struct FightRoundCalloutView {
    bool visible = false;
    std::string text;
    std::uint8_t r = 222;
    std::uint8_t g = 226;
    std::uint8_t b = 232;
    int frame = 0;
};

struct FightRoundResultView {
    bool visible = false;
    std::string resultText;
    FightRoundPipsView p1RoundPips;
    FightRoundPipsView p2RoundPips;
    std::string footerText;
    int frame = 0;
};

struct FightResultMenuRowView {
    std::string label;
    bool selected = false;
};

struct FightMatchResultView {
    std::string modeLabel;
    std::string winnerText;
    std::string scoreText;
    std::string methodText;
    std::string quoteText;
    std::string stageText;
    std::array<FightResultMenuRowView, kMatchResultOptionCount> menuRows;
    int menuRowCount = 0;
    int frame = 0;
};

} // namespace dragon
