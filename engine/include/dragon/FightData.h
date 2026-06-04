#pragma once

#include <filesystem>
#include <string>

namespace dragon {

struct FightComboSettings {
    float posX = 10.0f;
    float posY = 80.0f;
    float startX = -40.0f;
    int counterFontPalette = 4;
    bool counterShake = true;
    std::string text = "Rush!";
    int textFontPalette = 0;
    float textOffsetX = 3.0f;
    float textOffsetY = 0.0f;
    int displayTime = 90;
};

struct FightPowerbarSettings {
    float p1PosX = 140.0f;
    float p1PosY = 22.0f;
    float p2PosX = 178.0f;
    float p2PosY = 22.0f;
    float p1RangeStart = 0.0f;
    float p1RangeEnd = -107.0f;
    float p2RangeStart = 0.0f;
    float p2RangeEnd = 107.0f;
};

struct FightRoundSettings {
    int matchWins = 2;
    int startWaitTime = 30;
    int roundDisplayTime = 60;
    int ctrlTime = 30;
    int koDisplayTime = 60;
    int dkoDisplayTime = 60;
    int timeOverDisplayTime = 60;
    int overWaitTime = 45;
    int overHitTime = 10;
    int overWinTime = 45;
    int overTime = 210;
    int winTime = 60;
    FightComboSettings combo;
    FightPowerbarSettings powerbar;
};

bool hasFightDefinition(const std::filesystem::path& gameRoot);
const char* fightDefinitionRequirementText();
FightRoundSettings loadFightRoundSettings(const std::filesystem::path& gameRoot);

} // namespace dragon
