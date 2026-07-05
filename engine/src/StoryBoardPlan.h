#pragma once

#include "dragon/MugenText.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
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

struct StoryBoardNode {
    std::string id;
    std::string title;
    std::string stageRef;
    std::string enemyRef;
    std::string shopRef;
    std::string shopDoorPrompt = "LK / X SHOP";
    std::string shopDoorEnterText = "ENTERING SHOP";
    std::string shopDoorOpenText = "SHOP DOOR OPEN";
    std::string waveClearText;
    StoryBoardNodeKind kind = StoryBoardNodeKind::SideScroller;
    int waves = 3;
    bool optional = false;
    float shopDoorOffsetX = 160.0f;
    float shopDoorRadiusX = 56.0f;
    float shopDoorRadiusZ = 44.0f;
};

struct StoryBoardRoute {
    std::string id = "default";
    std::string title = "STORY ROUTE";
    std::string forwardCueImagePath = "data/story/wave_clear_arrow.png";
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
        if (!storyBoardStartsWithNoCase(section.name, "Board")) {
            continue;
        }

        StoryBoardNode node;
        node.id = storyBoardPropertyValue(section, "id", "board_" + std::to_string(route.nodes.size() + 1));
        node.title = storyBoardPropertyValue(section, "title", node.id);
        node.stageRef = storyBoardPropertyValue(section, "stage");
        node.enemyRef = storyBoardPropertyValue(section, "enemy");
        node.shopRef = storyBoardPropertyValue(section, "shop");
        node.shopDoorPrompt = storyBoardPropertyValue(section, "shop_prompt", node.shopDoorPrompt);
        node.shopDoorEnterText = storyBoardPropertyValue(section, "shop_enter_text", node.shopDoorEnterText);
        node.shopDoorOpenText = storyBoardPropertyValue(section, "shop_open_text", node.shopDoorOpenText);
        node.waveClearText = storyBoardPropertyValue(section, "wave_clear_text", node.waveClearText);
        node.kind = storyBoardNodeKindFromText(storyBoardPropertyValue(section, "type", "side_scroller"));
        node.waves = std::clamp(storyBoardIntValue(storyBoardPropertyValue(section, "waves", "3"), 3), 1, 8);
        node.optional = storyBoardBoolValue(storyBoardPropertyValue(section, "optional", "0"), false);
        node.shopDoorOffsetX = std::max(0.0f, storyBoardFloatValue(storyBoardPropertyValue(section, "shop_door_x_offset", "160"), 160.0f));
        node.shopDoorRadiusX = std::max(8.0f, storyBoardFloatValue(storyBoardPropertyValue(section, "shop_door_radius_x", "56"), 56.0f));
        node.shopDoorRadiusZ = std::max(0.0f, storyBoardFloatValue(storyBoardPropertyValue(section, "shop_door_radius_z", "44"), 44.0f));
        if (node.kind == StoryBoardNodeKind::Shop && node.waves <= 0) {
            node.waves = 1;
        }
        route.nodes.push_back(std::move(node));
    }

    return route;
}

} // namespace dragon
