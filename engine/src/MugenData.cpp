#include "dragon/MugenData.h"

#include "dragon/MugenText.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string_view>

namespace dragon {
namespace {

std::string unquote(std::string value) {
    value = trim(value);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

bool startsWithNoCase(std::string_view value, std::string_view prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    for (size_t i = 0; i < prefix.size(); ++i) {
        const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
        const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
}

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(lhs[i])));
        const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(rhs[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
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

int parseIntValue(const std::string& value, int fallback) {
    try {
        size_t consumed = 0;
        const int parsed = std::stoi(trim(value), &consumed, 0);
        return consumed == 0 ? fallback : parsed;
    } catch (...) {
        return fallback;
    }
}

float parseFloatOr(const MugenSection* section, std::string_view key, float fallback) {
    if (!section) {
        return fallback;
    }
    const auto* property = findProperty(*section, key);
    if (!property) {
        return fallback;
    }
    try {
        size_t consumed = 0;
        const float parsed = std::stof(trim(property->value), &consumed);
        return consumed == 0 ? fallback : parsed;
    } catch (...) {
        return fallback;
    }
}

std::filesystem::path resolveContentPath(const std::filesystem::path& base, std::string value) {
    value = unquote(value);
    if (value.empty()) {
        return {};
    }
    std::filesystem::path path(value);
    if (path.is_absolute()) {
        return path;
    }
    return (base / path).lexically_normal();
}

std::filesystem::path resolveSelectCharacterDefPath(const std::filesystem::path& gameRoot, std::string value) {
    value = unquote(value);
    if (value.empty()) {
        return {};
    }

    std::filesystem::path entry(value);
    if (entry.is_absolute()) {
        return entry;
    }

    const std::string generic = entry.generic_string();
    if (startsWithNoCase(generic, "chars/")) {
        return gameRoot / entry;
    }

    const auto charsRoot = gameRoot / "chars";
    if (entry.extension() == ".def" || generic.find('/') != std::string::npos || generic.find('\\') != std::string::npos) {
        return charsRoot / entry;
    }
    return charsRoot / entry / (entry.filename().string() + ".def");
}

std::filesystem::path resolveSelectStagePath(const std::filesystem::path& gameRoot, std::string value) {
    value = unquote(value);
    if (value.empty()) {
        return {};
    }

    std::filesystem::path entry(value);
    if (entry.is_absolute()) {
        return entry;
    }

    const std::string generic = entry.generic_string();
    if (startsWithNoCase(generic, "stages/")) {
        return gameRoot / entry;
    }

    const auto rootRelative = gameRoot / entry;
    if (std::filesystem::exists(rootRelative)) {
        return rootRelative;
    }
    return gameRoot / "stages" / entry;
}

std::optional<CharacterSlot> loadCharacterSlotFromDef(const std::filesystem::path& def) {
    if (def.empty() || !std::filesystem::exists(def)) {
        return std::nullopt;
    }

    CharacterSlot slot;
    slot.folder = def.parent_path().lexically_normal();
    slot.defPath = def.lexically_normal();
    slot.id = slot.folder.filename().string();
    slot.displayName = slot.id;
    slot.author = "Unknown";

    try {
        const auto doc = parseMugenTextFile(def);
        if (const auto* info = findSection(doc, "Info")) {
            if (const auto* displayName = findProperty(*info, "displayname")) {
                slot.displayName = unquote(displayName->value);
            } else if (const auto* name = findProperty(*info, "name")) {
                slot.displayName = unquote(name->value);
            }
            if (const auto* author = findProperty(*info, "author")) {
                slot.author = unquote(author->value);
            }
        }
    } catch (const std::exception& ex) {
        SDL_Log("Character DEF parse failed %s: %s", def.string().c_str(), ex.what());
        return std::nullopt;
    }

    return slot;
}

void applySelectOptions(CharacterSlot& slot, const std::vector<std::string>& parts) {
    for (size_t i = 2; i < parts.size(); ++i) {
        const auto equals = parts[i].find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const auto key = trim(std::string_view(parts[i]).substr(0, equals));
        const auto value = unquote(trim(std::string_view(parts[i]).substr(equals + 1)));
        if (equalsNoCase(key, "order")) {
            slot.order = std::clamp(parseIntValue(value, slot.order), 1, 10);
        } else if (equalsNoCase(key, "includestage")) {
            slot.includeStage = parseIntValue(value, slot.includeStage ? 1 : 0) != 0;
        } else if (equalsNoCase(key, "music")) {
            slot.music = value;
        }
    }
}

std::optional<CharacterSlot> loadSelectCharacterEntry(const std::filesystem::path& gameRoot, const std::string& line) {
    const auto parts = splitCsv(line);
    if (parts.empty() || parts.front().empty() || equalsNoCase(parts.front(), "randomselect")) {
        return std::nullopt;
    }

    auto slot = loadCharacterSlotFromDef(resolveSelectCharacterDefPath(gameRoot, parts.front()));
    if (!slot) {
        SDL_Log("select.def character entry skipped, DEF not found: %s", parts.front().c_str());
        return std::nullopt;
    }

    if (parts.size() >= 2) {
        slot->preferredStagePath = resolveSelectStagePath(gameRoot, parts[1]).lexically_normal();
    }
    applySelectOptions(*slot, parts);
    return slot;
}

std::vector<CharacterSlot> loadCharactersFromFolders(const std::filesystem::path& gameRoot) {
    std::vector<CharacterSlot> characters;
    const auto charsRoot = gameRoot / "chars";
    if (!std::filesystem::exists(charsRoot)) {
        return characters;
    }

    for (const auto& entry : std::filesystem::directory_iterator(charsRoot)) {
        if (!entry.is_directory()) {
            continue;
        }

        const auto id = entry.path().filename().string();
        auto slot = loadCharacterSlotFromDef(entry.path() / (id + ".def"));
        if (slot) {
            characters.push_back(std::move(*slot));
        }
    }

    std::sort(characters.begin(), characters.end(), [](const CharacterSlot& lhs, const CharacterSlot& rhs) {
        return lhs.displayName < rhs.displayName;
    });
    return characters;
}

std::optional<StageSlot> loadStageSlotFromDef(const std::filesystem::path& def) {
    if (def.empty() || !std::filesystem::exists(def)) {
        return std::nullopt;
    }

    StageSlot slot;
    slot.id = def.stem().string();
    slot.displayName = slot.id;
    slot.author = "Unknown";
    slot.defPath = def.lexically_normal();

    try {
        const auto doc = parseMugenTextFile(def);
        if (const auto* info = findSection(doc, "Info")) {
            if (const auto* displayName = findProperty(*info, "displayname")) {
                slot.displayName = unquote(displayName->value);
            } else if (const auto* name = findProperty(*info, "name")) {
                slot.displayName = unquote(name->value);
            }
            if (const auto* author = findProperty(*info, "author")) {
                slot.author = unquote(author->value);
            }
        }
        const auto* camera = findSection(doc, "Camera");
        slot.cameraStartx = parseFloatOr(camera, "startx", slot.cameraStartx);
        slot.cameraStarty = parseFloatOr(camera, "starty", slot.cameraStarty);
        slot.cameraBoundleft = parseFloatOr(camera, "boundleft", slot.cameraBoundleft);
        slot.cameraBoundright = parseFloatOr(camera, "boundright", slot.cameraBoundright);
        slot.cameraBoundhigh = parseFloatOr(camera, "boundhigh", slot.cameraBoundhigh);
        slot.cameraBoundlow = parseFloatOr(camera, "boundlow", slot.cameraBoundlow);
        slot.cameraVerticalfollow = parseFloatOr(camera, "verticalfollow", slot.cameraVerticalfollow);
        slot.cameraFloortension = parseFloatOr(camera, "floortension", slot.cameraFloortension);
        slot.cameraTension = parseFloatOr(camera, "tension", slot.cameraTension);

        const auto* playerInfo = findSection(doc, "PlayerInfo");
        slot.p1startx = parseFloatOr(playerInfo, "p1startx", slot.p1startx);
        slot.p1starty = parseFloatOr(playerInfo, "p1starty", slot.p1starty);
        slot.p2startx = parseFloatOr(playerInfo, "p2startx", slot.p2startx);
        slot.p2starty = parseFloatOr(playerInfo, "p2starty", slot.p2starty);
        slot.leftbound = parseFloatOr(playerInfo, "leftbound", slot.leftbound);
        slot.rightbound = parseFloatOr(playerInfo, "rightbound", slot.rightbound);

        const auto* bound = findSection(doc, "Bound");
        slot.screenleft = parseFloatOr(bound, "screenleft", slot.screenleft);
        slot.screenright = parseFloatOr(bound, "screenright", slot.screenright);

        const auto* stageInfo = findSection(doc, "StageInfo");
        slot.zoffset = parseFloatOr(stageInfo, "zoffset", slot.zoffset);
    } catch (const std::exception& ex) {
        SDL_Log("Stage DEF parse failed %s: %s", def.string().c_str(), ex.what());
        return std::nullopt;
    }

    return slot;
}

bool hasStageDefPath(const std::vector<StageSlot>& stages, const std::filesystem::path& def) {
    const auto wanted = def.lexically_normal().generic_string();
    return std::any_of(stages.begin(), stages.end(), [&wanted](const StageSlot& stage) {
        return equalsNoCase(stage.defPath.lexically_normal().generic_string(), wanted);
    });
}

void appendStageFromDef(std::vector<StageSlot>& stages, const std::filesystem::path& def) {
    if (def.empty() || hasStageDefPath(stages, def)) {
        return;
    }
    if (auto slot = loadStageSlotFromDef(def)) {
        stages.push_back(std::move(*slot));
    } else {
        SDL_Log("select.def stage entry skipped, DEF not found: %s", def.string().c_str());
    }
}

bool selectEntryIncludesStage(const std::vector<std::string>& parts) {
    bool include = true;
    for (size_t i = 2; i < parts.size(); ++i) {
        const auto equals = parts[i].find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const auto key = trim(std::string_view(parts[i]).substr(0, equals));
        if (equalsNoCase(key, "includestage")) {
            include = parseIntValue(trim(std::string_view(parts[i]).substr(equals + 1)), include ? 1 : 0) != 0;
        }
    }
    return include;
}

std::vector<StageSlot> loadStagesFromFolders(const std::filesystem::path& gameRoot) {
    std::vector<StageSlot> stages;
    const auto stagesRoot = gameRoot / "stages";
    if (!std::filesystem::exists(stagesRoot)) {
        return stages;
    }

    for (const auto& entry : std::filesystem::directory_iterator(stagesRoot)) {
        if (entry.is_regular_file() && equalsNoCase(entry.path().extension().string(), ".def")) {
            appendStageFromDef(stages, entry.path());
        }
    }
    return stages;
}

std::filesystem::path resolveCharacterFile(
    const CharacterSlot& character,
    const MugenSection* files,
    std::string_view key,
    const std::string& fallback) {
    if (files) {
        if (const auto* property = findProperty(*files, key)) {
            return resolveContentPath(character.folder, property->value);
        }
    }
    return character.folder / fallback;
}

void addUniqueExistingPath(std::vector<std::filesystem::path>& paths, const std::filesystem::path& path) {
    if (path.empty() || !std::filesystem::exists(path)) {
        return;
    }
    const auto normalized = path.lexically_normal();
    if (std::find(paths.begin(), paths.end(), normalized) == paths.end()) {
        paths.push_back(normalized);
    }
}

std::pair<float, float> parseCharacterFloatPairValue(const std::string& value, float fallbackX, float fallbackY) {
    const auto parts = splitCsv(value);
    if (parts.empty()) {
        return { fallbackX, fallbackY };
    }

    auto parseFloat = [](const std::string& part, float fallback) {
        try {
            size_t consumed = 0;
            const float parsed = std::stof(trim(part), &consumed);
            return consumed == 0 ? fallback : parsed;
        } catch (...) {
            return fallback;
        }
    };

    return {
        parseFloat(parts[0], fallbackX),
        parts.size() >= 2 ? parseFloat(parts[1], fallbackY) : fallbackY,
    };
}

} // namespace

std::vector<CharacterSlot> loadCharacters(const std::filesystem::path& gameRoot) {
    std::vector<CharacterSlot> characters;
    const auto selectDef = gameRoot / "data" / "select.def";
    if (std::filesystem::exists(selectDef)) {
        try {
            const auto doc = parseMugenTextFile(selectDef);
            if (const auto* section = findSection(doc, "Characters")) {
                for (const auto& line : section->body) {
                    if (auto slot = loadSelectCharacterEntry(gameRoot, line)) {
                        characters.push_back(std::move(*slot));
                    }
                }
            }
        } catch (const std::exception& ex) {
            SDL_Log("select.def character load failed: %s", ex.what());
        }
        if (!characters.empty()) {
            return characters;
        }
    }

    return loadCharactersFromFolders(gameRoot);
}

std::vector<StageSlot> loadStages(const std::filesystem::path& gameRoot) {
    std::vector<StageSlot> stages;
    const auto selectDef = gameRoot / "data" / "select.def";
    if (std::filesystem::exists(selectDef)) {
        try {
            const auto doc = parseMugenTextFile(selectDef);
            if (const auto* section = findSection(doc, "Characters")) {
                for (const auto& line : section->body) {
                    const auto parts = splitCsv(line);
                    if (parts.size() < 2 || equalsNoCase(parts.front(), "randomselect") || !selectEntryIncludesStage(parts)) {
                        continue;
                    }
                    appendStageFromDef(stages, resolveSelectStagePath(gameRoot, parts[1]));
                }
            }
            if (const auto* section = findSection(doc, "ExtraStages")) {
                for (const auto& line : section->body) {
                    const auto parts = splitCsv(line);
                    if (!parts.empty()) {
                        appendStageFromDef(stages, resolveSelectStagePath(gameRoot, parts.front()));
                    }
                }
            }
        } catch (const std::exception& ex) {
            SDL_Log("select.def stage load failed: %s", ex.what());
        }
        if (!stages.empty()) {
            return stages;
        }
    }

    return loadStagesFromFolders(gameRoot);
}

CharacterFiles resolveCharacterFiles(const std::filesystem::path& gameRoot, const CharacterSlot& character) {
    const auto def = parseMugenTextFile(character.defPath);
    const auto* files = findSection(def, "Files");

    CharacterFiles resolved;
    resolved.root = character.folder;
    resolved.def = character.defPath;
    resolved.cmd = resolveCharacterFile(character, files, "cmd", character.id + ".cmd");
    resolved.sprite = resolveCharacterFile(character, files, "sprite", character.id + ".sff");
    resolved.anim = resolveCharacterFile(character, files, "anim", character.id + ".air");
    resolved.sound = resolveCharacterFile(character, files, "sound", character.id + ".snd");
    resolved.palette = resolveCharacterFile(character, files, "pal1", character.id + ".act");

    addUniqueExistingPath(resolved.stateFiles, resolveCharacterFile(character, files, "cns", character.id + ".cns"));
    if (files) {
        for (const auto& property : files->properties) {
            if (startsWithNoCase(property.key, "st") && !startsWithNoCase(property.key, "stcommon")) {
                addUniqueExistingPath(resolved.stateFiles, resolveContentPath(character.folder, property.value));
            }
        }
        if (const auto* common = findProperty(*files, "stcommon")) {
            addUniqueExistingPath(resolved.stateFiles, resolveContentPath(gameRoot / "data", common->value));
        }
        for (const auto& property : files->properties) {
            if (startsWithNoCase(property.key, "pal")) {
                const auto palette = resolveContentPath(character.folder, property.value);
                if (std::filesystem::exists(palette)) {
                    resolved.palette = palette;
                    break;
                }
            }
        }
    }

    return resolved;
}

CharacterConstants loadCharacterConstants(const CharacterFiles& files) {
    CharacterConstants constants;
    for (const auto& path : files.stateFiles) {
        const auto doc = parseMugenTextFile(path);
        if (const auto* data = findSection(doc, "Data")) {
            if (const auto* power = findProperty(*data, "power")) {
                constants.maxPower = parseIntValue(power->value, constants.maxPower);
            }
        }
        if (const auto* size = findSection(doc, "Size")) {
            constants.size.groundBack = parseFloatOr(size, "ground.back", constants.size.groundBack);
            constants.size.groundFront = parseFloatOr(size, "ground.front", constants.size.groundFront);
            constants.size.airBack = parseFloatOr(size, "air.back", constants.size.airBack);
            constants.size.airFront = parseFloatOr(size, "air.front", constants.size.airFront);
            constants.size.height = parseFloatOr(size, "height", constants.size.height);
            constants.attackDistance = parseIntValue(
                findProperty(*size, "attack.dist") ? findProperty(*size, "attack.dist")->value : std::to_string(constants.attackDistance),
                constants.attackDistance);
        }
        if (const auto* velocity = findSection(doc, "Velocity")) {
            constants.velocityWalkFwdX = parseFloatOr(velocity, "walk.fwd", constants.velocityWalkFwdX);
            constants.velocityWalkBackX = parseFloatOr(velocity, "walk.back", constants.velocityWalkBackX);
            if (const auto* runFwd = findProperty(*velocity, "run.fwd")) {
                const auto values = parseCharacterFloatPairValue(runFwd->value, constants.velocityRunFwdX, constants.velocityRunFwdY);
                constants.velocityRunFwdX = values.first;
                constants.velocityRunFwdY = values.second;
            }
            if (const auto* runBack = findProperty(*velocity, "run.back")) {
                const auto values = parseCharacterFloatPairValue(runBack->value, constants.velocityRunBackX, constants.velocityRunBackY);
                constants.velocityRunBackX = values.first;
                constants.velocityRunBackY = values.second;
            }
        }
        if (const auto* movement = findSection(doc, "Movement")) {
            constants.movementYAccel = parseFloatOr(movement, "yaccel", constants.movementYAccel);
        }
    }
    return constants;
}

} // namespace dragon
