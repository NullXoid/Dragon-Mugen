#include "dragon/DragonProgression.h"

#include "dragon/MugenText.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace dragon {
namespace {

std::string lowercase(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    return lowercase(lhs) == lowercase(rhs);
}

bool startsWithNoCase(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size()
        && equalsNoCase(value.substr(0, prefix.size()), prefix);
}

std::string unquote(std::string_view value) {
    std::string out = trim(value);
    if (out.size() >= 2 && ((out.front() == '"' && out.back() == '"') || (out.front() == '\'' && out.back() == '\''))) {
        return out.substr(1, out.size() - 2);
    }
    return out;
}

int parseInt(std::string_view value, int fallback) {
    try {
        size_t consumed = 0;
        const int parsed = std::stoi(trim(value), &consumed, 10);
        return consumed == 0 ? fallback : parsed;
    } catch (...) {
        return fallback;
    }
}

bool parseBool(std::string_view value, bool fallback) {
    const std::string normalized = lowercase(trim(value));
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return fallback;
}

std::string propertyString(const MugenSection& section, std::string_view key, std::string fallback = {}) {
    if (const auto* property = findProperty(section, key)) {
        return unquote(property->value);
    }
    return fallback;
}

int propertyInt(const MugenSection& section, std::string_view key, int fallback) {
    if (const auto* property = findProperty(section, key)) {
        return parseInt(property->value, fallback);
    }
    return fallback;
}

bool propertyBool(const MugenSection& section, std::string_view key, bool fallback) {
    if (const auto* property = findProperty(section, key)) {
        return parseBool(property->value, fallback);
    }
    return fallback;
}

std::vector<std::string> splitCsv(std::string_view value) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t comma = value.find(',', start);
        const size_t end = comma == std::string_view::npos ? value.size() : comma;
        std::string item = unquote(value.substr(start, end - start));
        if (!item.empty()) {
            out.push_back(std::move(item));
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return out;
}

std::string joinCsv(const std::vector<std::string>& values) {
    std::string out;
    for (const auto& value : values) {
        if (!out.empty()) {
            out += ", ";
        }
        out += value;
    }
    return out;
}

DragonCharacterProgressionDefinition defaultDefinition(
    const DragonProgressionConfig& config,
    std::string_view characterId) {
    DragonCharacterProgressionDefinition definition;
    definition.id = std::string(characterId);
    definition.displayName = std::string(characterId);
    definition.maxLevel = std::max(1, config.defaultMaxLevel);
    definition.baseXp = std::max(1, config.defaultBaseXp);
    definition.xpGrowth = std::max(0, config.defaultXpGrowth);
    definition.lifePerLevel = config.defaultLifePerLevel;
    definition.attackPermillePerLevel = config.defaultAttackPermillePerLevel;
    definition.defencePermillePerLevel = config.defaultDefencePermillePerLevel;
    return definition;
}

DragonCharacterProgressionDefinition parseCharacterDefinition(
    const MugenSection& section,
    const DragonProgressionConfig& config) {
    DragonCharacterProgressionDefinition definition = defaultDefinition(config, propertyString(section, "id"));
    definition.displayName = propertyString(section, "name", definition.id);
    definition.maxLevel = std::max(1, propertyInt(section, "maxlevel", definition.maxLevel));
    definition.baseXp = std::max(1, propertyInt(section, "xp.base", definition.baseXp));
    definition.xpGrowth = std::max(0, propertyInt(section, "xp.growth", definition.xpGrowth));
    definition.lifePerLevel = propertyInt(section, "life.perlevel", definition.lifePerLevel);
    definition.attackPermillePerLevel = propertyInt(section, "attack.perlevel", definition.attackPermillePerLevel);
    definition.defencePermillePerLevel = propertyInt(section, "defence.perlevel", definition.defencePermillePerLevel);
    return definition;
}

DragonItemDefinition parseItemDefinition(const MugenSection& section) {
    DragonItemDefinition item;
    item.id = propertyString(section, "id");
    item.displayName = propertyString(section, "name", item.id);
    item.slot = propertyString(section, "slot", "accessory");
    item.description = propertyString(section, "description");
    item.requiredLevel = std::max(1, propertyInt(section, "level", item.requiredLevel));
    item.price = std::max(0, propertyInt(section, "price", item.price));
    item.sellPrice = std::max(0, propertyInt(section, "sell", item.price > 0 ? std::max(1, item.price / 2) : item.sellPrice));
    item.lifeBonus = propertyInt(section, "life", item.lifeBonus);
    item.powerBonus = propertyInt(section, "power", item.powerBonus);
    item.attackPermille = propertyInt(section, "attack", item.attackPermille);
    item.defencePermille = propertyInt(section, "defence", item.defencePermille);
    return item;
}

DragonProgressionConfig parseConfig(const MugenDocument& doc) {
    DragonProgressionConfig config;
    if (const auto* section = findSection(doc, "Dragon.Progression")) {
        config.enabled = propertyBool(*section, "enabled", config.enabled);
        config.winXp = std::max(0, propertyInt(*section, "win.xp", config.winXp));
        config.winGold = std::max(0, propertyInt(*section, "win.gold", config.winGold));
        config.lossXp = std::max(0, propertyInt(*section, "loss.xp", config.lossXp));
        config.lossGold = std::max(0, propertyInt(*section, "loss.gold", config.lossGold));
        config.arenaWinXp = std::max(0, propertyInt(*section, "arena.win.xp", config.arenaWinXp));
        config.arenaWinGold = std::max(0, propertyInt(*section, "arena.win.gold", config.arenaWinGold));
        config.enemyDefeatXp = std::max(0, propertyInt(*section, "enemy.defeat.xp", config.enemyDefeatXp));
        config.enemyDefeatGold = std::max(0, propertyInt(*section, "enemy.defeat.gold", config.enemyDefeatGold));
        config.defaultMaxLevel = std::max(1, propertyInt(*section, "default.maxlevel", config.defaultMaxLevel));
        config.defaultBaseXp = std::max(1, propertyInt(*section, "default.xp.base", config.defaultBaseXp));
        config.defaultXpGrowth = std::max(0, propertyInt(*section, "default.xp.growth", config.defaultXpGrowth));
        config.defaultLifePerLevel = propertyInt(*section, "default.life.perlevel", config.defaultLifePerLevel);
        config.defaultAttackPermillePerLevel =
            propertyInt(*section, "default.attack.perlevel", config.defaultAttackPermillePerLevel);
        config.defaultDefencePermillePerLevel =
            propertyInt(*section, "default.defence.perlevel", config.defaultDefencePermillePerLevel);
    }
    return config;
}

DragonCharacterProgressionState parseCharacterState(const MugenSection& section, std::string_view explicitId = {}) {
    DragonCharacterProgressionState state;
    state.id = std::string(explicitId);
    const std::string prefix = "Character ";
    if (state.id.empty()
        && section.name.size() > prefix.size()
        && startsWithNoCase(section.name, prefix)) {
        state.id = trim(std::string_view(section.name).substr(prefix.size()));
    }
    state.id = propertyString(section, "id", state.id);
    state.level = std::max(1, propertyInt(section, "level", state.level));
    state.xp = std::max(0, propertyInt(section, "xp", state.xp));
    state.victories = std::max(0, propertyInt(section, "victories", state.victories));
    state.defeats = std::max(0, propertyInt(section, "defeats", state.defeats));
    state.equippedItems = splitCsv(propertyString(section, "equipped"));
    return state;
}

void parseInventoryInto(const MugenSection& section, std::vector<DragonInventoryEntry>& inventory) {
    for (const auto& property : section.properties) {
        const int quantity = std::max(0, parseInt(property.value, 0));
        if (property.key.empty() || quantity <= 0) {
            continue;
        }
        auto existing = std::find_if(inventory.begin(), inventory.end(), [&](const auto& item) {
            return equalsNoCase(item.itemId, property.key);
        });
        if (existing != inventory.end()) {
            existing->quantity = std::max(0, existing->quantity + quantity);
        } else {
            inventory.push_back(DragonInventoryEntry{ property.key, quantity });
        }
    }
}

DragonCharacterProgressionState* findCharacterState(
    std::vector<DragonCharacterProgressionState>& characters,
    std::string_view characterId) {
    auto it = std::find_if(characters.begin(), characters.end(), [&](const auto& character) {
        return equalsNoCase(character.id, characterId);
    });
    return it == characters.end() ? nullptr : &*it;
}

const DragonCharacterProgressionState* findCharacterState(
    const std::vector<DragonCharacterProgressionState>& characters,
    std::string_view characterId) {
    auto it = std::find_if(characters.begin(), characters.end(), [&](const auto& character) {
        return equalsNoCase(character.id, characterId);
    });
    return it == characters.end() ? nullptr : &*it;
}

void mergeCharacterState(
    std::vector<DragonCharacterProgressionState>& characters,
    DragonCharacterProgressionState state) {
    if (state.id.empty()) {
        return;
    }
    if (auto* existing = findCharacterState(characters, state.id)) {
        *existing = std::move(state);
    } else {
        characters.push_back(std::move(state));
    }
}

void mergeInventoryEntry(std::vector<DragonInventoryEntry>& inventory, DragonInventoryEntry entry) {
    if (entry.itemId.empty() || entry.quantity <= 0) {
        return;
    }
    auto existing = std::find_if(inventory.begin(), inventory.end(), [&](const auto& item) {
        return equalsNoCase(item.itemId, entry.itemId);
    });
    if (existing != inventory.end()) {
        existing->quantity = std::max(0, existing->quantity + entry.quantity);
    } else {
        inventory.push_back(std::move(entry));
    }
}

} // namespace

std::filesystem::path dragonProgressionDataPath(const std::filesystem::path& gameRoot) {
    return gameRoot / "data" / "dragon.def";
}

std::filesystem::path dragonProgressionSavePath(const std::filesystem::path& gameRoot) {
    return gameRoot / "save" / "progression.def";
}

DragonProgressionData loadDragonProgressionData(const std::filesystem::path& gameRoot) {
    DragonProgressionData data;
    const auto path = dragonProgressionDataPath(gameRoot);
    if (!std::filesystem::exists(path)) {
        return data;
    }

    const MugenDocument doc = parseMugenTextFile(path);
    data.config = parseConfig(doc);
    for (const auto& section : doc.sections) {
        if (equalsNoCase(section.name, "Dragon.Progression.Character")) {
            auto definition = parseCharacterDefinition(section, data.config);
            if (!definition.id.empty()) {
                data.characters.push_back(std::move(definition));
            }
        } else if (equalsNoCase(section.name, "Dragon.Progression.Item")) {
            auto item = parseItemDefinition(section);
            if (!item.id.empty()) {
                data.items.push_back(std::move(item));
            }
        }
    }
    return data;
}

DragonProgressionSave loadDragonProgressionSave(const std::filesystem::path& path) {
    DragonProgressionSave save;
    if (!std::filesystem::exists(path)) {
        setDragonProgressionPlayerProfile(save, 0, defaultDragonProgressionProfileName());
        setDragonProgressionPlayerProfile(save, 1, dragonProgressionGuestProfileId());
        return save;
    }

    const MugenDocument doc = parseMugenTextFile(path);
    std::vector<DragonCharacterProgressionState> legacyCharacters;
    std::vector<DragonInventoryEntry> legacyInventory;
    std::string declaredActiveProfile;
    std::string declaredP1Profile;
    std::string declaredP2Profile;
    for (const auto& section : doc.sections) {
        if (equalsNoCase(section.name, "Dragon.Progression.Save")) {
            declaredActiveProfile = propertyString(section, "active.profile", declaredActiveProfile);
            declaredP1Profile = propertyString(section, "p1.profile", declaredP1Profile);
            declaredP2Profile = propertyString(section, "p2.profile", declaredP2Profile);
        } else if (startsWithNoCase(section.name, "Profile ")) {
            const std::string rest = trim(std::string_view(section.name).substr(8));
            const size_t dot = rest.find('.');
            const std::string profileId = dot == std::string::npos ? rest : trim(std::string_view(rest).substr(0, dot));
            const std::string subSection = dot == std::string::npos
                ? std::string{}
                : trim(std::string_view(rest).substr(dot + 1));
            auto& profile = ensureDragonProgressionProfile(
                save,
                profileId,
                propertyString(section, "name", profileId));
            if (subSection.empty()) {
                profile.gold = std::max(0, propertyInt(section, "gold", profile.gold));
                continue;
            }
            if (startsWithNoCase(subSection, "Character ")) {
                auto state = parseCharacterState(section, trim(std::string_view(subSection).substr(10)));
                mergeCharacterState(profile.characters, std::move(state));
            } else if (equalsNoCase(subSection, "Inventory")) {
                parseInventoryInto(section, profile.inventory);
            }
        } else if (startsWithNoCase(section.name, "Character")) {
            auto state = parseCharacterState(section);
            if (!state.id.empty()) {
                legacyCharacters.push_back(std::move(state));
            }
        } else if (equalsNoCase(section.name, "Inventory")) {
            parseInventoryInto(section, legacyInventory);
        }
    }
    if (!declaredP1Profile.empty()) {
        setDragonProgressionPlayerProfile(save, 0, declaredP1Profile);
    } else if (!declaredActiveProfile.empty()) {
        setDragonProgressionPlayerProfile(save, 0, declaredActiveProfile);
    } else if (save.playerProfileIds[0].empty()) {
        setDragonProgressionPlayerProfile(save, 0, defaultDragonProgressionProfileName());
    }
    if (!declaredP2Profile.empty()) {
        setDragonProgressionPlayerProfile(save, 1, declaredP2Profile);
    } else if (save.playerProfileIds[1].empty()) {
        setDragonProgressionPlayerProfile(save, 1, dragonProgressionGuestProfileId());
    }

    if (!legacyCharacters.empty() || !legacyInventory.empty()) {
        auto& profile = ensureDragonProgressionProfile(save, dragonProgressionPlayerProfileId(save, 0));
        for (auto& character : legacyCharacters) {
            mergeCharacterState(profile.characters, std::move(character));
        }
        for (auto& item : legacyInventory) {
            mergeInventoryEntry(profile.inventory, std::move(item));
        }
    }
    setDragonProgressionPlayerProfile(save, 0, dragonProgressionPlayerProfileId(save, 0));
    setDragonProgressionPlayerProfile(save, 1, dragonProgressionPlayerProfileId(save, 1));
    return save;
}

void saveDragonProgressionSave(const std::filesystem::path& path, const DragonProgressionSave& save) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Could not write Dragon progression save: " + path.string());
    }

    output << "; Dragon local progression save. This is local player data, not M.U.G.E.N content.\n";
    const std::string activeProfileId = save.activeProfileId.empty()
        ? dragonProgressionPlayerProfileId(save, 0)
        : normalizeDragonProgressionProfileId(save.activeProfileId);
    output << "[Dragon.Progression.Save]\nversion = 2\n";
    output << "active.profile = " << activeProfileId << "\n";
    output << "p1.profile = " << dragonProgressionPlayerProfileId(save, 0) << "\n";
    output << "p2.profile = " << dragonProgressionPlayerProfileId(save, 1) << "\n\n";
    for (const auto& profile : save.profiles) {
        const std::string profileId = normalizeDragonProgressionProfileId(
            profile.id.empty() ? profile.displayName : profile.id);
        if (profileId.empty() || isDragonProgressionGuestProfile(profileId)) {
            continue;
        }
        const std::string displayName = trim(profile.displayName).empty() ? profileId : trim(profile.displayName);
        output << "[Profile " << profileId << "]\n";
        output << "name = " << displayName << "\n";
        output << "gold = " << std::max(0, profile.gold) << "\n\n";
        for (const auto& character : profile.characters) {
            if (character.id.empty()) {
                continue;
            }
            output << "[Profile " << profileId << ".Character " << character.id << "]\n";
            output << "level = " << std::max(1, character.level) << "\n";
            output << "xp = " << std::max(0, character.xp) << "\n";
            output << "victories = " << std::max(0, character.victories) << "\n";
            output << "defeats = " << std::max(0, character.defeats) << "\n";
            output << "equipped = " << joinCsv(character.equippedItems) << "\n\n";
        }
        output << "[Profile " << profileId << ".Inventory]\n";
        for (const auto& item : profile.inventory) {
            if (!item.itemId.empty() && item.quantity > 0) {
                output << item.itemId << " = " << item.quantity << "\n";
            }
        }
        output << "\n";
    }
}

const DragonCharacterProgressionDefinition& resolveCharacterProgressionDefinition(
    const DragonProgressionData& data,
    std::string_view characterId) {
    for (const auto& definition : data.characters) {
        if (equalsNoCase(definition.id, characterId)) {
            return definition;
        }
    }

    static thread_local DragonCharacterProgressionDefinition fallback;
    fallback = defaultDefinition(data.config, characterId);
    return fallback;
}

const DragonItemDefinition* findDragonProgressionItem(
    const DragonProgressionData& data,
    std::string_view itemId) {
    for (const auto& item : data.items) {
        if (equalsNoCase(item.id, itemId)) {
            return &item;
        }
    }
    return nullptr;
}

int dragonXpForNextLevel(const DragonCharacterProgressionDefinition& definition, int level) {
    if (level >= std::max(1, definition.maxLevel)) {
        return 0;
    }
    return std::max(1, definition.baseXp + std::max(0, level - 1) * definition.xpGrowth);
}

DragonCharacterProgressionState& ensureCharacterProgression(
    DragonProgressionSave& save,
    std::string_view characterId) {
    return ensureCharacterProgressionForProfile(save, dragonProgressionPlayerProfileId(save, 0), characterId);
}

DragonCharacterProgressionState& ensureCharacterProgressionForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId) {
    auto& profile = ensureDragonProgressionProfile(save, profileId);
    for (auto& character : profile.characters) {
        if (equalsNoCase(character.id, characterId)) {
            character.level = std::max(1, character.level);
            character.xp = std::max(0, character.xp);
            return character;
        }
    }
    DragonCharacterProgressionState state;
    state.id = std::string(characterId);
    profile.characters.push_back(std::move(state));
    return profile.characters.back();
}

std::optional<DragonInventoryEntry> inventoryEntry(
    const DragonProgressionSave& save,
    std::string_view itemId) {
    return inventoryEntryForProfile(save, dragonProgressionPlayerProfileId(save, 0), itemId);
}

std::optional<DragonInventoryEntry> inventoryEntryForProfile(
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view itemId) {
    if (isDragonProgressionGuestProfile(profileId)) {
        return std::nullopt;
    }
    const DragonProgressionProfile* profile = findDragonProgressionProfile(save, profileId);
    if (!profile) {
        return std::nullopt;
    }
    for (const auto& item : profile->inventory) {
        if (equalsNoCase(item.itemId, itemId)) {
            return item;
        }
    }
    return std::nullopt;
}

int dragonProgressionGoldForProfile(
    const DragonProgressionSave& save,
    std::string_view profileId) {
    if (isDragonProgressionGuestProfile(profileId)) {
        return 0;
    }
    const DragonProgressionProfile* profile = findDragonProgressionProfile(save, profileId);
    return profile ? std::max(0, profile->gold) : 0;
}

void addDragonProgressionGoldForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    int amount) {
    if (amount <= 0 || isDragonProgressionGuestProfile(profileId)) {
        return;
    }
    auto& profile = ensureDragonProgressionProfile(save, profileId);
    profile.gold = std::max(0, profile.gold + amount);
}

bool spendDragonProgressionGoldForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    int amount) {
    if (amount < 0 || isDragonProgressionGuestProfile(profileId)) {
        return false;
    }
    auto& profile = ensureDragonProgressionProfile(save, profileId);
    if (profile.gold < amount) {
        return false;
    }
    profile.gold -= amount;
    return true;
}

void grantDragonProgressionItem(
    DragonProgressionSave& save,
    std::string_view itemId,
    int quantity) {
    grantDragonProgressionItemForProfile(save, dragonProgressionPlayerProfileId(save, 0), itemId, quantity);
}

void grantDragonProgressionItemForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view itemId,
    int quantity) {
    if (itemId.empty() || quantity <= 0) {
        return;
    }
    if (isDragonProgressionGuestProfile(profileId)) {
        return;
    }
    auto& profile = ensureDragonProgressionProfile(save, profileId);
    for (auto& item : profile.inventory) {
        if (equalsNoCase(item.itemId, itemId)) {
            item.quantity = std::max(0, item.quantity + quantity);
            return;
        }
    }
    profile.inventory.push_back(DragonInventoryEntry{ std::string(itemId), quantity });
}

bool removeDragonProgressionItemForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view itemId,
    int quantity) {
    if (itemId.empty() || quantity <= 0 || isDragonProgressionGuestProfile(profileId)) {
        return false;
    }
    const std::string normalizedProfileId = normalizeDragonProgressionProfileId(profileId);
    auto profileIt = std::find_if(save.profiles.begin(), save.profiles.end(), [&](const auto& profile) {
        return equalsNoCase(profile.id, normalizedProfileId);
    });
    if (profileIt == save.profiles.end()) return false;
    auto& profile = *profileIt;
    for (auto it = profile.inventory.begin(); it != profile.inventory.end(); ++it) {
        if (!equalsNoCase(it->itemId, itemId)) {
            continue;
        }
        if (it->quantity < quantity) {
            return false;
        }
        it->quantity -= quantity;
        if (it->quantity <= 0) {
            profile.inventory.erase(it);
        }
        return true;
    }
    return false;
}

bool equipDragonProgressionItem(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view characterId,
    std::string_view itemId,
    std::string* reason) {
    return equipDragonProgressionItemForProfile(
        data,
        save,
        dragonProgressionPlayerProfileId(save, 0),
        characterId,
        itemId,
        reason);
}

bool equipDragonProgressionItemForProfile(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view itemId,
    std::string* reason) {
    if (isDragonProgressionGuestProfile(profileId)) {
        if (reason) *reason = "guest profile has no inventory";
        return false;
    }
    const DragonItemDefinition* item = findDragonProgressionItem(data, itemId);
    if (!item) {
        if (reason) *reason = "item not defined";
        return false;
    }
    const auto inventory = inventoryEntryForProfile(save, profileId, itemId);
    if (!inventory || inventory->quantity <= 0) {
        if (reason) *reason = "item not owned";
        return false;
    }

    auto& character = ensureCharacterProgressionForProfile(save, profileId, characterId);
    const auto& definition = resolveCharacterProgressionDefinition(data, characterId);
    character.level = std::clamp(character.level, 1, std::max(1, definition.maxLevel));
    if (character.level < item->requiredLevel) {
        if (reason) *reason = "level requirement not met";
        return false;
    }

    character.equippedItems.erase(
        std::remove_if(
            character.equippedItems.begin(),
            character.equippedItems.end(),
            [&](const std::string& equippedId) {
                if (equalsNoCase(equippedId, itemId)) {
                    return true;
                }
                const DragonItemDefinition* equippedItem = findDragonProgressionItem(data, equippedId);
                return equippedItem && !item->slot.empty() && equalsNoCase(equippedItem->slot, item->slot);
            }),
        character.equippedItems.end());
    character.equippedItems.push_back(std::string(itemId));
    if (reason) *reason = "equipped";
    return true;
}

bool unequipDragonProgressionItemForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view itemId) {
    if (isDragonProgressionGuestProfile(profileId)) {
        return false;
    }
    auto& character = ensureCharacterProgressionForProfile(save, profileId, characterId);
    const auto before = character.equippedItems.size();
    character.equippedItems.erase(
        std::remove_if(
            character.equippedItems.begin(),
            character.equippedItems.end(),
            [&](const std::string& equippedId) { return equalsNoCase(equippedId, itemId); }),
        character.equippedItems.end());
    return character.equippedItems.size() != before;
}

bool isDragonProgressionItemEquippedForProfile(
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view itemId) {
    if (isDragonProgressionGuestProfile(profileId)) {
        return false;
    }
    const DragonProgressionProfile* profile = findDragonProgressionProfile(save, profileId);
    if (!profile) {
        return false;
    }
    const DragonCharacterProgressionState* character = findCharacterState(profile->characters, characterId);
    if (!character) {
        return false;
    }
    return std::any_of(character->equippedItems.begin(), character->equippedItems.end(), [&](const auto& equippedId) {
        return equalsNoCase(equippedId, itemId);
    });
}

DragonProgressionAwardResult recordDragonProgressionMatch(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view characterId,
    std::string_view characterName,
    bool won,
    bool arenaMode) {
    return recordDragonProgressionMatchForProfile(
        data,
        save,
        dragonProgressionPlayerProfileId(save, 0),
        characterId,
        characterName,
        won,
        arenaMode);
}

DragonProgressionAwardResult recordDragonProgressionMatchForProfile(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view characterName,
    bool won,
    bool arenaMode) {
    DragonProgressionAwardResult result;
    if (!data.config.enabled || characterId.empty() || isDragonProgressionGuestProfile(profileId)) {
        return result;
    }

    auto& character = ensureCharacterProgressionForProfile(save, profileId, characterId);
    const auto& definition = resolveCharacterProgressionDefinition(data, characterId);
    const int maxLevel = std::max(1, definition.maxLevel);
    character.level = std::clamp(character.level, 1, maxLevel);
    character.xp = std::max(0, character.xp);

    result.applied = true;
    result.won = won;
    result.oldLevel = character.level;
    result.characterId = std::string(characterId);
    result.characterName = characterName.empty() ? std::string(characterId) : std::string(characterName);
    result.xpGained = won ? (arenaMode ? data.config.arenaWinXp : data.config.winXp) : data.config.lossXp;
    result.goldGained = won ? (arenaMode ? data.config.arenaWinGold : data.config.winGold) : data.config.lossGold;

    if (won) {
        ++character.victories;
    } else {
        ++character.defeats;
    }

    character.xp += std::max(0, result.xpGained);
    while (character.level < maxLevel) {
        const int needed = dragonXpForNextLevel(definition, character.level);
        if (needed <= 0 || character.xp < needed) {
            break;
        }
        character.xp -= needed;
        ++character.level;
    }

    if (character.level >= maxLevel) {
        character.level = maxLevel;
        character.xp = 0;
    }
    result.newLevel = character.level;
    result.xp = character.xp;
    result.xpForNextLevel = dragonXpForNextLevel(definition, character.level);
    if (result.goldGained > 0) {
        addDragonProgressionGoldForProfile(save, profileId, result.goldGained);
    }
    return result;
}

DragonProgressionAwardResult recordDragonProgressionRewardForProfile(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view characterName,
    int xp,
    int gold) {
    DragonProgressionAwardResult result;
    const int clampedXp = std::max(0, xp);
    const int clampedGold = std::max(0, gold);
    if (!data.config.enabled
        || characterId.empty()
        || isDragonProgressionGuestProfile(profileId)
        || (clampedXp <= 0 && clampedGold <= 0)) {
        return result;
    }

    auto& character = ensureCharacterProgressionForProfile(save, profileId, characterId);
    const auto& definition = resolveCharacterProgressionDefinition(data, characterId);
    const int maxLevel = std::max(1, definition.maxLevel);
    character.level = std::clamp(character.level, 1, maxLevel);
    character.xp = std::max(0, character.xp);

    result.applied = true;
    result.oldLevel = character.level;
    result.characterId = std::string(characterId);
    result.characterName = characterName.empty() ? std::string(characterId) : std::string(characterName);
    result.xpGained = clampedXp;
    result.goldGained = clampedGold;

    character.xp += clampedXp;
    while (character.level < maxLevel) {
        const int needed = dragonXpForNextLevel(definition, character.level);
        if (needed <= 0 || character.xp < needed) {
            break;
        }
        character.xp -= needed;
        ++character.level;
    }
    if (character.level >= maxLevel) {
        character.level = maxLevel;
        character.xp = 0;
    }
    result.newLevel = character.level;
    result.xp = character.xp;
    result.xpForNextLevel = dragonXpForNextLevel(definition, character.level);
    if (clampedGold > 0) {
        addDragonProgressionGoldForProfile(save, profileId, clampedGold);
    }
    return result;
}

DragonEffectiveProgressionStats effectiveDragonProgressionStats(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view characterId) {
    return effectiveDragonProgressionStatsForProfile(
        data,
        save,
        dragonProgressionPlayerProfileId(save, 0),
        characterId);
}

DragonEffectiveProgressionStats effectiveDragonProgressionStatsForProfile(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId) {
    DragonEffectiveProgressionStats stats;
    stats.characterId = std::string(characterId);
    const auto& definition = resolveCharacterProgressionDefinition(data, characterId);
    const DragonProgressionProfile* profile = isDragonProgressionGuestProfile(profileId)
        ? nullptr
        : findDragonProgressionProfile(save, profileId);
    const DragonCharacterProgressionState* character = profile
        ? findCharacterState(profile->characters, characterId)
        : nullptr;
    const int level = character ? character->level : 1;
    const int levelDelta = std::max(0, std::clamp(level, 1, std::max(1, definition.maxLevel)) - 1);
    stats.level = levelDelta + 1;
    stats.xp = character ? std::max(0, character->xp) : 0;
    stats.xpForNextLevel = dragonXpForNextLevel(definition, stats.level);
    stats.lifeBonus = levelDelta * definition.lifePerLevel;
    stats.attackPermille = 1000 + levelDelta * definition.attackPermillePerLevel;
    stats.defencePermille = 1000 + levelDelta * definition.defencePermillePerLevel;

    if (!character) {
        return stats;
    }

    for (const auto& itemId : character->equippedItems) {
        const DragonItemDefinition* item = findDragonProgressionItem(data, itemId);
        if (!item || stats.level < item->requiredLevel) {
            continue;
        }
        stats.equippedItems.push_back(item->id);
        stats.lifeBonus += item->lifeBonus;
        stats.powerBonus += item->powerBonus;
        stats.attackPermille += item->attackPermille;
        stats.defencePermille += item->defencePermille;
    }
    return stats;
}

std::string dragonProgressionAwardSummary(const DragonProgressionAwardResult& result) {
    if (!result.applied) {
        return {};
    }

    std::string summary = "PROGRESS ";
    summary += result.characterName.empty() ? result.characterId : result.characterName;
    summary += " +";
    summary += std::to_string(result.xpGained);
    summary += " XP";
    if (result.goldGained > 0) {
        summary += " +";
        summary += std::to_string(result.goldGained);
        summary += "G";
    }
    if (result.newLevel > result.oldLevel) {
        summary += " LV ";
        summary += std::to_string(result.oldLevel);
        summary += "->";
        summary += std::to_string(result.newLevel);
    } else {
        summary += " LV ";
        summary += std::to_string(result.newLevel);
        if (result.xpForNextLevel > 0) {
            summary += " ";
            summary += std::to_string(result.xp);
            summary += "/";
            summary += std::to_string(result.xpForNextLevel);
        } else {
            summary += " MAX";
        }
    }
    return summary;
}

std::string dragonProgressionAwardSummaryWithGoldBalance(
    const DragonProgressionAwardResult& result,
    int goldBalance) {
    std::string summary = dragonProgressionAwardSummary(result);
    if (summary.empty()) {
        return {};
    }
    summary += " BAL ";
    summary += std::to_string(std::max(0, goldBalance));
    summary += "G";
    return summary;
}

std::string dragonProgressionStatsSummary(const DragonEffectiveProgressionStats& stats) {
    std::string summary = "LV ";
    summary += std::to_string(std::max(1, stats.level));
    summary += " XP ";
    if (stats.xpForNextLevel > 0) {
        summary += std::to_string(std::max(0, stats.xp));
        summary += "/";
        summary += std::to_string(stats.xpForNextLevel);
    } else {
        summary += "MAX";
    }
    return summary;
}

std::string dragonProgressionCharacterSummary(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view characterId) {
    return dragonProgressionStatsSummary(effectiveDragonProgressionStats(data, save, characterId));
}

std::string dragonProgressionCharacterSummaryForProfile(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId) {
    return dragonProgressionStatsSummary(effectiveDragonProgressionStatsForProfile(data, save, profileId, characterId));
}

} // namespace dragon
