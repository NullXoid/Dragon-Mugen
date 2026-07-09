#pragma once

#include "dragon/MugenText.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace dragon {

enum class StoryBoardNodeKind {
    SideScroller,
    MidBoss,
    Shop,
    ArenaBoss,
};

struct StoryBoardWaveEnemy {
    std::string enemyRef;
    float xOffset = 0.0f;
    float depthZ = 0.0f;
    bool hasXOffset = false;
    bool hasDepthZ = false;
};

struct StoryBoardWaveSpec {
    std::vector<StoryBoardWaveEnemy> enemies;
    std::string clearText;
    std::string clearCueImagePath;
    int rewardXp = 0;
    int rewardGold = 0;
};

struct StoryBoardNode {
    std::string id;
    std::string title;
    std::string stageRef;
    std::string enemyRef;
    std::string regularEnemyRef;
    std::string midBossEnemyRef;
    std::string bossEnemyRef;
    std::string shopRef;
    std::string shopDoorPrompt = "LK / X SHOP";
    std::string shopDoorEnterText = "ENTERING SHOP";
    std::string shopDoorOpenText = "SHOP DOOR OPEN";
    std::string waveClearText;
    std::string clearCueImagePath;
    std::string shopCueImagePath;
    std::vector<StoryBoardWaveSpec> waveSpecs;
    StoryBoardNodeKind kind = StoryBoardNodeKind::SideScroller;
    int waves = 3;
    int rewardXp = 0;
    int rewardGold = 0;
    bool optional = false;
    float shopDoorOffsetX = 160.0f;
    float shopDoorRadiusX = 56.0f;
    float shopDoorRadiusZ = 44.0f;
};

struct StoryEnemyRoleSetup {
    std::vector<std::string> grunts;
    std::vector<std::string> miniBosses;
    std::vector<std::string> bosses;
};

struct StoryBoardRoute {
    std::string id = "default";
    std::string title = "STORY ROUTE";
    std::string forwardCueImagePath = "data/story/wave_clear_arrow.png";
    StoryEnemyRoleSetup enemySetup;
    std::vector<StoryBoardNode> nodes;
};

inline std::string storyBoardLowercase(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

inline bool storyBoardEqualsNoCase(std::string_view lhs, std::string_view rhs) {
    return storyBoardLowercase(lhs) == storyBoardLowercase(rhs);
}

inline bool storyBoardStartsWithNoCase(std::string_view value, std::string_view prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    return storyBoardEqualsNoCase(value.substr(0, prefix.size()), prefix);
}

inline int storyBoardIntValue(std::string_view value, int fallback) {
    try {
        size_t consumed = 0;
        const int parsed = std::stoi(std::string(value), &consumed);
        return consumed > 0 ? parsed : fallback;
    } catch (const std::exception&) {
        return fallback;
    }
}

inline bool storyBoardBoolValue(std::string_view value, bool fallback) {
    const std::string normalized = storyBoardLowercase(trim(value));
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return fallback;
}

inline float storyBoardFloatValue(std::string_view value, float fallback) {
    try {
        size_t consumed = 0;
        const float parsed = std::stof(std::string(value), &consumed);
        return consumed > 0 ? parsed : fallback;
    } catch (const std::exception&) {
        return fallback;
    }
}

inline std::string storyBoardPropertyValue(const MugenSection& section, std::string_view key, std::string fallback = {}) {
    if (const MugenProperty* property = findProperty(section, key)) {
        return trim(property->value);
    }
    return fallback;
}

inline std::vector<std::string> storyBoardCsvValues(std::string_view value) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t comma = value.find(',', start);
        const size_t end = comma == std::string_view::npos ? value.size() : comma;
        const std::string part = trim(value.substr(start, end - start));
        if (!part.empty()) {
            parts.push_back(part);
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return parts;
}

inline std::vector<float> storyBoardFloatList(std::string_view value) {
    std::vector<float> out;
    for (const std::string& part : storyBoardCsvValues(value)) {
        out.push_back(storyBoardFloatValue(part, 0.0f));
    }
    return out;
}

inline std::vector<std::string> storyBoardPropertyCsv(const MugenSection& section, std::string_view key) {
    return storyBoardCsvValues(storyBoardPropertyValue(section, key));
}

inline void storyBoardAssignFirstNonEmptyList(
    std::vector<std::string>& target,
    const MugenSection& section,
    std::initializer_list<std::string_view> keys) {
    if (!target.empty()) {
        return;
    }
    for (std::string_view key : keys) {
        target = storyBoardPropertyCsv(section, key);
        if (!target.empty()) {
            return;
        }
    }
}

inline const MugenSection* storyBoardFindSectionNoCase(const MugenDocument& doc, std::string_view wanted) {
    for (const MugenSection& section : doc.sections) {
        if (storyBoardEqualsNoCase(trim(section.name), wanted)) {
            return &section;
        }
    }
    return nullptr;
}

inline StoryEnemyRoleSetup storyBoardEnemySetupFromDocument(const MugenDocument& doc) {
    StoryEnemyRoleSetup setup;
    const MugenSection* section = storyBoardFindSectionNoCase(doc, "Enemy Setup");
    if (!section) {
        section = storyBoardFindSectionNoCase(doc, "Story Enemies");
    }
    if (!section) {
        return setup;
    }

    storyBoardAssignFirstNonEmptyList(
        setup.grunts,
        *section,
        { "grunts", "grunt", "normal_enemies", "normal_enemy", "regular_enemies", "regular_enemy", "wave_enemies", "wave_enemy" });
    storyBoardAssignFirstNonEmptyList(
        setup.miniBosses,
        *section,
        { "mini_bosses", "mini_boss", "mid_bosses", "mid_boss", "midbosses", "midboss", "midboss_enemy", "mid_boss_enemy" });
    storyBoardAssignFirstNonEmptyList(
        setup.bosses,
        *section,
        { "bosses", "boss", "boss_enemy", "arena_bosses", "arena_boss" });
    return setup;
}

inline std::string storyBoardFirstRoleRef(const std::vector<std::string>& refs) {
    return refs.empty() ? std::string{} : refs.front();
}

inline std::string storyBoardResolveEnemyRoleToken(const StoryEnemyRoleSetup& setup, std::string_view token) {
    const std::string normalized = storyBoardLowercase(trim(token));
    if (normalized == "grunt" || normalized == "grunts"
        || normalized == "normal" || normalized == "normal_enemy" || normalized == "normal_enemies"
        || normalized == "regular" || normalized == "regular_enemy" || normalized == "regular_enemies"
        || normalized == "wave_enemy" || normalized == "wave_enemies") {
        return storyBoardFirstRoleRef(setup.grunts);
    }
    if (normalized == "mini_boss" || normalized == "mini_bosses"
        || normalized == "mid_boss" || normalized == "mid_bosses"
        || normalized == "midboss" || normalized == "midbosses") {
        return storyBoardFirstRoleRef(setup.miniBosses);
    }
    if (normalized == "boss" || normalized == "bosses" || normalized == "arena_boss" || normalized == "arena_bosses") {
        return storyBoardFirstRoleRef(setup.bosses);
    }
    return {};
}

inline std::string storyBoardResolveEnemyRef(const StoryEnemyRoleSetup& setup, std::string_view value) {
    if (const std::string resolved = storyBoardResolveEnemyRoleToken(setup, value); !resolved.empty()) {
        return resolved;
    }
    return trim(value);
}

inline void storyBoardApplyEnemySetupToNode(const StoryEnemyRoleSetup& setup, StoryBoardNode& node) {
    if (node.regularEnemyRef.empty()) {
        node.regularEnemyRef = storyBoardFirstRoleRef(setup.grunts);
    } else {
        node.regularEnemyRef = storyBoardResolveEnemyRef(setup, node.regularEnemyRef);
    }
    if (node.midBossEnemyRef.empty()) {
        node.midBossEnemyRef = storyBoardFirstRoleRef(setup.miniBosses);
    } else {
        node.midBossEnemyRef = storyBoardResolveEnemyRef(setup, node.midBossEnemyRef);
    }
    if (node.bossEnemyRef.empty()) {
        node.bossEnemyRef = storyBoardFirstRoleRef(setup.bosses);
    } else {
        node.bossEnemyRef = storyBoardResolveEnemyRef(setup, node.bossEnemyRef);
    }
    if (!node.enemyRef.empty()) {
        node.enemyRef = storyBoardResolveEnemyRef(setup, node.enemyRef);
    }
}

inline void storyBoardResolveWaveEnemyRefs(const StoryEnemyRoleSetup& setup, StoryBoardWaveSpec& wave) {
    for (StoryBoardWaveEnemy& enemy : wave.enemies) {
        if (!enemy.enemyRef.empty()) {
            enemy.enemyRef = storyBoardResolveEnemyRef(setup, enemy.enemyRef);
        }
    }
}

inline bool storyBoardReadFirstIntAfter(std::string_view value, size_t start, int& out) {
    while (start < value.size() && !std::isdigit(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    if (start >= value.size()) {
        return false;
    }
    size_t end = start;
    while (end < value.size() && std::isdigit(static_cast<unsigned char>(value[end]))) {
        ++end;
    }
    out = storyBoardIntValue(value.substr(start, end - start), -1);
    return out >= 0;
}

inline bool storyBoardSectionIndices(std::string_view name, int& boardIndex, int& waveIndex) {
    boardIndex = -1;
    waveIndex = -1;
    const std::string normalized = storyBoardLowercase(trim(name));
    if (!storyBoardStartsWithNoCase(normalized, "board")) {
        return false;
    }
    const size_t wavePos = normalized.find("wave");
    if (!storyBoardReadFirstIntAfter(normalized, 5, boardIndex)) {
        return false;
    }
    if (wavePos != std::string::npos) {
        return storyBoardReadFirstIntAfter(normalized, wavePos + 4, waveIndex);
    }
    return true;
}

inline StoryBoardWaveSpec storyBoardWaveSpecFromSection(const MugenSection& section) {
    StoryBoardWaveSpec wave;
    wave.clearText = storyBoardPropertyValue(section, "clear_text");
    wave.clearText = storyBoardPropertyValue(section, "wave_clear_text", wave.clearText);
    wave.clearCueImagePath = storyBoardPropertyValue(section, "cue_image");
    wave.clearCueImagePath = storyBoardPropertyValue(section, "clear_cue_image", wave.clearCueImagePath);
    wave.rewardXp = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "reward_xp", "0"), 0));
    wave.rewardXp = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "xp", std::to_string(wave.rewardXp)), wave.rewardXp));
    wave.rewardGold = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "reward_gold", "0"), 0));
    wave.rewardGold = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "gold", std::to_string(wave.rewardGold)), wave.rewardGold));

    const auto enemies = storyBoardCsvValues(storyBoardPropertyValue(section, "enemies"));
    const auto spawnX = storyBoardFloatList(storyBoardPropertyValue(section, "spawn_x"));
    const auto spawnZ = storyBoardFloatList(storyBoardPropertyValue(section, "spawn_z"));
    const size_t count = std::max({ enemies.size(), spawnX.size(), spawnZ.size() });
    wave.enemies.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        StoryBoardWaveEnemy enemy;
        if (i < enemies.size()) {
            enemy.enemyRef = enemies[i];
        }
        if (i < spawnX.size()) {
            enemy.xOffset = spawnX[i];
            enemy.hasXOffset = true;
        }
        if (i < spawnZ.size()) {
            enemy.depthZ = spawnZ[i];
            enemy.hasDepthZ = true;
        }
        wave.enemies.push_back(std::move(enemy));
    }
    return wave;
}

inline StoryBoardNodeKind storyBoardNodeKindFromText(std::string_view value) {
    const std::string normalized = storyBoardLowercase(trim(value));
    if (normalized == "mid_boss" || normalized == "midboss" || normalized == "mini_boss") {
        return StoryBoardNodeKind::MidBoss;
    }
    if (normalized == "shop" || normalized == "shop_door" || normalized == "door") {
        return StoryBoardNodeKind::Shop;
    }
    if (normalized == "arena_boss" || normalized == "boss" || normalized == "arena") {
        return StoryBoardNodeKind::ArenaBoss;
    }
    return StoryBoardNodeKind::SideScroller;
}

inline std::string storyBoardNodeKindTag(StoryBoardNodeKind kind) {
    switch (kind) {
    case StoryBoardNodeKind::MidBoss:
        return "mid_boss";
    case StoryBoardNodeKind::Shop:
        return "shop";
    case StoryBoardNodeKind::ArenaBoss:
        return "arena_boss";
    case StoryBoardNodeKind::SideScroller:
    default:
        return "side_scroller";
    }
}

inline std::string storyBoardNodeKindLabel(StoryBoardNodeKind kind) {
    switch (kind) {
    case StoryBoardNodeKind::MidBoss:
        return "MID BOSS";
    case StoryBoardNodeKind::Shop:
        return "SHOP DOOR";
    case StoryBoardNodeKind::ArenaBoss:
        return "BOSS ARENA";
    case StoryBoardNodeKind::SideScroller:
    default:
        return "SIDE SCROLL";
    }
}

inline StoryBoardRoute loadStoryBoardRouteFile(const std::filesystem::path& path) {
    const MugenDocument doc = parseMugenTextFile(path);

    StoryBoardRoute route;
    route.enemySetup = storyBoardEnemySetupFromDocument(doc);
    if (const MugenSection* routeSection = findSection(doc, "Route")) {
        route.id = storyBoardPropertyValue(*routeSection, "id", route.id);
        route.title = storyBoardPropertyValue(*routeSection, "title", route.title);
        route.forwardCueImagePath = storyBoardPropertyValue(*routeSection, "forward_cue_image", route.forwardCueImagePath);
        route.forwardCueImagePath = storyBoardPropertyValue(*routeSection, "clear_cue_image", route.forwardCueImagePath);
    } else if (const MugenSection* routeDefault = findSection(doc, "Route Default")) {
        route.id = storyBoardPropertyValue(*routeDefault, "id", route.id);
        route.title = storyBoardPropertyValue(*routeDefault, "title", route.title);
        route.forwardCueImagePath = storyBoardPropertyValue(*routeDefault, "forward_cue_image", route.forwardCueImagePath);
        route.forwardCueImagePath = storyBoardPropertyValue(*routeDefault, "clear_cue_image", route.forwardCueImagePath);
    }

    for (const MugenSection& section : doc.sections) {
        int boardIndex = -1;
        int waveIndex = -1;
        if (!storyBoardSectionIndices(section.name, boardIndex, waveIndex) || waveIndex >= 0) {
            continue;
        }

        StoryBoardNode node;
        node.id = storyBoardPropertyValue(section, "id", "board_" + std::to_string(route.nodes.size() + 1));
        node.title = storyBoardPropertyValue(section, "title", node.id);
        node.stageRef = storyBoardPropertyValue(section, "stage");
        node.enemyRef = storyBoardPropertyValue(section, "enemy");
        node.regularEnemyRef = storyBoardPropertyValue(section, "regular_enemy");
        node.regularEnemyRef = storyBoardPropertyValue(section, "normal_enemy", node.regularEnemyRef);
        node.regularEnemyRef = storyBoardPropertyValue(section, "wave_enemy", node.regularEnemyRef);
        node.midBossEnemyRef = storyBoardPropertyValue(section, "midboss_enemy");
        node.midBossEnemyRef = storyBoardPropertyValue(section, "mid_boss_enemy", node.midBossEnemyRef);
        node.midBossEnemyRef = storyBoardPropertyValue(section, "mini_boss_enemy", node.midBossEnemyRef);
        node.bossEnemyRef = storyBoardPropertyValue(section, "boss_enemy");
        node.shopRef = storyBoardPropertyValue(section, "shop");
        node.shopDoorPrompt = storyBoardPropertyValue(section, "shop_prompt", node.shopDoorPrompt);
        node.shopDoorEnterText = storyBoardPropertyValue(section, "shop_enter_text", node.shopDoorEnterText);
        node.shopDoorOpenText = storyBoardPropertyValue(section, "shop_open_text", node.shopDoorOpenText);
        node.waveClearText = storyBoardPropertyValue(section, "wave_clear_text", node.waveClearText);
        node.clearCueImagePath = storyBoardPropertyValue(section, "clear_cue_image", node.clearCueImagePath);
        node.clearCueImagePath = storyBoardPropertyValue(section, "cue_image", node.clearCueImagePath);
        node.shopCueImagePath = storyBoardPropertyValue(section, "shop_cue_image", node.shopCueImagePath);
        node.kind = storyBoardNodeKindFromText(storyBoardPropertyValue(section, "type", "side_scroller"));
        node.waves = std::clamp(storyBoardIntValue(storyBoardPropertyValue(section, "waves", "3"), 3), 1, 8);
        node.rewardXp = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "reward_xp", "0"), 0));
        node.rewardXp = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "board_reward_xp", std::to_string(node.rewardXp)), node.rewardXp));
        node.rewardGold = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "reward_gold", "0"), 0));
        node.rewardGold = std::max(0, storyBoardIntValue(storyBoardPropertyValue(section, "board_reward_gold", std::to_string(node.rewardGold)), node.rewardGold));
        node.optional = storyBoardBoolValue(storyBoardPropertyValue(section, "optional", "0"), false);
        node.shopDoorOffsetX = std::max(0.0f, storyBoardFloatValue(storyBoardPropertyValue(section, "shop_door_x_offset", "160"), 160.0f));
        node.shopDoorRadiusX = std::max(8.0f, storyBoardFloatValue(storyBoardPropertyValue(section, "shop_door_radius_x", "56"), 56.0f));
        node.shopDoorRadiusZ = std::max(0.0f, storyBoardFloatValue(storyBoardPropertyValue(section, "shop_door_radius_z", "44"), 44.0f));
        storyBoardApplyEnemySetupToNode(route.enemySetup, node);
        if (node.kind == StoryBoardNodeKind::Shop && node.waves <= 0) {
            node.waves = 1;
        }
        route.nodes.push_back(std::move(node));
    }

    for (const MugenSection& section : doc.sections) {
        int boardIndex = -1;
        int waveIndex = -1;
        if (!storyBoardSectionIndices(section.name, boardIndex, waveIndex) || waveIndex < 0) {
            continue;
        }
        if (boardIndex < 0 || boardIndex >= static_cast<int>(route.nodes.size()) || waveIndex >= 8) {
            continue;
        }
        StoryBoardNode& node = route.nodes[static_cast<size_t>(boardIndex)];
        if (static_cast<int>(node.waveSpecs.size()) <= waveIndex) {
            node.waveSpecs.resize(static_cast<size_t>(waveIndex + 1));
        }
        node.waveSpecs[static_cast<size_t>(waveIndex)] = storyBoardWaveSpecFromSection(section);
        storyBoardResolveWaveEnemyRefs(route.enemySetup, node.waveSpecs[static_cast<size_t>(waveIndex)]);
        node.waves = std::max(node.waves, waveIndex + 1);
    }

    return route;
}

} // namespace dragon
