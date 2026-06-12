#pragma once

#include <filesystem>
#include <string>

namespace dragon {

struct ArenaConfig {
    std::string modeName = "Arena Mode";
    std::string description = "Free-for-all battle with up to 4 fighters.";
    std::filesystem::path defaultStage = "stages/kfm.def";
    int cpuCountDefault = 1;
    int cpuCountMin = 1;
    int cpuCountMax = 3;
    bool playerSelect = true;
    std::string cpuSelect = "random";
    std::string ruleType = "free_for_all";
    std::string winCondition = "last_fighter_standing";
    bool allowTeams = false;
    std::string winTitle = "Winner";
    std::string endTitle = "Arena Match Complete";
    int timerDefault = 0;
    bool zAxisDefault = true;
    float depthMin = -48.0f;
    float depthMax = 48.0f;
    float depthProjectionScale = 0.65f;
    float depthMoveSpeed = 1.5f;
    float depthSidestepDistance = 24.0f;
    int depthSidestepFrames = 10;
    int depthModifierDoubleTapFrames = 18;
    float fighterDepthHitTolerance = 16.0f;
    float projectileDepthHitTolerance = 12.0f;
    bool cameraRotationDefault = false;
    float cameraRotationMaxYawDeg = 18.0f;
    float cameraRotationEase = 0.10f;
};

ArenaConfig loadArenaConfig(const std::filesystem::path& gameRoot);
ArenaConfig sanitizeArenaConfig(ArenaConfig config);

} // namespace dragon
