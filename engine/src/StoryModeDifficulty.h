#pragma once

#include <algorithm>
#include <cmath>
#include <string_view>

namespace dragon {

enum class StoryDifficulty {
    Easy = 0,
    Medium = 1,
    Hard = 2,
};

inline constexpr int kStoryDifficultyCount = 3;

struct StoryDifficultyTuning {
    int lifePermille = 1000;
    float attackMultiplier = 1.0f;
    float defenceMultiplier = 1.0f;
};

inline int storyDifficultyIndex(StoryDifficulty difficulty) {
    return std::clamp(static_cast<int>(difficulty), 0, kStoryDifficultyCount - 1);
}

inline StoryDifficulty storyDifficultyFromIndex(int index) {
    return static_cast<StoryDifficulty>(std::clamp(index, 0, kStoryDifficultyCount - 1));
}

inline StoryDifficulty cycleStoryDifficulty(StoryDifficulty difficulty, int direction) {
    const int next = (storyDifficultyIndex(difficulty) + direction + kStoryDifficultyCount) % kStoryDifficultyCount;
    return storyDifficultyFromIndex(next);
}

inline std::string_view storyDifficultyLabel(StoryDifficulty difficulty) {
    switch (difficulty) {
    case StoryDifficulty::Easy:
        return "EASY";
    case StoryDifficulty::Hard:
        return "HARD";
    case StoryDifficulty::Medium:
    default:
        return "MEDIUM";
    }
}

inline std::string_view storyDifficultyShortLabel(StoryDifficulty difficulty) {
    switch (difficulty) {
    case StoryDifficulty::Easy:
        return "EASY";
    case StoryDifficulty::Hard:
        return "HARD";
    case StoryDifficulty::Medium:
    default:
        return "MED";
    }
}

inline StoryDifficultyTuning storyDifficultyTuning(StoryDifficulty difficulty) {
    switch (difficulty) {
    case StoryDifficulty::Easy:
        return StoryDifficultyTuning{ 850, 0.85f, 0.85f };
    case StoryDifficulty::Hard:
        return StoryDifficultyTuning{ 1200, 1.15f, 1.15f };
    case StoryDifficulty::Medium:
    default:
        return StoryDifficultyTuning{ 1000, 1.0f, 1.0f };
    }
}

inline int scaleStoryDifficultyLife(int baseLife, StoryDifficulty difficulty) {
    const int safeBaseLife = std::max(1, baseLife);
    const int permille = std::max(1, storyDifficultyTuning(difficulty).lifePermille);
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(safeBaseLife * permille) / 1000.0f)));
}

} // namespace dragon
