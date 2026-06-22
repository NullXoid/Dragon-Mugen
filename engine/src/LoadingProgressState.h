#pragma once

#include <algorithm>
#include <string>
#include <string_view>

namespace dragon {

struct LoadingProgressState {
    bool active = false;
    bool failed = false;
    float fraction = 0.0f;
    std::string phase = "Waiting";
};

inline void resetLoadingProgress(LoadingProgressState& progress) {
    progress = LoadingProgressState{};
}

inline void startLoadingProgress(LoadingProgressState& progress, std::string_view phase) {
    progress.active = true;
    progress.failed = false;
    progress.fraction = 0.0f;
    progress.phase = std::string(phase);
}

inline void updateLoadingProgress(LoadingProgressState& progress, float fraction, std::string_view phase) {
    progress.active = true;
    progress.failed = false;
    progress.fraction = std::clamp(fraction, 0.0f, 1.0f);
    progress.phase = std::string(phase);
}

inline void completeLoadingProgress(LoadingProgressState& progress, std::string_view phase = "Ready") {
    progress.active = true;
    progress.failed = false;
    progress.fraction = 1.0f;
    progress.phase = std::string(phase);
}

inline void failLoadingProgress(LoadingProgressState& progress, std::string_view phase) {
    progress.active = true;
    progress.failed = true;
    progress.fraction = 1.0f;
    progress.phase = std::string(phase);
}

inline float loadingProgressFraction(const LoadingProgressState& progress) {
    return std::clamp(progress.fraction, 0.0f, 1.0f);
}

inline int loadingProgressPercent(const LoadingProgressState& progress) {
    return static_cast<int>(loadingProgressFraction(progress) * 100.0f + 0.5f);
}

} // namespace dragon
