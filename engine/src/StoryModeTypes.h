#pragma once

#include "StoryBoardPlan.h"
#include "StoryModeDifficulty.h"

#include <array>
#include <string>
#include <vector>

namespace dragon {

inline constexpr int kStoryMaxEnemies = 3;
inline constexpr int kStoryWaveCount = 3;

struct StoryRewardPopup {
    float x = 0.0f;
    float y = 0.0f;
    float depthZ = 0.0f;
    int xp = 0;
    int gold = 0;
    int oldLevel = 1;
    int newLevel = 1;
    int ageTicks = 0;
    int durationTicks = 90;
};

struct StoryRewardCoin {
    float x = 0.0f;
    float y = 0.0f;
    float depthZ = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    int ageTicks = 0;
    int durationTicks = 75;
};

struct StoryModeState {
    std::string chapterTitle = "TMNT STREET PATROL";
    StoryBoardRoute boardRoute;
    std::array<int, kStoryMaxEnemies> enemyCharacterIndices{ -1, -1, -1 };
    StoryDifficulty difficulty = StoryDifficulty::Medium;
    int selectedBoardNode = 0;
    int activeBoardNode = 0;
    int waveIndex = 0;
    int activeWaveEnemyCount = 0;
    int enemiesDefeated = 0;
    int totalEnemies = 0;
    int waveTransitionTicks = 0;
    std::array<bool, kStoryMaxEnemies> enemyRewarded{ false, false, false };
    std::vector<StoryRewardPopup> rewardPopups;
    std::vector<StoryRewardCoin> rewardCoins;
    bool boardRouteLoaded = false;
    bool shopDoorAvailable = false;
    bool pendingShopDoorTransition = false;
    bool resumeRouteAfterShop = false;
    int resumeBoardNodeAfterShop = -1;
    bool stageClear = false;
    bool stageFailed = false;
};

} // namespace dragon
