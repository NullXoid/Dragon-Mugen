#include "VerificationScenario.h"

#include "dragon/DragonProgression.h"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>

namespace dragon::verification {
namespace {

enum class Status { Pass, Fail, Blocked };

struct Counts {
    int pass = 0;
    int fail = 0;
    int blocked = 0;
};

const char* statusText(Status status) {
    switch (status) {
    case Status::Pass:
        return "PASS";
    case Status::Fail:
        return "FAIL";
    case Status::Blocked:
    default:
        return "BLOCKED";
    }
}

void record(std::ostream& out, Counts& counts, Status status, std::string_view name, std::string_view detail) {
    out << statusText(status) << ' ' << name << "\n";
    if (!detail.empty()) {
        out << "  " << detail << "\n";
    }
    if (status == Status::Pass) {
        ++counts.pass;
    } else if (status == Status::Fail) {
        ++counts.fail;
    } else {
        ++counts.blocked;
    }
}

void summary(std::ostream& out, const Counts& counts) {
    out << "SUMMARY pass=" << counts.pass << " partial=0 fail=" << counts.fail
        << " blocked=" << counts.blocked << "\n";
}

int exitCode(const Counts& counts) {
    if (counts.fail > 0) return 1;
    if (counts.blocked > 0) return 2;
    return 0;
}

} // namespace

int runDragonProgressionLevelItems(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY dragon-progression-level-items\n";
    out << "root: " << runtime.rootText() << "\n";

    const std::filesystem::path root(runtime.rootText());
    DragonProgressionData data;
    try {
        data = loadDragonProgressionData(root);
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "load_progression_data", ex.what());
        summary(out, counts);
        return exitCode(counts);
    }

    record(out, counts, data.config.enabled ? Status::Pass : Status::Fail,
        "progression_enabled",
        "characters=" + std::to_string(data.characters.size()) + " items=" + std::to_string(data.items.size()));
    record(out, counts, findDragonProgressionItem(data, "training_weight") ? Status::Pass : Status::Fail,
        "default_item_loaded",
        "training_weight");

    DragonProgressionSave save;
    const std::string defaultProfileName = dragonProgressionProfileDisplayName(save);
    record(out, counts, !defaultProfileName.empty() ? Status::Pass : Status::Fail,
        "default_profile_available",
        defaultProfileName);

    auto firstAward = recordDragonProgressionMatch(data, save, "kfm", "Kung Fu Man", true, false);
    record(out, counts, firstAward.applied && firstAward.xpGained == data.config.winXp ? Status::Pass : Status::Fail,
        "win_xp_awarded",
        dragonProgressionAwardSummary(firstAward));

    auto secondAward = recordDragonProgressionMatch(data, save, "kfm", "Kung Fu Man", true, false);
    record(out, counts, secondAward.newLevel > firstAward.newLevel ? Status::Pass : Status::Fail,
        "level_up_after_enough_xp",
        dragonProgressionAwardSummary(secondAward));

    grantDragonProgressionItem(save, "training_weight", 1);
    std::string equipReason;
    const bool equippedTrainingWeight = equipDragonProgressionItem(data, save, "kfm", "training_weight", &equipReason);
    record(out, counts, equippedTrainingWeight ? Status::Pass : Status::Fail,
        "equip_owned_level_one_item",
        equipReason);

    grantDragonProgressionItem(save, "guard_charm", 1);
    const bool equippedGuardCharm = equipDragonProgressionItem(data, save, "kfm", "guard_charm", &equipReason);
    record(out, counts, equippedGuardCharm ? Status::Pass : Status::Fail,
        "equip_level_gated_item_after_level_up",
        equipReason);

    const auto stats = effectiveDragonProgressionStats(data, save, "kfm");
    const bool statsIncludeLevelsAndItem =
        stats.level >= 2
        && stats.lifeBonus > 0
        && stats.attackPermille > 1000
        && !stats.equippedItems.empty();
    record(out, counts, statsIncludeLevelsAndItem ? Status::Pass : Status::Fail,
        "effective_stats_include_level_and_items",
        "level=" + std::to_string(stats.level)
            + " life_bonus=" + std::to_string(stats.lifeBonus)
            + " attack_permille=" + std::to_string(stats.attackPermille)
            + " equipped=" + std::to_string(stats.equippedItems.size()));

    const DragonProgressionProfile& activeProfile = activeDragonProgressionProfile(save);
    const std::string firstProfileId = activeProfile.id;
    setActiveDragonProgressionProfile(save, "Verifier Two");
    const auto isolatedStats = effectiveDragonProgressionStats(data, save, "kfm");
    record(out, counts,
        isolatedStats.level == 1 && isolatedStats.xp == 0 && !firstProfileId.empty()
            ? Status::Pass
            : Status::Fail,
        "profile_switch_isolates_character_xp",
        dragonProgressionStatsSummary(isolatedStats));

    const auto secondProfileAward = recordDragonProgressionMatch(data, save, "kfm", "Kung Fu Man", false, false);
    const auto secondProfileStats = effectiveDragonProgressionStats(data, save, "kfm");
    record(out, counts,
        secondProfileAward.applied
            && secondProfileStats.level == 1
            && secondProfileStats.xp == data.config.lossXp
            ? Status::Pass
            : Status::Fail,
        "profile_specific_loss_xp",
        dragonProgressionAwardSummary(secondProfileAward));

    setActiveDragonProgressionProfile(save, firstProfileId);
    const auto restoredStats = effectiveDragonProgressionStats(data, save, "kfm");
    record(out, counts,
        restoredStats.level == stats.level
            && restoredStats.xp == stats.xp
            && restoredStats.attackPermille == stats.attackPermille
            ? Status::Pass
            : Status::Fail,
        "profile_restore_keeps_original_xp",
        dragonProgressionStatsSummary(restoredStats));

    record(out, counts,
        dragonProgressionCharacterSummary(data, save, "kfm").find("LV ") != std::string::npos
            ? Status::Pass
            : Status::Fail,
        "character_progression_display_summary",
        dragonProgressionCharacterSummary(data, save, "kfm"));

    const auto tempPath = std::filesystem::temp_directory_path() / "dragon_mugen_progression_verify.def";
    try {
        saveDragonProgressionSave(tempPath, save);
        const auto loaded = loadDragonProgressionSave(tempPath);
        const auto loadedStats = effectiveDragonProgressionStats(data, loaded, "kfm");
        record(out, counts,
            loadedStats.level == stats.level
                && loadedStats.attackPermille == stats.attackPermille
                && inventoryEntry(loaded, "training_weight").has_value()
                && inventoryEntry(loaded, "guard_charm").has_value()
                ? Status::Pass
                : Status::Fail,
            "save_round_trip",
            tempPath.string());
        std::filesystem::remove(tempPath);
    } catch (const std::exception& ex) {
        record(out, counts, Status::Fail, "save_round_trip", ex.what());
    }

    const auto legacyPath = std::filesystem::temp_directory_path() / "dragon_mugen_progression_legacy_verify.def";
    try {
        {
            std::ofstream legacy(legacyPath, std::ios::trunc);
            legacy << "[Dragon.Progression.Save]\nversion = 1\n\n";
            legacy << "[Character legacy_kfm]\nlevel = 3\nxp = 12\nvictories = 2\ndefeats = 1\nequipped = training_weight\n\n";
            legacy << "[Inventory]\ntraining_weight = 2\n";
        }
        const auto migrated = loadDragonProgressionSave(legacyPath);
        const auto migratedStats = effectiveDragonProgressionStats(data, migrated, "legacy_kfm");
        record(out, counts,
            migratedStats.level == 3
                && migratedStats.xp == 12
                && inventoryEntry(migrated, "training_weight").has_value()
                && !dragonProgressionProfileDisplayName(migrated).empty()
                ? Status::Pass
                : Status::Fail,
            "legacy_flat_save_migrates_to_profile",
            dragonProgressionProfileDisplayName(migrated) + " " + dragonProgressionStatsSummary(migratedStats));
        std::filesystem::remove(legacyPath);
    } catch (const std::exception& ex) {
        record(out, counts, Status::Fail, "legacy_flat_save_migrates_to_profile", ex.what());
    }

    summary(out, counts);
    return exitCode(counts);
}

int runDragonProgressionPlayerProfiles(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY dragon-progression-player-profiles\n";
    out << "root: " << runtime.rootText() << "\n";

    const std::filesystem::path root(runtime.rootText());
    DragonProgressionData data;
    try {
        data = loadDragonProgressionData(root);
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "load_progression_data", ex.what());
        summary(out, counts);
        return exitCode(counts);
    }

    DragonProgressionSave save;
    setDragonProgressionPlayerProfile(save, 0, "Verifier One");
    setDragonProgressionPlayerProfile(save, 1, dragonProgressionGuestProfileId());
    const std::string p1Id = dragonProgressionPlayerProfileId(save, 0);
    record(out, counts,
        !p1Id.empty() && isDragonProgressionGuestProfile(dragonProgressionPlayerProfileId(save, 1))
            ? Status::Pass
            : Status::Fail,
        "p2_defaults_to_guest_slot",
        "p1=" + p1Id + " p2=" + dragonProgressionPlayerProfileId(save, 1));

    const std::string p2Id = createNextDragonProgressionProfile(save, "Verifier");
    setDragonProgressionPlayerProfile(save, 1, p2Id);
    record(out, counts,
        !isDragonProgressionGuestProfile(p2Id)
            && dragonProgressionPlayerProfileId(save, 1) == p2Id
            && dragonProgressionPlayerProfileId(save, 0) != dragonProgressionPlayerProfileId(save, 1)
            ? Status::Pass
            : Status::Fail,
        "p2_can_select_real_profile",
        "p1=" + dragonProgressionPlayerProfileId(save, 0)
            + " p2=" + dragonProgressionPlayerProfileId(save, 1));

    setDragonProgressionPlayerProfile(save, 1, dragonProgressionPlayerProfileId(save, 0));
    record(out, counts,
        isDragonProgressionGuestProfile(dragonProgressionPlayerProfileId(save, 1))
            ? Status::Pass
            : Status::Fail,
        "duplicate_real_profile_reverts_p2_to_guest",
        dragonProgressionPlayerProfileId(save, 1));
    setDragonProgressionPlayerProfile(save, 1, p2Id);

    const auto p1Award = recordDragonProgressionMatchForProfile(
        data,
        save,
        dragonProgressionPlayerProfileId(save, 0),
        "kfm",
        "Kung Fu Man",
        true,
        false);
    const auto p2Award = recordDragonProgressionMatchForProfile(
        data,
        save,
        dragonProgressionPlayerProfileId(save, 1),
        "kfm",
        "Kung Fu Man",
        false,
        false);
    const auto p1Stats = effectiveDragonProgressionStatsForProfile(data, save, p1Id, "kfm");
    const auto p2Stats = effectiveDragonProgressionStatsForProfile(data, save, p2Id, "kfm");
    record(out, counts,
        p1Award.applied
            && p2Award.applied
            && p1Stats.xp == data.config.winXp
            && p2Stats.xp == data.config.lossXp
            && p1Stats.xp != p2Stats.xp
            ? Status::Pass
            : Status::Fail,
        "same_character_xp_is_profile_scoped",
        "p1=" + dragonProgressionStatsSummary(p1Stats)
            + " p2=" + dragonProgressionStatsSummary(p2Stats));

    const auto guestAward = recordDragonProgressionMatchForProfile(
        data,
        save,
        dragonProgressionGuestProfileId(),
        "kfm",
        "Kung Fu Man",
        true,
        false);
    record(out, counts,
        !guestAward.applied && findDragonProgressionProfile(save, dragonProgressionGuestProfileId()) == nullptr
            ? Status::Pass
            : Status::Fail,
        "guest_profile_does_not_persist_xp",
        dragonProgressionAwardSummary(guestAward));

    setDragonProgressionPlayerProfile(save, 1, dragonProgressionGuestProfileId());
    cycleDragonProgressionPlayerProfile(save, 1, 1);
    record(out, counts,
        dragonProgressionPlayerProfileId(save, 1) == p2Id
            ? Status::Pass
            : Status::Fail,
        "p2_cycle_off_guest_selects_real_profile",
        dragonProgressionPlayerProfileId(save, 1));

    const auto tempPath = std::filesystem::temp_directory_path() / "dragon_mugen_player_profiles_verify.def";
    try {
        saveDragonProgressionSave(tempPath, save);
        const auto loaded = loadDragonProgressionSave(tempPath);
        const auto loadedP1Stats = effectiveDragonProgressionStatsForProfile(data, loaded, p1Id, "kfm");
        const auto loadedP2Stats = effectiveDragonProgressionStatsForProfile(data, loaded, p2Id, "kfm");
        record(out, counts,
            dragonProgressionPlayerProfileId(loaded, 0) == p1Id
                && dragonProgressionPlayerProfileId(loaded, 1) == p2Id
                && loadedP1Stats.xp == data.config.winXp
                && loadedP2Stats.xp == data.config.lossXp
                ? Status::Pass
                : Status::Fail,
            "player_profile_slots_round_trip",
            tempPath.string());
        std::filesystem::remove(tempPath);
    } catch (const std::exception& ex) {
        record(out, counts, Status::Fail, "player_profile_slots_round_trip", ex.what());
    }

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
