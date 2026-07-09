#include "VerificationScenario.h"

#include "AppTypes.h"
#include "VerificationStoryDepthWalkChecks.h"
#include "VerificationStoryEnemyRoles.h"
#include "dragon/Sff.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <exception>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace dragon::verification {
namespace {

enum class Status {
    Pass,
    Fail,
    Blocked,
};

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

int exitCode(const Counts& counts) {
    if (counts.fail > 0) return 1;
    if (counts.blocked > 0) return 2;
    return 0;
}

void summary(std::ostream& out, const Counts& counts) {
    out << "SUMMARY pass=" << counts.pass << " partial=0 fail=" << counts.fail
        << " blocked=" << counts.blocked << "\n";
}

void header(std::ostream& out, RuntimeProbe& runtime, std::string_view scenario) {
    out << "VERIFY " << scenario << "\n" << "root: " << runtime.rootText() << "\n"
        << "stage: " << runtime.stageName() << "\n" << "p1: " << runtime.p1Name() << "\n";
}

bool waitForActiveFight(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight)) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight);
}

bool waitForMatchResult(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::MatchResult)) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::MatchResult);
}

bool setupStory(RuntimeProbe& runtime, std::ostream& out, Counts& counts, std::string_view scenario, std::string_view stageHint = "") {
    if (!runtime.setup("A.Ben", stageHint, ScenarioMode::Story, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Story setup failed");
        summary(out, counts);
        return false;
    }
    header(out, runtime, scenario);
    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "story_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " pending_mode=" + std::to_string(runtime.snapshot().pendingMode));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "story_checks", "Story fight phase was not active");
        summary(out, counts);
        return false;
    }
    return true;
}

bool decodedSpriteHasVisibleColor(const DecodedSprite& decoded) {
    for (size_t i = 0; i + 3 < decoded.rgba.size(); i += 4) {
        const bool visible = decoded.rgba[i + 3] > 0;
        const bool colored = decoded.rgba[i + 0] != 0 || decoded.rgba[i + 1] != 0 || decoded.rgba[i + 2] != 0;
        if (visible && colored) {
            return true;
        }
    }
    return false;
}

void clearCurrentWave(RuntimeProbe& runtime) {
    const int active = runtime.snapshot().storyActiveEnemies;
    for (int i = 1; i <= active; ++i) {
        runtime.setFighterLife(i, 0);
    }
    runtime.step({}, 70);
}

int expectedStoryEnemyTotalForDifficulty(StoryDifficulty difficulty) {
    if (difficulty == StoryDifficulty::Easy) return 1;
    if (difficulty == StoryDifficulty::Hard) return 7;
    return 3;
}

void captureOptionalScreenshot(
    RuntimeProbe& runtime,
    std::ostream& out,
    Counts& counts,
    const char* envName,
    std::string_view checkName) {
    const char* screenshotPath = std::getenv(envName);
    if (!screenshotPath || !*screenshotPath) {
        return;
    }
    const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
    record(out, counts, captured ? Status::Pass : Status::Fail, checkName, screenshotPath);
}

} // namespace

int runStoryModeMenuRoute(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-mode-menu-route")) {
        return exitCode(counts);
    }
    const auto snapshot = runtime.snapshot();
    record(out, counts, snapshot.pendingMode == static_cast<int>(PendingMode::Story) ? Status::Pass : Status::Fail,
        "story_pending_mode",
        "pending_mode=" + std::to_string(snapshot.pendingMode));
    record(out, counts, snapshot.fighterCount == 4 && snapshot.arenaRuntimeCount == 4 ? Status::Pass : Status::Fail,
        "story_preloads_player_and_three_enemy_runtimes",
        "fighters=" + std::to_string(snapshot.fighterCount)
        + " runtimes=" + std::to_string(snapshot.arenaRuntimeCount));
    const int expectedTotal = expectedStoryEnemyTotalForDifficulty(static_cast<StoryDifficulty>(snapshot.storyDifficulty));
    record(out, counts, snapshot.storyActiveEnemies == 1 && snapshot.storyTotalEnemies == expectedTotal ? Status::Pass : Status::Fail,
        "story_initial_wave_state",
        "wave=" + std::to_string(snapshot.storyWaveIndex)
        + " active=" + std::to_string(snapshot.storyActiveEnemies)
        + " total=" + std::to_string(snapshot.storyTotalEnemies)
        + " board_waves=" + std::to_string(snapshot.storySelectedBoardWaves));
    StoryBoardRoute parsedRoute;
    try {
        parsedRoute = loadStoryBoardRouteFile(std::filesystem::path(runtime.rootText()) / "data" / "story_boards.def");
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "story_route_role_enemy_file_loads", ex.what());
    }
    const auto roleNode = std::find_if(parsedRoute.nodes.begin(), parsedRoute.nodes.end(), [](const StoryBoardNode& node) {
        return node.kind == StoryBoardNodeKind::SideScroller;
    });
    const bool roleRefsConfigured = storyRouteEnemySetupConfigured(
        parsedRoute,
        roleNode != parsedRoute.nodes.end() ? &*roleNode : nullptr);
    record(out, counts,
        roleRefsConfigured ? Status::Pass : Status::Fail,
        "story_route_enemy_roles_configured",
        storyRouteEnemySetupDetail(parsedRoute, roleNode != parsedRoute.nodes.end() ? &*roleNode : nullptr));
    record(out, counts,
        characterRefMatchesSnapshot(snapshot.p2CharacterId, snapshot.p2CharacterName, firstRoleRef(parsedRoute.enemySetup.grunts))
            ? Status::Pass
            : Status::Fail,
        "story_first_wave_uses_grunt_role",
        "p2_id=" + snapshot.p2CharacterId + " p2_name=" + snapshot.p2CharacterName);
    summary(out, counts);
    return exitCode(counts);
}

int runStoryStageSelectMap(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-stage-select-map\n";
    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::Story, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Story stage-select setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto initial = runtime.snapshot();
    record(out, counts,
        initial.screen == static_cast<int>(Screen::StageSelect)
            && initial.pendingMode == static_cast<int>(PendingMode::Story)
            ? Status::Pass
            : Status::Fail,
        "story_stage_select_screen",
        "screen=" + std::to_string(initial.screen)
        + " pending=" + std::to_string(initial.pendingMode));
    record(out, counts, initial.storyBoardNodeCount >= 3 ? Status::Pass : Status::Fail,
        "story_board_card_count",
        "boards=" + std::to_string(initial.storyBoardNodeCount)
            + " stages=" + std::to_string(initial.stageCount));
    record(out, counts, initial.selectedStageLegacyOpenBorSection ? Status::Pass : Status::Fail,
        "story_stage_map_defaults_to_openbor",
        "stage=\"" + runtime.stageName() + "\" index=" + std::to_string(initial.selectedStageIndex)
            + " board=\"" + initial.storySelectedBoardTitle + "\"");
    captureOptionalScreenshot(
        runtime,
        out,
        counts,
        "DRAGON_STORY_STAGE_SELECT_SCREENSHOT",
        "story_stage_select_screenshot");

    if (const char* characterSelectPath = std::getenv("DRAGON_STORY_CHARACTER_SELECT_SCREENSHOT");
        characterSelectPath && *characterSelectPath) {
        runtime.pressKey("escape");
        const auto characterSelect = runtime.snapshot();
        record(out, counts,
            characterSelect.screen == static_cast<int>(Screen::CharacterSelect)
                ? Status::Pass
                : Status::Fail,
            "story_returns_to_character_select_for_screenshot",
            "screen=" + std::to_string(characterSelect.screen));
        const bool captured = runtime.captureScreenshot(std::filesystem::path(characterSelectPath));
        record(out, counts, captured ? Status::Pass : Status::Fail, "story_character_select_screenshot", characterSelectPath);
        runtime.pressKey("enter");
        const auto backToStage = runtime.snapshot();
        record(out, counts,
            backToStage.screen == static_cast<int>(Screen::StageSelect)
                ? Status::Pass
                : Status::Fail,
            "story_returns_to_stage_select_after_screenshot",
            "screen=" + std::to_string(backToStage.screen));
    }

    const std::string defaultStage = runtime.stageName();
    runtime.pressKey("right");
    const auto afterRight = runtime.snapshot();
    record(out, counts,
        initial.storyBoardNodeCount > 1 && afterRight.storySelectedBoardNode != initial.storySelectedBoardNode
            ? Status::Pass
            : Status::Fail,
        "story_board_map_cycles_right",
        "before=" + std::to_string(initial.storySelectedBoardNode)
        + " after=" + std::to_string(afterRight.storySelectedBoardNode)
        + " stage=\"" + runtime.stageName() + "\"");
    runtime.pressKey("left");
    const auto afterLeft = runtime.snapshot();
    record(out, counts,
        afterLeft.storySelectedBoardNode == initial.storySelectedBoardNode && runtime.stageName() == defaultStage
            ? Status::Pass
            : Status::Fail,
        "story_board_map_cycles_left",
        "board=" + std::to_string(afterLeft.storySelectedBoardNode)
        + " index=" + std::to_string(afterLeft.selectedStageIndex)
        + " stage=\"" + runtime.stageName() + "\"");

    runtime.pressKey("enter");
    const auto afterEnter = runtime.snapshot();
    record(out, counts,
        afterEnter.screen == static_cast<int>(Screen::VersusScreen)
            && afterEnter.loadingProgressActive
            && !afterEnter.loadingProgressFailed
            ? Status::Pass
            : Status::Fail,
        "story_stage_map_enters_loading",
        "screen=" + std::to_string(afterEnter.screen)
        + " loading=" + std::to_string(afterEnter.loadingProgressActive ? 1 : 0)
        + " phase=\"" + afterEnter.loadingProgressPhase + "\"");
    captureOptionalScreenshot(
        runtime,
        out,
        counts,
        "DRAGON_STORY_LOADING_SCREENSHOT",
        "story_loading_screenshot");

    summary(out, counts);
    return exitCode(counts);
}

int runStoryDifficultyEnemyScaling(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-difficulty-enemy-scaling\n";

    StoryBoardRoute parsedRoute;
    try {
        parsedRoute = loadStoryBoardRouteFile(std::filesystem::path(runtime.rootText()) / "data" / "story_boards.def");
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "story_role_enemy_route_loads", ex.what());
    }
    const auto roleNode = std::find_if(parsedRoute.nodes.begin(), parsedRoute.nodes.end(), [](const StoryBoardNode& node) {
        return node.kind == StoryBoardNodeKind::SideScroller;
    });
    const bool roleRefsConfigured = storyRouteEnemySetupConfigured(
        parsedRoute,
        roleNode != parsedRoute.nodes.end() ? &*roleNode : nullptr);
    record(out, counts, roleRefsConfigured ? Status::Pass : Status::Fail,
        "story_enemy_roles_configured",
        storyRouteEnemySetupDetail(parsedRoute, roleNode != parsedRoute.nodes.end() ? &*roleNode : nullptr));

    if (!runtime.setup("A.Ben", "TMNT OpenBOR Street", ScenarioMode::Story, out, 1)) {
        record(out, counts, Status::Blocked, "setup_medium_story", "Story setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    waitForActiveFight(runtime, 420);
    const auto medium = runtime.snapshot();
    record(out, counts,
        characterRefMatchesSnapshot(medium.p2CharacterId, medium.p2CharacterName, firstRoleRef(parsedRoute.enemySetup.grunts))
            ? Status::Pass
            : Status::Fail,
        "story_medium_first_wave_uses_grunt_role",
        "p2_id=" + medium.p2CharacterId + " p2_name=" + medium.p2CharacterName);

    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::Story, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Story stage-select setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    const auto initial = runtime.snapshot();
    record(out, counts, initial.storyDifficulty == 1 ? Status::Pass : Status::Fail,
        "story_difficulty_defaults_medium", "difficulty=" + std::to_string(initial.storyDifficulty));
    record(out, counts, initial.storySelectedBoardWaves == 3 ? Status::Pass : Status::Fail,
        "story_medium_uses_three_wave_plan", "waves=" + std::to_string(initial.storySelectedBoardWaves));

    runtime.pressKey("up");
    const auto hardSelect = runtime.snapshot();
    record(out, counts, hardSelect.storyDifficulty == 2 ? Status::Pass : Status::Fail,
        "story_difficulty_cycles_on_map", "difficulty=" + std::to_string(hardSelect.storyDifficulty)
            + " stage_index=" + std::to_string(hardSelect.selectedStageIndex));
    record(out, counts, hardSelect.storySelectedBoardWaves == 5 ? Status::Pass : Status::Fail,
        "story_hard_uses_five_wave_plan", "waves=" + std::to_string(hardSelect.storySelectedBoardWaves));

    runtime.pressKey("down");
    runtime.pressKey("down");
    const auto easySelect = runtime.snapshot();
    record(out, counts, easySelect.storyDifficulty == 0 && easySelect.storySelectedBoardWaves == 1 ? Status::Pass : Status::Fail,
        "story_easy_uses_one_boss_wave",
        "difficulty=" + std::to_string(easySelect.storyDifficulty)
            + " waves=" + std::to_string(easySelect.storySelectedBoardWaves));
    runtime.pressKey("up");
    runtime.pressKey("up");

    runtime.pressKey("enter");
    runtime.pressKey("enter");
    const bool activeFight = waitForActiveFight(runtime, 520);
    const auto hard = runtime.snapshot();
    record(out, counts, activeFight && hard.pendingMode == static_cast<int>(PendingMode::Story) ? Status::Pass : Status::Fail,
        "hard_story_fight_starts", "phase=" + std::to_string(hard.matchPhase) + " difficulty=" + std::to_string(hard.storyDifficulty));
    record(out, counts, hard.p1.maxLife == medium.p1.maxLife ? Status::Pass : Status::Fail,
        "story_difficulty_does_not_scale_player_level", "medium_p1_max=" + std::to_string(medium.p1.maxLife) + " hard_p1_max=" + std::to_string(hard.p1.maxLife));
    record(out, counts, hard.p2.maxLife > medium.p2.maxLife && hard.p2.life == hard.p2.maxLife ? Status::Pass : Status::Fail,
        "story_difficulty_scales_enemy_life", "medium_enemy_max=" + std::to_string(medium.p2.maxLife) + " hard_enemy_max=" + std::to_string(hard.p2.maxLife) + " hard_enemy_life=" + std::to_string(hard.p2.life));

    summary(out, counts);
    return exitCode(counts);
}

int runStoryOpenBorStageDefault(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-openbor-stage-default")) {
        return exitCode(counts);
    }
    const auto snapshot = runtime.snapshot();
    record(out, counts, snapshot.selectedStageLegacyOpenBorSection ? Status::Pass : Status::Fail,
        "story_defaults_to_openbor_stage",
        "stage=\"" + runtime.stageName() + "\" camera=" + std::to_string(snapshot.cameraX));
    record(out, counts, snapshot.arenaZAxisEnabled && std::fabs(snapshot.p1.depthZ) <= 0.5f ? Status::Pass : Status::Fail,
        "story_depth_projection_enabled",
        "depth_active=" + std::to_string(snapshot.arenaZAxisEnabled ? 1 : 0)
        + " p1_depth=" + std::to_string(snapshot.p1.depthZ));

    runtime.setArenaCpuFrozen(true);
    const auto depthProbe = probeStoryDepthWalkAnimation(runtime);
    record(out, counts, depthProbe.idle ? Status::Pass : Status::Fail,
        "story_controllable_idle_ready",
        "state=" + std::to_string(depthProbe.idleState)
            + " ctrl=" + std::to_string(depthProbe.idleCtrl ? 1 : 0)
            + " p1_id=" + depthProbe.p1CharacterId
            + " p1_name=" + depthProbe.p1CharacterName);
    if (!depthProbe.idle) {
        record(out, counts, Status::Blocked, "story_depth_walk_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }
    record(out, counts, depthProbe.forwardPass ? Status::Pass : Status::Fail,
        "story_depth_walk_animates_without_modifier", depthProbe.forwardDetail);
    record(out, counts, depthProbe.reversePass ? Status::Pass : Status::Fail,
        "story_depth_walk_animates_reverse_without_modifier", depthProbe.reverseDetail);
    summary(out, counts);
    return exitCode(counts);
}

int runStoryStageBoardExpansion(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-stage-board-expansion\n";
    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::Story, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Story stage-select setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto initial = runtime.snapshot();
    record(out, counts, initial.storyBoardNodeCount >= 5 ? Status::Pass : Status::Fail,
        "story_route_board_count",
        "boards=" + std::to_string(initial.storyBoardNodeCount)
            + " stages=" + std::to_string(initial.stageCount));

    bool sawSewer = false;
    bool sawComic = false;
    bool sawMusic = false;
    std::string musicStageName;
    std::string musicPath;
    bool sawShop = false;
    bool sawBoss = false;
    const int cycles = std::max(1, initial.storyBoardNodeCount);
    for (int i = 0; i < cycles; ++i) {
        const std::string stageName = runtime.stageName();
        const auto snap = runtime.snapshot();
        sawSewer = sawSewer || storyVerifierContainsNoCase(stageName, "sewer");
        sawComic = sawComic || storyVerifierContainsNoCase(stageName, "comic");
        sawShop = sawShop || snap.storySelectedBoardShop;
        sawBoss = sawBoss || snap.storySelectedBoardKind == "arena_boss";
        if (!sawMusic && snap.selectedStageHasMusic) {
            sawMusic = true;
            musicStageName = stageName;
            musicPath = snap.selectedStageMusicPath;
        }
        runtime.pressKey("right");
    }

    record(out, counts, sawSewer ? Status::Pass : Status::Fail,
        "story_stage_includes_sewer_board",
        sawSewer ? "found" : "missing");
    record(out, counts, sawComic ? Status::Pass : Status::Fail,
        "story_stage_includes_comic_board",
        sawComic ? "found" : "missing");
    record(out, counts, sawShop ? Status::Pass : Status::Fail,
        "story_route_includes_shop_door",
        sawShop ? "found" : "missing");
    record(out, counts, sawBoss ? Status::Pass : Status::Fail,
        "story_route_includes_arena_boss",
        sawBoss ? "found" : "missing");
    record(out, counts, !sawMusic || !musicPath.empty() ? Status::Pass : Status::Fail,
        "story_stage_music_metadata",
        sawMusic
            ? "stage=\"" + musicStageName + "\" music=\"" + musicPath + "\""
            : "no routed story board declares music");

    if (sawMusic) {
        if (!runtime.setup("A.Ben", musicStageName, ScenarioMode::Story, out, 1)) {
            record(out, counts, Status::Blocked, "story_stage_music_starts", "Story music-board setup failed");
            summary(out, counts);
            return exitCode(counts);
        }
        waitForActiveFight(runtime, 420);
        runtime.step({}, 20);
        const auto playing = runtime.snapshot();
        record(out, counts, playing.activeSounds > 0 ? Status::Pass : Status::Fail,
            "story_stage_music_starts",
            "active_sounds=" + std::to_string(playing.activeSounds)
                + " screen=" + std::to_string(playing.screen)
                + " phase=" + std::to_string(playing.matchPhase));
        record(out, counts, playing.stageBackgroundCount > 0 ? Status::Pass : Status::Fail,
            "story_stage_music_background_loaded",
            "backgrounds=" + std::to_string(playing.stageBackgroundCount));
        try {
            const auto archive = loadSffArchive(std::filesystem::path(runtime.rootText()) / "stages" / "tmnt_openbor_street.sff");
            const auto* sprite = findSprite(archive, 0, 0);
            const auto decoded = sprite ? decodeSffSprite(archive, *sprite) : std::optional<DecodedSprite>{};
            record(out, counts, decoded && decodedSpriteHasVisibleColor(*decoded) ? Status::Pass : Status::Fail,
                "story_stage_music_background_visible",
                decoded ? "size=" + std::to_string(decoded->width) + "x" + std::to_string(decoded->height) : "decode failed");
        } catch (const std::exception& ex) {
            record(out, counts, Status::Fail, "story_stage_music_background_visible", ex.what());
        }
        if (const char* screenshotPath = std::getenv("DRAGON_SCREENSHOT_PATH"); screenshotPath && *screenshotPath) {
            const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
            record(out, counts, captured ? Status::Pass : Status::Fail, "screenshot_captured", screenshotPath);
        }
    } else {
        record(out, counts, Status::Pass, "story_stage_music_starts", "no routed story board declares music");
    }

    summary(out, counts);
    return exitCode(counts);
}

int runStoryBoardRoutePlan(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-board-route-plan\n";
    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::Story, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Story stage-select setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const std::filesystem::path root(runtime.rootText());
    const std::filesystem::path routePath = root / "data" / "story_boards.def";
    StoryBoardRoute parsedRoute;
    try {
        parsedRoute = loadStoryBoardRouteFile(routePath);
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "route_def_loads", ex.what());
    }
    bool sawConfigurableShop = false;
    bool sawConfigurableWave = false;
    for (const StoryBoardNode& node : parsedRoute.nodes) {
        sawConfigurableShop = sawConfigurableShop
            || (node.kind == StoryBoardNodeKind::Shop
                && node.shopDoorPrompt == "LK / X SHOP"
                && node.shopDoorOffsetX == 160.0f
                && node.shopDoorRadiusX == 56.0f);
        sawConfigurableWave = sawConfigurableWave || (!node.waveSpecs.empty() && !node.waveSpecs.front().enemies.empty()
            && node.rewardXp > 0 && node.rewardGold > 0 && !node.clearCueImagePath.empty());
    }
    record(out, counts,
        parsedRoute.forwardCueImagePath == "data/story/wave_clear_arrow.png" && sawConfigurableShop && sawConfigurableWave
            ? Status::Pass
            : Status::Fail,
        "route_def_exposes_mugen_style_story_settings",
        "cue=" + parsedRoute.forwardCueImagePath + " shop_config=" + std::to_string(sawConfigurableShop ? 1 : 0) + " wave_config=" + std::to_string(sawConfigurableWave ? 1 : 0));

    const auto initial = runtime.snapshot();
    record(out, counts, initial.storyBoardNodeCount >= 5 ? Status::Pass : Status::Fail,
        "route_has_multiple_board_nodes",
        "boards=" + std::to_string(initial.storyBoardNodeCount));
    record(out, counts,
        initial.storySelectedBoardKind == "side_scroller" && initial.storySelectedBoardWaves == 3
            ? Status::Pass
            : Status::Fail,
        "route_defaults_to_side_scroller",
        "kind=" + initial.storySelectedBoardKind
            + " waves=" + std::to_string(initial.storySelectedBoardWaves)
            + " title=\"" + initial.storySelectedBoardTitle + "\"");

    bool sawMidBoss = false;
    bool sawShop = false;
    bool sawArenaBoss = false;
    int shopBoard = -1;
    for (int i = 0; i < std::max(1, initial.storyBoardNodeCount); ++i) {
        const auto snap = runtime.snapshot();
        sawMidBoss = sawMidBoss || snap.storySelectedBoardKind == "mid_boss";
        sawArenaBoss = sawArenaBoss || snap.storySelectedBoardKind == "arena_boss";
        if (snap.storySelectedBoardShop) {
            sawShop = true;
            shopBoard = snap.storySelectedBoardNode;
        }
        runtime.pressKey("right");
    }
    record(out, counts, sawMidBoss ? Status::Pass : Status::Fail,
        "route_includes_midboss",
        sawMidBoss ? "found" : "missing");
    record(out, counts, sawArenaBoss ? Status::Pass : Status::Fail,
        "route_includes_arena_boss",
        sawArenaBoss ? "found" : "missing");
    record(out, counts, sawShop && shopBoard >= 0 ? Status::Pass : Status::Fail,
        "route_includes_shop_door",
        "shop_board=" + std::to_string(shopBoard));

    if (shopBoard >= 0) {
        for (int i = 0; i < initial.storyBoardNodeCount + 1; ++i) {
            if (runtime.snapshot().storySelectedBoardNode == shopBoard) {
                break;
            }
            runtime.pressKey("right");
        }
        const auto shop = runtime.snapshot();
        runtime.pressKey("enter");
        const auto afterEnter = runtime.snapshot();
        record(out, counts,
            shop.storySelectedBoardShop && afterEnter.screen == static_cast<int>(Screen::ShopDemo)
                ? Status::Pass
                : Status::Fail,
            "shop_board_enters_shop_hub",
            "before_kind=" + shop.storySelectedBoardKind
                + " after_screen=" + std::to_string(afterEnter.screen));
    } else {
        record(out, counts, Status::Blocked, "shop_board_enters_shop_hub", "shop board missing");
    }

    summary(out, counts);
    return exitCode(counts);
}

int runStoryWaveClearForwardCue(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-wave-clear-forward-cue", "TMNT OpenBOR Street")) {
        return exitCode(counts);
    }
    runtime.setArenaCpuFrozen(true);
    const auto before = runtime.snapshot();
    runtime.setFighterLife(1, 0);
    runtime.step({}, 6);
    const auto afterClear = runtime.snapshot();
    record(out, counts,
        !before.storyForwardCueVisible && afterClear.storyForwardCueVisible
            ? Status::Pass
            : Status::Fail,
        "forward_arrow_appears_after_wave_clear",
        "before=" + std::to_string(before.storyForwardCueVisible ? 1 : 0)
            + " after=" + std::to_string(afterClear.storyForwardCueVisible ? 1 : 0)
            + " wave=" + std::to_string(afterClear.storyWaveIndex)
            + " living=" + std::to_string(afterClear.storyLivingEnemies));
    record(out, counts, Status::Pass,
        "forward_arrow_custom_image_optional",
        afterClear.storyForwardCueImageLoaded
            ? "data/story/wave_clear_arrow.png loaded"
            : "custom image absent; code-drawn fallback active");
    runtime.step({}, 70);
    const auto nextWave = runtime.snapshot();
    record(out, counts,
        !nextWave.storyForwardCueVisible && nextWave.storyLivingEnemies > 0
            ? Status::Pass
            : Status::Fail,
        "forward_arrow_hides_when_next_wave_spawns",
        "visible=" + std::to_string(nextWave.storyForwardCueVisible ? 1 : 0)
            + " wave=" + std::to_string(nextWave.storyWaveIndex)
            + " living=" + std::to_string(nextWave.storyLivingEnemies));
    summary(out, counts);
    return exitCode(counts);
}

int runStoryShopDoorTrigger(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-shop-door-trigger\n";
    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::Story, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Story stage-select setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto initial = runtime.snapshot();
    int shopBoard = -1;
    for (int i = 0; i < std::max(1, initial.storyBoardNodeCount); ++i) {
        const auto snap = runtime.snapshot();
        if (snap.storySelectedBoardShop) {
            shopBoard = snap.storySelectedBoardNode;
            break;
        }
        runtime.pressKey("right");
    }
    record(out, counts,
        shopBoard > 0 ? Status::Pass : Status::Blocked,
        "route_has_preceding_shop_door_board",
        "shop_board=" + std::to_string(shopBoard));
    if (shopBoard <= 0) {
        summary(out, counts);
        return exitCode(counts);
    }

    const int doorBoard = shopBoard - 1;
    for (int i = 0; i < initial.storyBoardNodeCount + 1; ++i) {
        if (runtime.snapshot().storySelectedBoardNode == doorBoard) {
            break;
        }
        runtime.pressKey("right");
    }
    const auto selectedDoorBoard = runtime.snapshot();
    runtime.pressKey("enter");
    const bool activeFight = waitForActiveFight(runtime, 520);
    const auto fight = runtime.snapshot();
    record(out, counts,
        activeFight && fight.storyActiveBoardNode == doorBoard
            ? Status::Pass
            : Status::Fail,
        "door_board_fight_started",
        "selected=\"" + selectedDoorBoard.storySelectedBoardTitle
            + "\" active=" + std::to_string(fight.storyActiveBoardNode)
            + " phase=" + std::to_string(fight.matchPhase));
    if (!activeFight) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setArenaCpuFrozen(true);
    clearCurrentWave(runtime);
    const auto afterClear = runtime.snapshot();
    record(out, counts,
        afterClear.storyShopDoorAvailable && !afterClear.matchComplete
            ? Status::Pass
            : Status::Fail,
        "shop_door_opens_after_board_clear",
        "available=" + std::to_string(afterClear.storyShopDoorAvailable ? 1 : 0)
            + " match_complete=" + std::to_string(afterClear.matchComplete ? 1 : 0)
            + " door_x=" + std::to_string(afterClear.storyShopDoorX));

    runtime.setFighterDepth(0, 0.0f);
    RuntimeSnapshot atDoor;
    for (int frame = 0; frame < 1600; ++frame) {
        runtime.step(SymbolicInput{ .right = true }, 1);
        atDoor = runtime.snapshot();
        if (atDoor.storyShopDoorPromptVisible) {
            break;
        }
    }
    record(out, counts,
        atDoor.storyShopDoorPromptVisible
            ? Status::Pass
            : Status::Fail,
        "shop_door_prompt_visible_in_trigger",
        "prompt=" + std::to_string(atDoor.storyShopDoorPromptVisible ? 1 : 0)
            + " p1_x=" + std::to_string(atDoor.p1.x)
            + " door_x=" + std::to_string(atDoor.storyShopDoorX));

    runtime.step(SymbolicInput{ .a = true }, 1);
    const auto afterEnter = runtime.snapshot();
    record(out, counts,
        afterEnter.screen == static_cast<int>(Screen::ShopDemo)
            ? Status::Pass
            : Status::Fail,
        "lk_x_enters_shop_demo",
        "screen=" + std::to_string(afterEnter.screen)
            + " pending=" + std::to_string(afterEnter.storyShopDoorTransitionPending ? 1 : 0));

    summary(out, counts);
    return exitCode(counts);
}

int runStoryShopRouteResume(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-shop-route-resume\n";
    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::Story, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Story stage-select setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto initial = runtime.snapshot();
    int shopBoard = -1;
    for (int i = 0; i < std::max(1, initial.storyBoardNodeCount); ++i) {
        const auto snap = runtime.snapshot();
        if (snap.storySelectedBoardShop) {
            shopBoard = snap.storySelectedBoardNode;
            break;
        }
        runtime.pressKey("right");
    }
    if (shopBoard <= 0) {
        record(out, counts, Status::Blocked, "route_has_shop_resume_target", "shop_board=" + std::to_string(shopBoard));
        summary(out, counts);
        return exitCode(counts);
    }
    const int expectedNextBoard = shopBoard + 1;
    for (int i = 0; i < initial.storyBoardNodeCount + 1; ++i) {
        if (runtime.snapshot().storySelectedBoardNode == shopBoard - 1) break;
        runtime.pressKey("right");
    }
    runtime.pressKey("enter");
    if (!waitForActiveFight(runtime, 520)) {
        record(out, counts, Status::Blocked, "door_board_fight_started", "fight did not start");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setArenaCpuFrozen(true);
    clearCurrentWave(runtime);
    const auto afterClear = runtime.snapshot();
    record(out, counts, afterClear.storyShopDoorAvailable ? Status::Pass : Status::Fail,
        "route_shop_door_available",
        "door_x=" + std::to_string(afterClear.storyShopDoorX));

    for (int frame = 0; frame < 1600 && !runtime.snapshot().storyShopDoorPromptVisible; ++frame) {
        runtime.step(SymbolicInput{ .right = true }, 1);
    }
    runtime.step(SymbolicInput{ .a = true }, 1);
    const auto inShop = runtime.snapshot();
    record(out, counts,
        inShop.screen == static_cast<int>(Screen::ShopDemo)
            && inShop.storyResumeRouteAfterShop
            && inShop.storyResumeBoardNodeAfterShop == expectedNextBoard
            ? Status::Pass
            : Status::Fail,
        "route_shop_records_resume_target",
        "screen=" + std::to_string(inShop.screen)
            + " resume=" + std::to_string(inShop.storyResumeRouteAfterShop ? 1 : 0)
            + " target=" + std::to_string(inShop.storyResumeBoardNodeAfterShop)
            + " expected=" + std::to_string(expectedNextBoard));

    runtime.pressKey("escape");
    const auto afterExit = runtime.snapshot();
    record(out, counts,
        afterExit.screen == static_cast<int>(Screen::VersusScreen)
            && afterExit.loadingProgressActive
            && afterExit.storySelectedBoardNode == expectedNextBoard
            && afterExit.storyActiveBoardNode == expectedNextBoard
            ? Status::Pass
            : Status::Fail,
        "shop_exit_starts_next_story_board",
        "screen=" + std::to_string(afterExit.screen)
            + " loading=" + std::to_string(afterExit.loadingProgressActive ? 1 : 0)
            + " selected=" + std::to_string(afterExit.storySelectedBoardNode)
            + " active=" + std::to_string(afterExit.storyActiveBoardNode));
    summary(out, counts);
    return exitCode(counts);
}

int runStoryWaveSpawnScroll(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    StoryBoardRoute parsedRoute;
    try {
        parsedRoute = loadStoryBoardRouteFile(std::filesystem::path(runtime.rootText()) / "data" / "story_boards.def");
    } catch (const std::exception& ex) {
        record(out, counts, Status::Blocked, "story_wave_role_route_loads", ex.what());
    }
    const std::string expectedMiniBoss = firstRoleRef(parsedRoute.enemySetup.miniBosses);
    const std::string expectedBoss = firstRoleRef(parsedRoute.enemySetup.bosses);

    if (!setupStory(runtime, out, counts, "story-wave-spawn-scroll", "TMNT OpenBOR Street")) {
        return exitCode(counts);
    }
    runtime.setArenaCpuFrozen(true);
    const auto before = runtime.snapshot();
    runtime.step(SymbolicInput{ .right = true }, 220);
    const auto afterScroll = runtime.snapshot();
    record(out, counts, afterScroll.cameraX > before.cameraX + 20.0f && afterScroll.p1.x > before.p1.x + 40.0f
            ? Status::Pass
            : Status::Fail,
        "story_camera_scrolls_with_player",
        "camera_before=" + std::to_string(before.cameraX)
        + " camera_after=" + std::to_string(afterScroll.cameraX)
        + " p1_before=" + std::to_string(before.p1.x)
        + " p1_after=" + std::to_string(afterScroll.p1.x));

    clearCurrentWave(runtime);
    const auto wave2 = runtime.snapshot();
    const bool midBossUsesRole = characterRefMatchesSnapshot(wave2.p2CharacterId, wave2.p2CharacterName, expectedMiniBoss);
    record(out, counts, wave2.storyWaveIndex == 1 && wave2.storyActiveEnemies == 1 && wave2.storyLivingEnemies == 1 ? Status::Pass : Status::Fail,
        "story_spawns_medium_midboss_wave", "wave=" + std::to_string(wave2.storyWaveIndex)
        + " active=" + std::to_string(wave2.storyActiveEnemies) + " living=" + std::to_string(wave2.storyLivingEnemies)
        + " defeated=" + std::to_string(wave2.storyEnemiesDefeated));
    record(out, counts, midBossUsesRole ? Status::Pass : Status::Fail,
        "story_medium_midboss_uses_mini_boss_role",
        "expected=" + expectedMiniBoss + " p2_id=" + wave2.p2CharacterId + " p2_name=" + wave2.p2CharacterName);

    clearCurrentWave(runtime);
    const auto wave3 = runtime.snapshot();
    const bool bossUsesRole = characterRefMatchesSnapshot(wave3.p2CharacterId, wave3.p2CharacterName, expectedBoss);
    record(out, counts, wave3.storyWaveIndex == 2 && wave3.storyActiveEnemies == 1 && wave3.storyLivingEnemies == 1 ? Status::Pass : Status::Fail,
        "story_spawns_medium_boss_wave",
        "wave=" + std::to_string(wave3.storyWaveIndex)
            + " active=" + std::to_string(wave3.storyActiveEnemies)
            + " living=" + std::to_string(wave3.storyLivingEnemies)
            + " defeated=" + std::to_string(wave3.storyEnemiesDefeated));
    record(out, counts, bossUsesRole ? Status::Pass : Status::Fail,
        "story_medium_boss_uses_boss_role",
        "expected=" + expectedBoss + " p2_id=" + wave3.p2CharacterId + " p2_name=" + wave3.p2CharacterName);
    summary(out, counts);
    return exitCode(counts);
}

int runStoryEnemyTargeting(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-enemy-targeting", "TMNT OpenBOR Street")) {
        return exitCode(counts);
    }
    runtime.setArenaCpuFrozen(false);
    runtime.setFighterPosition(0, 220.0f, 0.0f);
    runtime.setFighterPosition(1, 390.0f, 0.0f);
    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    const auto before = runtime.snapshot();
    runtime.step({}, 120);
    const auto after = runtime.snapshot();
    const bool movedTowardPlayer = after.p2.x < before.p2.x - 4.0f;
    const bool facesPlayer = after.p2.facing < 0;
    record(out, counts, movedTowardPlayer && facesPlayer ? Status::Pass : Status::Fail,
        "story_enemy_targets_player_not_other_enemies",
        "enemy_x_before=" + std::to_string(before.p2.x)
        + " enemy_x_after=" + std::to_string(after.p2.x)
        + " enemy_facing=" + std::to_string(after.p2.facing));
    summary(out, counts);
    return exitCode(counts);
}

int runStoryStageClear(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-stage-clear", "TMNT OpenBOR Street")) {
        return exitCode(counts);
    }
    clearCurrentWave(runtime);
    clearCurrentWave(runtime);
    clearCurrentWave(runtime);
    const bool resultReady = waitForMatchResult(runtime, 360);
    const auto snapshot = runtime.snapshot();
    record(out, counts, resultReady && snapshot.storyStageClear && snapshot.matchWinner == 1 ? Status::Pass : Status::Fail,
        "story_stage_clear_result",
        "phase=" + std::to_string(snapshot.matchPhase)
        + " winner=" + std::to_string(snapshot.matchWinner)
        + " clear=" + std::to_string(snapshot.storyStageClear ? 1 : 0)
        + " defeated=" + std::to_string(snapshot.storyEnemiesDefeated));
    captureOptionalScreenshot(
        runtime,
        out,
        counts,
        "DRAGON_STORY_STAGE_CLEAR_SCREENSHOT",
        "story_stage_clear_screenshot");
    summary(out, counts);
    return exitCode(counts);
}

int runStoryPlayerDefeat(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-player-defeat", "TMNT OpenBOR Street")) {
        return exitCode(counts);
    }
    runtime.setFighterLife(0, 0);
    runtime.step({}, 6);
    const bool resultReady = waitForMatchResult(runtime, 360);
    const auto snapshot = runtime.snapshot();
    record(out, counts, resultReady && snapshot.storyStageFailed && snapshot.matchWinner == 2 ? Status::Pass : Status::Fail,
        "story_player_defeat_result",
        "phase=" + std::to_string(snapshot.matchPhase)
        + " winner=" + std::to_string(snapshot.matchWinner)
        + " failed=" + std::to_string(snapshot.storyStageFailed ? 1 : 0));
    summary(out, counts);
    return exitCode(counts);
}

int runStoryProgressionAward(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-progression-award", "TMNT OpenBOR Street")) {
        return exitCode(counts);
    }
    clearCurrentWave(runtime);
    clearCurrentWave(runtime);
    clearCurrentWave(runtime);
    const bool resultReady = waitForMatchResult(runtime, 360);
    const auto snapshot = runtime.snapshot();
    const bool includesGoldBalance = snapshot.progressionAwardText.find("G") != std::string::npos
        && snapshot.progressionAwardText.find("BAL") != std::string::npos;
    record(out, counts, resultReady && snapshot.matchWinner == 1 && !snapshot.progressionAwardText.empty() && includesGoldBalance
            ? Status::Pass
            : Status::Fail,
        "story_progression_awarded",
        "winner=" + std::to_string(snapshot.matchWinner)
        + " award=\"" + snapshot.progressionAwardText + "\"");
    summary(out, counts);
    return exitCode(counts);
}

int runStoryRewardFeedback(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupStory(runtime, out, counts, "story-reward-feedback", "TMNT OpenBOR Street")) {
        return exitCode(counts);
    }
    const auto before = runtime.snapshot();
    runtime.setFighterLife(1, 0);
    runtime.step({}, 6);
    const auto snapshot = runtime.snapshot();
    const bool sawGoldText = snapshot.progressionAwardText.find("G") != std::string::npos
        && snapshot.progressionAwardText.find("BAL") != std::string::npos;
    record(out, counts,
        snapshot.storyRewardPopups > 0
            && snapshot.storyRewardCoins > 0
            && snapshot.progressionGoldBalance > before.progressionGoldBalance
            && sawGoldText
            ? Status::Pass
            : Status::Fail,
        "story_enemy_defeat_shows_reward_feedback",
        "popups=" + std::to_string(snapshot.storyRewardPopups)
        + " coins=" + std::to_string(snapshot.storyRewardCoins)
        + " goldBefore=" + std::to_string(before.progressionGoldBalance)
        + " goldAfter=" + std::to_string(snapshot.progressionGoldBalance)
        + " award=\"" + snapshot.progressionAwardText + "\"");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
