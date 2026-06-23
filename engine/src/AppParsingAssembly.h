#pragma once

// Internal App.cpp implementation shard.
// AIR, numeric, sound, collision, and animation parsing helpers.

struct AirElement {
    int group = 0;
    int image = 0;
    int offsetX = 0;
    int offsetY = 0;
    int duration = 1;
    bool infiniteDuration = false;
    bool flipX = false;
    bool flipY = false;
    bool additive = false;
    std::vector<CollisionBox> clsn1;
    std::vector<CollisionBox> clsn2;
};

struct AirActionData {
    std::vector<AirElement> elements;
    size_t loopStart = 0;
    bool hasLoopStart = false;
};

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

std::optional<float> parsePlainFloatValue(const std::string& value) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    size_t consumed = 0;
    try {
        const float parsed = std::stof(trimmed, &consumed);
        if (consumed == trimmed.size()) {
            return parsed;
        }
    } catch (...) {
    }
    return std::nullopt;
}

std::vector<std::string> splitCsv(const std::string& line);

int parseIntValue(const std::string& value, int fallback = 0) {
    return static_cast<int>(parseFloatValue(value, static_cast<float>(fallback)));
}

std::optional<int> parsePlainIntValue(const std::string& value) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    size_t consumed = 0;
    try {
        const int parsed = std::stoi(trimmed, &consumed, 10);
        if (consumed == trimmed.size()) {
            return parsed;
        }
    } catch (...) {
    }
    return std::nullopt;
}

std::pair<float, float> parseFloatPairValue(const std::string& value, float fallbackX = 0.0f, float fallbackY = 0.0f) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackX, fallbackY };
    }
    const float x = parseFloatValue(parts[0], fallbackX);
    const float y = parts.size() >= 2 ? parseFloatValue(parts[1], fallbackY) : fallbackY;
    return { x, y };
}

std::pair<int, int> parseIntPairValue(const std::string& value, int fallbackX = 0, int fallbackY = 0) {
    const auto pair = parseFloatPairValue(value, static_cast<float>(fallbackX), static_cast<float>(fallbackY));
    return { static_cast<int>(pair.first), static_cast<int>(pair.second) };
}

std::pair<std::string, std::string> parseExpressionPairValue(
    const std::string& value,
    std::string fallbackX = {},
    std::string fallbackY = {}) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { std::move(fallbackX), std::move(fallbackY) };
    }
    std::string x = trim(parts[0]);
    std::string y = parts.size() >= 2 ? trim(parts[1]) : fallbackY;
    if (x.empty()) {
        x = std::move(fallbackX);
    }
    return { std::move(x), std::move(y) };
}

std::optional<ParsedSoundValue> parseSoundValue(const std::string& value) {
    const auto parts = splitCsv(value);
    if (parts.size() < 2) {
        return std::nullopt;
    }

    std::string groupPart = trim(parts[0]);
    ParsedSoundValue sound;
    if (!groupPart.empty() && (groupPart.front() == 'F' || groupPart.front() == 'f')) {
        sound.forceCommon = true;
        groupPart = trim(std::string_view(groupPart).substr(1));
    } else if (!groupPart.empty() && (groupPart.front() == 'S' || groupPart.front() == 's')) {
        groupPart = trim(std::string_view(groupPart).substr(1));
    }
    sound.groupExpression = groupPart;
    sound.indexExpression = trim(parts[1]);

    const auto group = parsePlainIntValue(groupPart);
    const auto index = parsePlainIntValue(parts[1]);
    if (!group || !index) {
        if (!sound.groupExpression.empty() && !sound.indexExpression.empty()) {
            return sound;
        }
        return std::nullopt;
    }

    sound.group = *group;
    sound.index = *index;
    return sound;
}

float mugenVolumeToGain(const std::string& value) {
    const int volume = parseIntValue(value, 0);
    return std::clamp(std::pow(10.0f, static_cast<float>(volume) / 100.0f), 0.0f, 4.0f);
}

std::optional<float> lookupCharacterConstant(const CharacterConstants& constants, std::string_view name) {
    const std::string key = lowercaseCopy(trim(name));
    if (key == "velocity.walk.fwd.x") {
        return constants.velocityWalkFwdX;
    }
    if (key == "velocity.walk.back.x") {
        return constants.velocityWalkBackX;
    }
    if (key == "velocity.run.fwd.x") {
        return constants.velocityRunFwdX;
    }
    if (key == "velocity.run.fwd.y") {
        return constants.velocityRunFwdY;
    }
    if (key == "velocity.run.back.x") {
        return constants.velocityRunBackX;
    }
    if (key == "velocity.run.back.y") {
        return constants.velocityRunBackY;
    }
    if (key == "velocity.jump.neu.x") {
        return constants.velocityJumpNeuX;
    }
    if (key == "velocity.jump.y") {
        return constants.velocityJumpY;
    }
    if (key == "velocity.jump.fwd.x") {
        return constants.velocityJumpFwdX;
    }
    if (key == "velocity.jump.back.x") {
        return constants.velocityJumpBackX;
    }
    if (key == "velocity.runjump.fwd.x") {
        return constants.velocityRunJumpFwdX;
    }
    if (key == "velocity.runjump.fwd.y") {
        return constants.velocityRunJumpFwdY;
    }
    if (key == "velocity.runjump.back.x") {
        return constants.velocityRunJumpBackX;
    }
    if (key == "velocity.runjump.back.y") {
        return constants.velocityRunJumpBackY;
    }
    if (key == "velocity.runjump.y") {
        return constants.velocityRunJumpFwdY;
    }
    if (key == "velocity.airjump.neu.x") {
        return constants.velocityAirJumpNeuX;
    }
    if (key == "velocity.airjump.y") {
        return constants.velocityAirJumpY;
    }
    if (key == "velocity.airjump.fwd.x") {
        return constants.velocityAirJumpFwdX;
    }
    if (key == "velocity.airjump.back.x") {
        return constants.velocityAirJumpBackX;
    }
    if (key == "movement.airjump.num") {
        return static_cast<float>(constants.movementAirJumpNum);
    }
    if (key == "movement.airjump.height") {
        return static_cast<float>(constants.movementAirJumpHeight);
    }
    if (key == "movement.stand.friction") {
        return constants.movementStandFriction;
    }
    if (key == "movement.crouch.friction") {
        return constants.movementCrouchFriction;
    }
    if (key == "movement.stand.friction.threshold") {
        return constants.movementStandFrictionThreshold;
    }
    if (key == "movement.crouch.friction.threshold") {
        return constants.movementCrouchFrictionThreshold;
    }
    if (key == "movement.yaccel") {
        return constants.movementYAccel;
    }
    if (key == "movement.down.bounce.offset.x") {
        return constants.movementDownBounceOffsetX;
    }
    if (key == "movement.down.bounce.offset.y") {
        return constants.movementDownBounceOffsetY;
    }
    if (key == "movement.down.bounce.yaccel") {
        return constants.movementDownBounceYAccel;
    }
    if (key == "movement.down.bounce.groundlevel") {
        return constants.movementDownBounceGroundLevel;
    }
    if (key == "movement.down.friction.threshold") {
        return constants.movementDownFrictionThreshold;
    }
    if (key == "data.power") {
        return static_cast<float>(constants.maxPower);
    }
    if (key == "data.life") {
        return static_cast<float>(constants.life);
    }
    if (key == "data.attack") {
        return static_cast<float>(constants.attack);
    }
    if (key == "data.defence") {
        return static_cast<float>(constants.defence);
    }
    if (key == "data.fall.defence_up") {
        return static_cast<float>(constants.fallDefenceUp);
    }
    if (key == "data.liedown.time") {
        return static_cast<float>(constants.liedownTime);
    }
    if (key == "size.ground.back") {
        return constants.size.groundBack;
    }
    if (key == "size.ground.front") {
        return constants.size.groundFront;
    }
    if (key == "size.air.back") {
        return constants.size.airBack;
    }
    if (key == "size.air.front") {
        return constants.size.airFront;
    }
    if (key == "size.height") {
        return constants.size.height;
    }
    if (key == "size.xscale") {
        return constants.sizeScaleX;
    }
    if (key == "size.yscale") {
        return constants.sizeScaleY;
    }
    return std::nullopt;
}

std::optional<float> parseControllerFloatValue(const std::string& value, const CharacterConstants& constants) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    if (const auto plain = parsePlainFloatValue(trimmed)) {
        return plain;
    }

    const std::string lowered = lowercaseCopy(trimmed);
    if (startsWithNoCase(lowered, "const(") && trimmed.back() == ')') {
        const std::string key = trim(std::string_view(trimmed).substr(6, trimmed.size() - 7));
        return lookupCharacterConstant(constants, key);
    }
    return std::nullopt;
}

std::optional<std::pair<float, float>> parseControllerFloatPairValue(
    const std::string& value,
    const CharacterConstants& constants,
    float fallbackY = 0.0f) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return std::nullopt;
    }

    const auto x = parseControllerFloatValue(parts[0], constants);
    if (!x) {
        return std::nullopt;
    }
    const auto y = parts.size() >= 2 ? parseControllerFloatValue(parts[1], constants) : std::optional<float>{ fallbackY };
    if (!y) {
        return std::nullopt;
    }
    return std::pair<float, float>{ *x, *y };
}

std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> parts;
    size_t start = 0;
    int parenDepth = 0;
    int bracketDepth = 0;
    bool inQuote = false;
    for (size_t i = 0; i <= line.size(); ++i) {
        const bool atEnd = i == line.size();
        const char ch = atEnd ? ',' : line[i];
        if (!atEnd && ch == '"') {
            inQuote = !inQuote;
        } else if (!inQuote && ch == '(') {
            ++parenDepth;
        } else if (!inQuote && ch == ')' && parenDepth > 0) {
            --parenDepth;
        } else if (!inQuote && ch == '[') {
            ++bracketDepth;
        } else if (!inQuote && ch == ']' && bracketDepth > 0) {
            --bracketDepth;
        }

        if (atEnd || (!inQuote && ch == ',' && parenDepth == 0 && bracketDepth == 0)) {
            parts.push_back(trim(std::string_view(line).substr(start, i - start)));
            start = i + 1;
        }
    }
    return parts;
}

bool hasFlagNoCase(std::string_view value, char flag) {
    const char wanted = static_cast<char>(SDL_tolower(static_cast<unsigned char>(flag)));
    for (const char ch : value) {
        const char current = static_cast<char>(SDL_tolower(static_cast<unsigned char>(ch)));
        if (current == wanted) {
            return true;
        }
    }
    return false;
}

enum class CollisionKind {
    None,
    Clsn1,
    Clsn2,
};

struct CollisionDirective {
    CollisionKind kind = CollisionKind::None;
    bool isDefault = false;
};

std::optional<CollisionDirective> parseCollisionDirective(const std::string& line) {
    const auto colon = line.find(':');
    if (colon == std::string::npos) {
        return std::nullopt;
    }

    const auto name = trim(std::string_view(line).substr(0, colon));
    if (startsWithNoCase(name, "Clsn1Default")) {
        return CollisionDirective{ CollisionKind::Clsn1, true };
    }
    if (startsWithNoCase(name, "Clsn2Default")) {
        return CollisionDirective{ CollisionKind::Clsn2, true };
    }
    if (startsWithNoCase(name, "Clsn1")) {
        return CollisionDirective{ CollisionKind::Clsn1, false };
    }
    if (startsWithNoCase(name, "Clsn2")) {
        return CollisionDirective{ CollisionKind::Clsn2, false };
    }
    return std::nullopt;
}

std::optional<CollisionBox> parseCollisionBox(const std::string& line) {
    if (!startsWithNoCase(line, "Clsn1[") && !startsWithNoCase(line, "Clsn2[")) {
        return std::nullopt;
    }

    const auto equals = line.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }

    const auto parts = splitCsv(trim(std::string_view(line).substr(equals + 1)));
    if (parts.size() < 4) {
        return std::nullopt;
    }

    try {
        const float x1 = std::stof(parts[0]);
        const float y1 = std::stof(parts[1]);
        const float x2 = std::stof(parts[2]);
        const float y2 = std::stof(parts[3]);
        return CollisionBox{
            std::min(x1, x2),
            std::min(y1, y2),
            std::max(x1, x2),
            std::max(y1, y2),
        };
    } catch (...) {
        return std::nullopt;
    }
}

AirActionData loadAirActionData(const MugenDocument& doc, int actionNo) {
    const auto wanted = std::string("Begin Action ") + std::to_string(actionNo);
    const MugenSection* action = nullptr;
    for (const auto& section : doc.sections) {
        if (startsWithNoCase(section.name, wanted)) {
            action = &section;
            break;
        }
    }
    if (!action) {
        return {};
    }

    AirActionData data;
    std::vector<CollisionBox> defaultClsn1;
    std::vector<CollisionBox> defaultClsn2;
    std::optional<std::vector<CollisionBox>> pendingClsn1;
    std::optional<std::vector<CollisionBox>> pendingClsn2;
    CollisionDirective activeCollision;

    for (const auto& line : action->body) {
        if (startsWithNoCase(line, "Loopstart")) {
            data.loopStart = data.elements.size();
            data.hasLoopStart = true;
            continue;
        }

        if (const auto directive = parseCollisionDirective(line)) {
            activeCollision = *directive;
            if (activeCollision.kind == CollisionKind::Clsn1) {
                if (activeCollision.isDefault) {
                    defaultClsn1.clear();
                } else {
                    pendingClsn1 = std::vector<CollisionBox>{};
                }
            } else if (activeCollision.kind == CollisionKind::Clsn2) {
                if (activeCollision.isDefault) {
                    defaultClsn2.clear();
                } else {
                    pendingClsn2 = std::vector<CollisionBox>{};
                }
            }
            continue;
        }

        if (const auto box = parseCollisionBox(line)) {
            if (activeCollision.kind == CollisionKind::Clsn1) {
                if (activeCollision.isDefault) {
                    defaultClsn1.push_back(*box);
                } else {
                    if (!pendingClsn1) {
                        pendingClsn1 = std::vector<CollisionBox>{};
                    }
                    pendingClsn1->push_back(*box);
                }
            } else if (activeCollision.kind == CollisionKind::Clsn2) {
                if (activeCollision.isDefault) {
                    defaultClsn2.push_back(*box);
                } else {
                    if (!pendingClsn2) {
                        pendingClsn2 = std::vector<CollisionBox>{};
                    }
                    pendingClsn2->push_back(*box);
                }
            }
            continue;
        }

        if (line.find('=') != std::string::npos || startsWithNoCase(line, "Clsn")) {
            continue;
        }

        const auto parts = splitCsv(line);
        if (parts.size() < 5) {
            continue;
        }

        try {
            AirElement element;
            element.group = std::stoi(parts[0]);
            element.image = std::stoi(parts[1]);
            element.offsetX = std::stoi(parts[2]);
            element.offsetY = std::stoi(parts[3]);
            const int rawDuration = std::stoi(parts[4]);
            element.infiniteDuration = rawDuration < 0;
            element.duration = element.infiniteDuration ? 1 : std::max(1, rawDuration);
            if (parts.size() >= 6) {
                element.flipX = hasFlagNoCase(parts[5], 'H');
                element.flipY = hasFlagNoCase(parts[5], 'V');
            }
            if (parts.size() >= 7) {
                element.additive = hasFlagNoCase(parts[6], 'A');
            }
            element.clsn1 = pendingClsn1 ? *pendingClsn1 : defaultClsn1;
            element.clsn2 = pendingClsn2 ? *pendingClsn2 : defaultClsn2;
            pendingClsn1.reset();
            pendingClsn2.reset();
            data.elements.push_back(element);
        } catch (...) {
            continue;
        }
    }
    data.loopStart = std::min(data.loopStart, data.elements.size());
    return data;
}

AnimationClip loadSffClip(
    SDL_Renderer* renderer,
    const SffArchive& sff,
    const MugenDocument& air,
    int actionNo,
    DecodeOptions options) {
    AnimationClip clip;
    clip.action = actionNo;
    clip.loopTicks = 0;

    const auto action = loadAirActionData(air, actionNo);
    clip.hasLoopStart = action.hasLoopStart;
    int tick = 0;
    for (size_t i = 0; i < action.elements.size(); ++i) {
        const auto& element = action.elements[i];
        if (i == action.loopStart) {
            clip.loopStartTick = tick;
        }
        if (element.group < 0 || element.image < 0) {
            tick += element.duration;
            continue;
        }
        const auto* sprite = findSprite(sff, element.group, element.image);
        if (!sprite) {
            tick += element.duration;
            continue;
        }
        DecodeOptions frameOptions = options;
        if (frameOptions.fallbackPalette && sprite->sharedPalette) {
            frameOptions.preferFallbackPalette = true;
            frameOptions.reverseFallbackPalette = true;
        } else {
            frameOptions.reverseFallbackPalette = false;
        }
        const auto decoded = decodeSffSprite(sff, *sprite, frameOptions);
        if (!decoded) {
            tick += element.duration;
            continue;
        }
        AnimationFrame frame;
        frame.sprite = makeTextureSprite(renderer, *decoded);
        frame.offsetX = element.offsetX;
        frame.offsetY = element.offsetY;
        frame.duration = element.duration;
        frame.infiniteDuration = element.infiniteDuration;
        frame.flipX = element.flipX;
        frame.flipY = element.flipY;
        frame.additive = element.additive;
        frame.clsn1 = element.clsn1;
        frame.clsn2 = element.clsn2;
        if (frame.infiniteDuration && !clip.hasInfiniteDuration) {
            clip.hasInfiniteDuration = true;
            clip.infiniteStartTick = tick;
        }
        clip.loopTicks += frame.duration;
        clip.frames.push_back(frame);
        tick += element.duration;
    }
    clip.loopTicks = std::max(1, clip.loopTicks);
    if (action.loopStart >= action.elements.size()) {
        clip.loopStartTick = 0;
    }
    return clip;
}

AnimationClip loadCharacterClip(
    SDL_Renderer* renderer,
    const SffArchive& sff,
    const Palette* palette,
    const MugenDocument& air,
    int actionNo) {
    DecodeOptions options;
    if (palette) {
        options.fallbackPalette = palette;
        options.preferFallbackPalette = false;
        options.reverseFallbackPalette = true;
    }
    options.transparentColorZero = true;
    return loadSffClip(renderer, sff, air, actionNo, options);
}

std::vector<int> collectAirActionNumbers(const MugenDocument& air);

std::vector<AnimationClip> loadCharacterClips(SDL_Renderer* renderer, const CharacterFiles& files) {
    std::optional<Palette> palette;
    if (!files.palette.empty() && std::filesystem::exists(files.palette)) {
        palette = loadActPalette(files.palette);
    }
    const auto sff = loadSffArchive(files.sprite);
    const auto air = parseMugenTextFile(files.anim);
    const auto actions = collectAirActionNumbers(air);

    std::vector<AnimationClip> clips;
    clips.reserve(actions.size());
    for (const int action : actions) {
        auto clip = loadCharacterClip(renderer, sff, palette ? &*palette : nullptr, air, action);
        if (!clip.frames.empty()) {
            clips.push_back(std::move(clip));
        }
    }
    return clips;
}

std::optional<int> parseAirActionNumber(std::string_view sectionName) {
    constexpr std::string_view prefix = "Begin Action ";
    if (!startsWithNoCase(sectionName, prefix)) {
        return std::nullopt;
    }
    try {
        return std::stoi(trim(sectionName.substr(prefix.size())));
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<int> collectAirActionNumbers(const MugenDocument& air) {
    std::vector<int> actions;
    for (const auto& section : air.sections) {
        if (const auto action = parseAirActionNumber(section.name)) {
            if (std::find(actions.begin(), actions.end(), *action) == actions.end()) {
                actions.push_back(*action);
            }
        }
    }
    std::sort(actions.begin(), actions.end());
    return actions;
}

std::vector<AnimationClip> loadFightFxClips(SDL_Renderer* renderer, const std::filesystem::path& gameRoot) {
    const auto dataRoot = gameRoot / "data";
    const auto sff = loadSffArchive(dataRoot / "fightfx.sff");
    const auto air = parseMugenTextFile(dataRoot / "fightfx.air");

    std::vector<AnimationClip> clips;
    const auto actions = collectAirActionNumbers(air);
    clips.reserve(actions.size());
    DecodeOptions options;
    options.transparentColorZero = true;
    for (const int action : actions) {
        auto clip = loadSffClip(renderer, sff, air, action, options);
        if (!clip.frames.empty()) {
            clips.push_back(std::move(clip));
        }
    }
    return clips;
}

#include "StateControllerParsing.h"
