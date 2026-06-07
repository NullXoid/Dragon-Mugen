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
};

ArenaConfig loadArenaConfig(const std::filesystem::path& gameRoot);
ArenaConfig sanitizeArenaConfig(ArenaConfig config);

} // namespace dragon
