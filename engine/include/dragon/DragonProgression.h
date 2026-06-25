#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dragon {

inline constexpr int kDragonProgressionPlayerCount = 2;

struct DragonProgressionConfig {
    bool enabled = false;
    int winXp = 75;
    int winGold = 30;
    int lossXp = 10;
    int lossGold = 0;
    int arenaWinXp = 100;
    int arenaWinGold = 50;
    int enemyDefeatXp = 18;
    int enemyDefeatGold = 20;
    int defaultMaxLevel = 20;
    int defaultBaseXp = 100;
    int defaultXpGrowth = 50;
    int defaultLifePerLevel = 0;
    int defaultAttackPermillePerLevel = 10;
    int defaultDefencePermillePerLevel = 10;
};

struct DragonCharacterProgressionDefinition {
    std::string id;
    std::string displayName;
    int maxLevel = 20;
    int baseXp = 100;
    int xpGrowth = 50;
    int lifePerLevel = 0;
    int attackPermillePerLevel = 10;
    int defencePermillePerLevel = 10;
};

struct DragonItemDefinition {
    std::string id;
    std::string displayName;
    std::string slot;
    std::string description;
    int requiredLevel = 1;
    int price = 100;
    int sellPrice = 50;
    int lifeBonus = 0;
    int powerBonus = 0;
    int attackPermille = 0;
    int defencePermille = 0;
};

struct DragonProgressionData {
    DragonProgressionConfig config;
    std::vector<DragonCharacterProgressionDefinition> characters;
    std::vector<DragonItemDefinition> items;
};

struct DragonInventoryEntry {
    std::string itemId;
    int quantity = 0;
};

struct DragonCharacterProgressionState {
    std::string id;
    int level = 1;
    int xp = 0;
    int victories = 0;
    int defeats = 0;
    std::vector<std::string> equippedItems;
};

struct DragonProgressionProfile {
    std::string id;
    std::string displayName;
    int gold = 500;
    std::vector<DragonCharacterProgressionState> characters;
    std::vector<DragonInventoryEntry> inventory;
};

struct DragonProgressionSave {
    std::string activeProfileId;
    std::array<std::string, kDragonProgressionPlayerCount> playerProfileIds;
    std::vector<DragonProgressionProfile> profiles;
};

struct DragonProgressionAwardResult {
    bool applied = false;
    bool won = false;
    int xpGained = 0;
    int goldGained = 0;
    int oldLevel = 1;
    int newLevel = 1;
    int xp = 0;
    int xpForNextLevel = 0;
    std::string characterId;
    std::string characterName;
};

struct DragonEffectiveProgressionStats {
    std::string characterId;
    int level = 1;
    int xp = 0;
    int xpForNextLevel = 0;
    int lifeBonus = 0;
    int powerBonus = 0;
    int attackPermille = 1000;
    int defencePermille = 1000;
    std::vector<std::string> equippedItems;
};

std::filesystem::path dragonProgressionDataPath(const std::filesystem::path& gameRoot);
std::filesystem::path dragonProgressionSavePath(const std::filesystem::path& gameRoot);

DragonProgressionData loadDragonProgressionData(const std::filesystem::path& gameRoot);
DragonProgressionSave loadDragonProgressionSave(const std::filesystem::path& path);
void saveDragonProgressionSave(const std::filesystem::path& path, const DragonProgressionSave& save);

const DragonCharacterProgressionDefinition& resolveCharacterProgressionDefinition(
    const DragonProgressionData& data,
    std::string_view characterId);
const DragonItemDefinition* findDragonProgressionItem(
    const DragonProgressionData& data,
    std::string_view itemId);

int dragonXpForNextLevel(const DragonCharacterProgressionDefinition& definition, int level);
std::string dragonProgressionGuestProfileId();
std::string dragonProgressionGuestProfileName();
bool isDragonProgressionGuestProfile(std::string_view profileId);
std::string defaultDragonProgressionProfileName();
std::string normalizeDragonProgressionProfileId(std::string_view profileName);
const DragonProgressionProfile* findDragonProgressionProfile(
    const DragonProgressionSave& save,
    std::string_view profileId);
DragonProgressionProfile& ensureDragonProgressionProfile(
    DragonProgressionSave& save,
    std::string_view profileId = {},
    std::string_view displayName = {});
std::string createNextDragonProgressionProfile(
    DragonProgressionSave& save,
    std::string_view preferredBaseName = "Player");
const DragonProgressionProfile* activeDragonProgressionProfile(const DragonProgressionSave& save);
DragonProgressionProfile& activeDragonProgressionProfile(DragonProgressionSave& save);
void setActiveDragonProgressionProfile(
    DragonProgressionSave& save,
    std::string_view profileName);
std::string dragonProgressionProfileDisplayName(const DragonProgressionSave& save);
std::string dragonProgressionProfileDisplayName(
    const DragonProgressionSave& save,
    std::string_view profileId);
std::string dragonProgressionPlayerProfileId(
    const DragonProgressionSave& save,
    int playerIndex);
std::string dragonProgressionPlayerProfileDisplayName(
    const DragonProgressionSave& save,
    int playerIndex);
void setDragonProgressionPlayerProfile(
    DragonProgressionSave& save,
    int playerIndex,
    std::string_view profileName);
void cycleDragonProgressionPlayerProfile(
    DragonProgressionSave& save,
    int playerIndex,
    int direction);
DragonCharacterProgressionState& ensureCharacterProgression(
    DragonProgressionSave& save,
    std::string_view characterId);
DragonCharacterProgressionState& ensureCharacterProgressionForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId);
std::optional<DragonInventoryEntry> inventoryEntry(
    const DragonProgressionSave& save,
    std::string_view itemId);
std::optional<DragonInventoryEntry> inventoryEntryForProfile(
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view itemId);

int dragonProgressionGoldForProfile(
    const DragonProgressionSave& save,
    std::string_view profileId);
void addDragonProgressionGoldForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    int amount);
bool spendDragonProgressionGoldForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    int amount);
void grantDragonProgressionItem(
    DragonProgressionSave& save,
    std::string_view itemId,
    int quantity = 1);
void grantDragonProgressionItemForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view itemId,
    int quantity = 1);
bool removeDragonProgressionItemForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view itemId,
    int quantity = 1);
bool equipDragonProgressionItem(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view characterId,
    std::string_view itemId,
    std::string* reason = nullptr);
bool equipDragonProgressionItemForProfile(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view itemId,
    std::string* reason = nullptr);
bool unequipDragonProgressionItemForProfile(
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view itemId);
bool isDragonProgressionItemEquippedForProfile(
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view itemId);

DragonProgressionAwardResult recordDragonProgressionMatch(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view characterId,
    std::string_view characterName,
    bool won,
    bool arenaMode);
DragonProgressionAwardResult recordDragonProgressionMatchForProfile(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view characterName,
    bool won,
    bool arenaMode);
DragonProgressionAwardResult recordDragonProgressionRewardForProfile(
    const DragonProgressionData& data,
    DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId,
    std::string_view characterName,
    int xp,
    int gold);
DragonEffectiveProgressionStats effectiveDragonProgressionStats(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view characterId);
DragonEffectiveProgressionStats effectiveDragonProgressionStatsForProfile(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId);
std::string dragonProgressionAwardSummary(const DragonProgressionAwardResult& result);
std::string dragonProgressionAwardSummaryWithGoldBalance(
    const DragonProgressionAwardResult& result,
    int goldBalance);
std::string dragonProgressionStatsSummary(const DragonEffectiveProgressionStats& stats);
std::string dragonProgressionCharacterSummary(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view characterId);
std::string dragonProgressionCharacterSummaryForProfile(
    const DragonProgressionData& data,
    const DragonProgressionSave& save,
    std::string_view profileId,
    std::string_view characterId);

} // namespace dragon
