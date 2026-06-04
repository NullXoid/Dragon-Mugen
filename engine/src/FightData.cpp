#include "dragon/FightData.h"

#include "dragon/MugenText.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

namespace dragon {
namespace {

std::string unquote(std::string value) {
    value = trim(value);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::optional<std::pair<float, float>> parsePair(const std::string& value) {
    const auto comma = value.find(',');
    if (comma == std::string::npos) {
        return std::nullopt;
    }

    try {
        return std::pair<float, float>{
            std::stof(trim(std::string_view(value).substr(0, comma))),
            std::stof(trim(std::string_view(value).substr(comma + 1))),
        };
    } catch (...) {
        return std::nullopt;
    }
}

float parseFloatValue(const std::string& value, float fallback = 0.0f) {
    try {
        return std::stof(trim(value));
    } catch (...) {
        return fallback;
    }
}

int parseIntValue(const std::string& value, int fallback = 0) {
    return static_cast<int>(parseFloatValue(value, static_cast<float>(fallback)));
}

std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= line.size()) {
        const auto comma = line.find(',', start);
        const auto end = comma == std::string::npos ? line.size() : comma;
        parts.push_back(trim(std::string_view(line).substr(start, end - start)));
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return parts;
}

float parseFloatOr(const MugenSection* section, std::string_view key, float fallback) {
    if (!section) {
        return fallback;
    }
    const auto* property = findProperty(*section, key);
    if (!property) {
        return fallback;
    }
    return parseFloatValue(property->value, fallback);
}

int parseSectionSettingInt(const MugenSection* section, std::string_view key, int fallback) {
    if (!section) {
        return fallback;
    }
    const auto* property = findProperty(*section, key);
    if (!property) {
        return fallback;
    }
    return std::max(0, parseIntValue(property->value, fallback));
}

int parseFontPaletteSetting(const MugenSection* section, std::string_view key, int fallback) {
    if (!section) {
        return fallback;
    }
    const auto* property = findProperty(*section, key);
    if (!property) {
        return fallback;
    }
    const auto parts = splitCsv(property->value);
    if (parts.size() < 2) {
        return fallback;
    }
    return std::max(0, parseIntValue(parts[1], fallback));
}

void loadFightComboSettings(const MugenDocument& doc, FightComboSettings& settings) {
    const auto* combo = findSection(doc, "Combo");
    if (!combo) {
        return;
    }

    if (const auto* pos = findProperty(*combo, "pos")) {
        const auto pair = parsePair(pos->value);
        if (pair) {
            settings.posX = pair->first;
            settings.posY = pair->second;
        }
    }
    settings.startX = parseFloatOr(combo, "start.x", settings.startX);
    settings.counterFontPalette = parseFontPaletteSetting(combo, "counter.font", settings.counterFontPalette);
    settings.counterShake = parseSectionSettingInt(combo, "counter.shake", settings.counterShake ? 1 : 0) != 0;
    if (const auto* text = findProperty(*combo, "text.text")) {
        settings.text = unquote(trim(text->value));
    }
    settings.textFontPalette = parseFontPaletteSetting(combo, "text.font", settings.textFontPalette);
    if (const auto* offset = findProperty(*combo, "text.offset")) {
        const auto pair = parsePair(offset->value);
        if (pair) {
            settings.textOffsetX = pair->first;
            settings.textOffsetY = pair->second;
        }
    }
    settings.displayTime = std::max(1, parseSectionSettingInt(combo, "displaytime", settings.displayTime));
}

void loadFightPowerbarSettings(const MugenDocument& doc, FightPowerbarSettings& settings) {
    const auto* powerbar = findSection(doc, "Powerbar");
    if (!powerbar) {
        return;
    }

    if (const auto* p1Pos = findProperty(*powerbar, "p1.pos")) {
        if (const auto pair = parsePair(p1Pos->value)) {
            settings.p1PosX = pair->first;
            settings.p1PosY = pair->second;
        }
    }
    if (const auto* p2Pos = findProperty(*powerbar, "p2.pos")) {
        if (const auto pair = parsePair(p2Pos->value)) {
            settings.p2PosX = pair->first;
            settings.p2PosY = pair->second;
        }
    }
    if (const auto* p1Range = findProperty(*powerbar, "p1.range.x")) {
        if (const auto pair = parsePair(p1Range->value)) {
            settings.p1RangeStart = pair->first;
            settings.p1RangeEnd = pair->second;
        }
    }
    if (const auto* p2Range = findProperty(*powerbar, "p2.range.x")) {
        if (const auto pair = parsePair(p2Range->value)) {
            settings.p2RangeStart = pair->first;
            settings.p2RangeEnd = pair->second;
        }
    }
}

} // namespace

bool hasFightDefinition(const std::filesystem::path& gameRoot) {
    return std::filesystem::exists(gameRoot / "data" / "fight.def");
}

const char* fightDefinitionRequirementText() {
    return "data/fight.def";
}

FightRoundSettings loadFightRoundSettings(const std::filesystem::path& gameRoot) {
    FightRoundSettings settings;
    const auto fightDef = gameRoot / "data" / "fight.def";
    if (!std::filesystem::exists(fightDef)) {
        return settings;
    }

    try {
        const auto doc = parseMugenTextFile(fightDef);
        const auto* round = findSection(doc, "Round");
        settings.matchWins = std::max(1, parseSectionSettingInt(round, "match.wins", settings.matchWins));
        settings.startWaitTime = parseSectionSettingInt(round, "start.waittime", settings.startWaitTime);
        settings.roundDisplayTime = parseSectionSettingInt(round, "round.default.displaytime", settings.roundDisplayTime);
        settings.ctrlTime = parseSectionSettingInt(round, "ctrl.time", settings.ctrlTime);
        settings.koDisplayTime = parseSectionSettingInt(round, "slow.time", settings.koDisplayTime);
        settings.dkoDisplayTime = parseSectionSettingInt(round, "DKO.displaytime", settings.dkoDisplayTime);
        settings.timeOverDisplayTime = parseSectionSettingInt(round, "TO.displaytime", settings.timeOverDisplayTime);
        settings.overWaitTime = parseSectionSettingInt(round, "over.waittime", settings.overWaitTime);
        settings.overHitTime = parseSectionSettingInt(round, "over.hittime", settings.overHitTime);
        settings.overWinTime = parseSectionSettingInt(round, "over.wintime", settings.overWinTime);
        settings.overTime = std::max(1, parseSectionSettingInt(round, "over.time", settings.overTime));
        settings.winTime = parseSectionSettingInt(round, "win.time", settings.winTime);
        loadFightComboSettings(doc, settings.combo);
        loadFightPowerbarSettings(doc, settings.powerbar);
    } catch (const std::exception& ex) {
        SDL_Log("fight.def round settings parse failed: %s", ex.what());
    }
    return settings;
}

} // namespace dragon
