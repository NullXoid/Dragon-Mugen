#include "ArenaConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

namespace dragon {
namespace {

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

std::string unescapeJsonString(std::string value) {
    std::string out;
    out.reserve(value.size());
    bool escape = false;
    for (const char ch : value) {
        if (escape) {
            switch (ch) {
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            default:
                out.push_back(ch);
                break;
            }
            escape = false;
            continue;
        }
        if (ch == '\\') {
            escape = true;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::string findJsonString(const std::string& text, std::string_view key, std::string fallback) {
    const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"");
    std::smatch match;
    if (std::regex_search(text, match, pattern) && match.size() > 1) {
        return unescapeJsonString(match[1].str());
    }
    return fallback;
}

int findJsonInt(const std::string& text, std::string_view key, int fallback) {
    const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() <= 1) {
        return fallback;
    }

    try {
        return std::stoi(match[1].str());
    } catch (...) {
        return fallback;
    }
}

float findJsonFloat(const std::string& text, std::string_view key, float fallback) {
    const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() <= 1) {
        return fallback;
    }

    try {
        return std::stof(match[1].str());
    } catch (...) {
        return fallback;
    }
}

bool findJsonBool(const std::string& text, std::string_view key, bool fallback) {
    const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(text, match, pattern) && match.size() > 1) {
        return match[1].str() == "true";
    }
    return fallback;
}

bool blank(std::string_view value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}

} // namespace

ArenaConfig sanitizeArenaConfig(ArenaConfig config) {
    ArenaConfig defaults;

    if (blank(config.modeName)) {
        config.modeName = defaults.modeName;
    }
    if (blank(config.description)) {
        config.description = defaults.description;
    }
    if (config.defaultStage.empty()) {
        config.defaultStage = defaults.defaultStage;
    }
    config.cpuCountMin = std::clamp(config.cpuCountMin, 1, 3);
    config.cpuCountMax = std::clamp(config.cpuCountMax, config.cpuCountMin, 3);
    config.cpuCountDefault = std::clamp(config.cpuCountDefault, config.cpuCountMin, config.cpuCountMax);
    if (config.cpuSelect != "random") {
        config.cpuSelect = defaults.cpuSelect;
    }
    if (blank(config.ruleType)) {
        config.ruleType = defaults.ruleType;
    }
    if (blank(config.winCondition)) {
        config.winCondition = defaults.winCondition;
    }
    if (blank(config.winTitle)) {
        config.winTitle = defaults.winTitle;
    }
    if (blank(config.endTitle)) {
        config.endTitle = defaults.endTitle;
    }
    if (config.timerDefault != 0 && config.timerDefault != 30 && config.timerDefault != 60 && config.timerDefault != 99) {
        config.timerDefault = defaults.timerDefault;
    }
    config.depthMin = std::clamp(config.depthMin, -160.0f, 0.0f);
    config.depthMax = std::clamp(config.depthMax, 0.0f, 160.0f);
    if (config.depthMin >= config.depthMax) {
        config.depthMin = defaults.depthMin;
        config.depthMax = defaults.depthMax;
    }
    config.depthProjectionScale = std::clamp(config.depthProjectionScale, 0.0f, 2.0f);
    config.depthMoveSpeed = std::clamp(config.depthMoveSpeed, 0.25f, 8.0f);
    config.depthSidestepDistance = std::clamp(config.depthSidestepDistance, 1.0f, 80.0f);
    config.depthSidestepFrames = std::clamp(config.depthSidestepFrames, 1, 40);
    config.depthModifierDoubleTapFrames = std::clamp(config.depthModifierDoubleTapFrames, 6, 30);
    config.fighterDepthHitTolerance = std::clamp(config.fighterDepthHitTolerance, 0.0f, 80.0f);
    config.projectileDepthHitTolerance = std::clamp(config.projectileDepthHitTolerance, 0.0f, 80.0f);
    return config;
}

ArenaConfig loadArenaConfig(const std::filesystem::path& gameRoot) {
    ArenaConfig config;
    const std::string text = readTextFile(gameRoot.parent_path() / "dlc" / "arena" / "arena_config.json");
    if (text.empty()) {
        return sanitizeArenaConfig(config);
    }

    config.modeName = findJsonString(text, "modeName", config.modeName);
    config.description = findJsonString(text, "description", config.description);
    config.defaultStage = findJsonString(text, "defaultStage", config.defaultStage.generic_string());
    config.cpuCountDefault = findJsonInt(text, "cpuCountDefault", config.cpuCountDefault);
    config.cpuCountMin = findJsonInt(text, "cpuCountMin", config.cpuCountMin);
    config.cpuCountMax = findJsonInt(text, "cpuCountMax", config.cpuCountMax);
    config.playerSelect = findJsonBool(text, "playerSelect", config.playerSelect);
    config.cpuSelect = findJsonString(text, "cpuSelect", config.cpuSelect);
    config.ruleType = findJsonString(text, "type", config.ruleType);
    config.winCondition = findJsonString(text, "winCondition", config.winCondition);
    config.allowTeams = findJsonBool(text, "allowTeams", config.allowTeams);
    config.winTitle = findJsonString(text, "winTitle", config.winTitle);
    config.endTitle = findJsonString(text, "endTitle", config.endTitle);
    config.timerDefault = findJsonInt(text, "timerDefault", config.timerDefault);
    config.zAxisDefault = findJsonBool(text, "zAxisDefault", config.zAxisDefault);
    config.depthMin = findJsonFloat(text, "depthMin", config.depthMin);
    config.depthMax = findJsonFloat(text, "depthMax", config.depthMax);
    config.depthProjectionScale = findJsonFloat(text, "depthProjectionScale", config.depthProjectionScale);
    config.depthMoveSpeed = findJsonFloat(text, "depthMoveSpeed", config.depthMoveSpeed);
    config.depthSidestepDistance = findJsonFloat(text, "depthSidestepDistance", config.depthSidestepDistance);
    config.depthSidestepFrames = findJsonInt(text, "depthSidestepFrames", config.depthSidestepFrames);
    config.depthModifierDoubleTapFrames = findJsonInt(text, "depthModifierDoubleTapFrames", config.depthModifierDoubleTapFrames);
    config.fighterDepthHitTolerance = findJsonFloat(text, "fighterDepthHitTolerance", config.fighterDepthHitTolerance);
    config.projectileDepthHitTolerance = findJsonFloat(text, "projectileDepthHitTolerance", config.projectileDepthHitTolerance);
    return sanitizeArenaConfig(config);
}

} // namespace dragon
