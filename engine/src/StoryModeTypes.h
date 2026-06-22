#pragma once

#include "StoryModeDifficulty.h"

#include <array>
#include <string>

namespace dragon {

inline constexpr int kStoryMaxEnemies = 3;
inline constexpr int kStoryWaveCount = 3;

struct StoryModeState {
    std::string chapterTitle = "TMNT STREET PATROL";
    std::array<int, kStoryMaxEnemies> enemyCharacterIndices{ -1, -1, -1 };
    StoryDifficulty difficulty = StoryDifficulty::Medium;
    int waveIndex = 0;
    int activeWaveEnemyCount = 0;
    int enemiesDefeated = 0;
    int totalEnemies = 0;
    int waveTransitionTicks = 0;
    bool stageClear = false;
    bool stageFailed = false;
};

} // namespace dragon
