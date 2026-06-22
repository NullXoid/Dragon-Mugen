#include "VerificationScenario.h"

#include "AppTypes.h"

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

bool waitForMatchPhase(RuntimeProbe& runtime, MatchPhase phase, int maxFrames) {
    const int expected = static_cast<int>(phase);
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == expected) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == expected;
}

bool waitForActiveFight(RuntimeProbe& runtime, int maxFrames) {
    return waitForMatchPhase(runtime, MatchPhase::Fight, maxFrames);
}

void forceClassicOutcome(RuntimeProbe& runtime, int p1Life, int p2Life, int timerTicks) {
    runtime.setFighterLife(0, p1Life);
    runtime.setFighterLife(1, p2Life);
    if (timerTicks >= 0) {
        runtime.setMatchTimerTicks(timerTicks);
    }
    runtime.step({}, 2);
}

std::string classicOutcomeDetail(const RuntimeSnapshot& snapshot) {
    return "phase=" + std::to_string(snapshot.matchPhase)
        + " winner=" + std::to_string(snapshot.roundWinner)
        + " reason=" + std::to_string(snapshot.roundEndReason)
        + " wins=" + std::to_string(snapshot.roundWinsP1) + "-" + std::to_string(snapshot.roundWinsP2)
        + " complete=" + std::to_string(snapshot.matchComplete ? 1 : 0)
        + " match_winner=" + std::to_string(snapshot.matchWinner)
        + " text=\"" + snapshot.lastHitText + "\"";
}

std::string routeDetail(const RuntimeSnapshot& snapshot) {
    return "screen=" + std::to_string(snapshot.screen)
        + " pending=" + std::to_string(snapshot.pendingMode)
        + " phase=" + std::to_string(snapshot.matchPhase)
        + " light_pause=" + std::to_string(snapshot.fightPauseOpen ? 1 : 0)
        + " match_pause=" + std::to_string(snapshot.singleFightPauseOpen ? 1 : 0)
        + " pause_option=" + std::to_string(snapshot.selectedSingleFightPauseOption)
        + " result_option=" + std::to_string(snapshot.selectedMatchResultOption)
        + " wins=" + std::to_string(snapshot.roundWinsP1) + "-" + std::to_string(snapshot.roundWinsP2);
}

void recordForcedRoundOutcome(
    RuntimeProbe& runtime,
    Counts& counts,
    std::ostream& out,
    std::string_view name,
    int p1Life,
    int p2Life,
    int timerTicks,
    RoundEndReason expectedReason,
    int expectedWinner,
    int expectedP1Wins,
    int expectedP2Wins) {
    forceClassicOutcome(runtime, p1Life, p2Life, timerTicks);
    const auto finish = runtime.snapshot();
    const bool finishMatches = finish.matchPhase == static_cast<int>(MatchPhase::RoundFinish)
        && finish.roundWinner == expectedWinner
        && finish.roundEndReason == static_cast<int>(expectedReason);
    record(out, counts, finishMatches ? Status::Pass : Status::Fail,
        std::string(name) + "_round_finish",
        classicOutcomeDetail(finish));

    const bool reachedResult = waitForMatchPhase(runtime, MatchPhase::RoundResult, 360);
    const auto result = runtime.snapshot();
    const bool scoreMatches = reachedResult
        && result.roundWinsP1 == expectedP1Wins
        && result.roundWinsP2 == expectedP2Wins
        && result.matchPhase == static_cast<int>(MatchPhase::RoundResult);
    record(out, counts, scoreMatches ? Status::Pass : Status::Fail,
        std::string(name) + "_round_score",
        classicOutcomeDetail(result));
}

bool setupReady(
    RuntimeProbe& runtime,
    Counts& counts,
    std::ostream& out,
    ScenarioMode mode,
    std::string_view name) {
    if (!runtime.setup("kfm", "Mountainside", mode, out)) {
        record(out, counts, Status::Blocked, std::string(name) + "_setup", "KFM/Mountainside setup failed");
        return false;
    }
    const bool ready = waitForActiveFight(runtime, 420);
    record(out, counts, ready ? Status::Pass : Status::Fail,
        std::string(name) + "_fight_phase_ready",
        routeDetail(runtime.snapshot()));
    return ready;
}

bool openSingleFightOptions(RuntimeProbe& runtime, Counts& counts, std::ostream& out, std::string_view name) {
    runtime.pressKey("enter");
    const auto lightPause = runtime.snapshot();
    const bool lightOpen = lightPause.fightPauseOpen && !lightPause.singleFightPauseOpen;
    record(out, counts, lightOpen ? Status::Pass : Status::Fail,
        std::string(name) + "_light_pause_open",
        routeDetail(lightPause));

    runtime.pressKey("f2");
    const auto options = runtime.snapshot();
    const bool optionsOpen = !options.fightPauseOpen && options.singleFightPauseOpen
        && options.selectedSingleFightPauseOption == 0;
    record(out, counts, optionsOpen ? Status::Pass : Status::Fail,
        std::string(name) + "_pause_options_open",
        routeDetail(options));
    return lightOpen && optionsOpen;
}

void recordLightPauseResume(
    RuntimeProbe& runtime,
    Counts& counts,
    std::ostream& out,
    ScenarioMode mode,
    std::string_view name) {
    if (!setupReady(runtime, counts, out, mode, name)) {
        return;
    }
    runtime.pressKey("enter");
    const auto paused = runtime.snapshot();
    runtime.pressKey("enter");
    const auto resumed = runtime.snapshot();
    const bool ok = paused.fightPauseOpen
        && !resumed.fightPauseOpen
        && resumed.screen == static_cast<int>(Screen::FightView);
    record(out, counts, ok ? Status::Pass : Status::Fail,
        std::string(name) + "_light_pause_resume",
        routeDetail(resumed));
}

void recordPauseOptionRoute(
    RuntimeProbe& runtime,
    Counts& counts,
    std::ostream& out,
    ScenarioMode mode,
    std::string_view name,
    int option,
    int expectedScreen,
    bool expectRoundReset = false) {
    if (!setupReady(runtime, counts, out, mode, name)) {
        return;
    }
    if (!openSingleFightOptions(runtime, counts, out, name)) {
        return;
    }
    for (int i = 0; i < option; ++i) {
        runtime.pressKey("down");
    }
    const auto selected = runtime.snapshot();
    runtime.pressKey("enter");
    const auto routed = runtime.snapshot();
    const bool ok = selected.selectedSingleFightPauseOption == option
        && routed.screen == expectedScreen
        && !routed.singleFightPauseOpen
        && (!expectRoundReset
            || (routed.matchPhase == static_cast<int>(MatchPhase::RoundStart)
                && routed.roundWinsP1 == 0
                && routed.roundWinsP2 == 0));
    record(out, counts, ok ? Status::Pass : Status::Fail,
        std::string(name) + "_pause_option_" + std::to_string(option) + "_route",
        "selected=" + routeDetail(selected) + " routed=" + routeDetail(routed));
}

bool forceMatchResult(
    RuntimeProbe& runtime,
    Counts& counts,
    std::ostream& out,
    ScenarioMode mode,
    std::string_view name) {
    if (!setupReady(runtime, counts, out, mode, name)) {
        return false;
    }
    forceClassicOutcome(runtime, 1000, 0, -1);
    if (!waitForMatchPhase(runtime, MatchPhase::RoundResult, 360)) {
        record(out, counts, Status::Fail, std::string(name) + "_first_round_result", routeDetail(runtime.snapshot()));
        return false;
    }
    if (!waitForActiveFight(runtime, 720)) {
        record(out, counts, Status::Fail, std::string(name) + "_second_round_ready", routeDetail(runtime.snapshot()));
        return false;
    }
    forceClassicOutcome(runtime, 1000, 0, -1);
    const bool matchResult = waitForMatchPhase(runtime, MatchPhase::MatchResult, 720);
    record(out, counts, matchResult ? Status::Pass : Status::Fail,
        std::string(name) + "_match_result_ready",
        routeDetail(runtime.snapshot()));
    return matchResult;
}

void recordMatchResultRoute(
    RuntimeProbe& runtime,
    Counts& counts,
    std::ostream& out,
    ScenarioMode mode,
    std::string_view name,
    int option,
    int expectedScreen,
    bool shortcutRestart = false) {
    if (!forceMatchResult(runtime, counts, out, mode, name)) {
        return;
    }
    if (shortcutRestart) {
        runtime.pressKey("r");
    } else {
        for (int i = 0; i < option; ++i) {
            runtime.pressKey("down");
        }
        const auto selected = runtime.snapshot();
        record(out, counts, selected.selectedMatchResultOption == option ? Status::Pass : Status::Fail,
            std::string(name) + "_result_option_selected",
            routeDetail(selected));
        runtime.pressKey("enter");
    }
    const auto routed = runtime.snapshot();
    const bool ok = routed.screen == expectedScreen
        && (!shortcutRestart
            || (routed.matchPhase == static_cast<int>(MatchPhase::RoundStart)
                && routed.roundWinsP1 == 0
                && routed.roundWinsP2 == 0));
    record(out, counts, ok ? Status::Pass : Status::Fail,
        std::string(name) + "_result_route",
        routeDetail(routed));
}

} // namespace

int runClassicFightOutcomes(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;

    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup_p1_ko", "KFM/Mountainside Single Player setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "classic-fight-outcomes");
    const bool singlePlayerReady = waitForActiveFight(runtime, 420);
    const auto singlePlayerStart = runtime.snapshot();
    record(out, counts, singlePlayerReady ? Status::Pass : Status::Fail,
        "single_player_fight_phase_ready",
        "match_phase=" + std::to_string(singlePlayerStart.matchPhase));
    recordForcedRoundOutcome(runtime, counts, out, "single_player_p1_ko", 1000, 0, -1, RoundEndReason::Ko, 1, 1, 0);

    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup_p2_ko", "KFM/Mountainside Single Player setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    waitForActiveFight(runtime, 420);
    recordForcedRoundOutcome(runtime, counts, out, "single_player_p2_ko", 0, 1000, -1, RoundEndReason::Ko, 2, 0, 1);

    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup_double_ko", "KFM/Mountainside Single Player setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    waitForActiveFight(runtime, 420);
    recordForcedRoundOutcome(runtime, counts, out, "single_player_double_ko", 0, 0, -1, RoundEndReason::DoubleKo, 0, 0, 0);

    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup_time_draw", "KFM/Mountainside Single Player setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    waitForActiveFight(runtime, 420);
    recordForcedRoundOutcome(runtime, counts, out, "single_player_time_over_draw", 500, 500, 0, RoundEndReason::TimeUp, 0, 0, 0);

    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup_match_result", "KFM/Mountainside Single Player setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    const bool matchStart = waitForActiveFight(runtime, 420);
    forceClassicOutcome(runtime, 1000, 0, -1);
    const bool firstResult = waitForMatchPhase(runtime, MatchPhase::RoundResult, 360);
    const auto firstRound = runtime.snapshot();
    const bool firstScore = firstResult && firstRound.roundWinsP1 == 1 && firstRound.roundWinsP2 == 0 && !firstRound.matchComplete;
    record(out, counts, matchStart && firstScore ? Status::Pass : Status::Fail,
        "single_player_first_round_score_before_match_complete",
        classicOutcomeDetail(firstRound));
    const bool secondFight = waitForActiveFight(runtime, 720);
    forceClassicOutcome(runtime, 1000, 0, -1);
    const bool reachedMatchResult = waitForMatchPhase(runtime, MatchPhase::MatchResult, 720);
    const auto matchResult = runtime.snapshot();
    const bool matchResultOk = secondFight
        && reachedMatchResult
        && matchResult.matchComplete
        && matchResult.matchWinner == 1
        && matchResult.roundWinsP1 >= 2
        && matchResult.selectedMatchResultOption == 0;
    record(out, counts, matchResultOk ? Status::Pass : Status::Fail,
        "single_player_match_result_after_two_wins",
        classicOutcomeDetail(matchResult)
            + " selected_result=" + std::to_string(matchResult.selectedMatchResultOption));

    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Versus, out)) {
        record(out, counts, Status::Blocked, "setup_vs", "KFM/Mountainside VS setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    const bool vsStart = waitForActiveFight(runtime, 420);
    forceClassicOutcome(runtime, 1000, 0, -1);
    const bool vsResult = waitForMatchPhase(runtime, MatchPhase::RoundResult, 360);
    const auto vsSnap = runtime.snapshot();
    const bool vsOutcomeOk = vsStart && vsResult && vsSnap.roundWinner == 1 && vsSnap.roundWinsP1 == 1;
    record(out, counts, vsOutcomeOk ? Status::Pass : Status::Fail,
        "versus_p1_ko_round_score",
        classicOutcomeDetail(vsSnap));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runClassicFightRouting(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY classic-fight-routing\n" << "root: " << runtime.rootText() << "\n";
    const int modeSelectScreen = static_cast<int>(Screen::ModeSelect);
    const int characterSelectScreen = static_cast<int>(Screen::CharacterSelect);
    const int stageSelectScreen = static_cast<int>(Screen::StageSelect);
    const int fightViewScreen = static_cast<int>(Screen::FightView);

    recordLightPauseResume(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player");
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_restart", 1, fightViewScreen, true);
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_fighter_select", 2, characterSelectScreen);
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_stage_select", 3, stageSelectScreen);
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_mode_select", 4, modeSelectScreen);

    recordLightPauseResume(runtime, counts, out, ScenarioMode::Versus, "versus");
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::Versus, "versus_restart", 1, fightViewScreen, true);
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::Versus, "versus_fighter_select", 2, characterSelectScreen);
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::Versus, "versus_stage_select", 3, stageSelectScreen);
    recordPauseOptionRoute(runtime, counts, out, ScenarioMode::Versus, "versus_mode_select", 4, modeSelectScreen);

    recordMatchResultRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_result_rematch", 0, fightViewScreen, true);
    recordMatchResultRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_result_fighter_select", 1, characterSelectScreen);
    recordMatchResultRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_result_stage_select", 2, stageSelectScreen);
    recordMatchResultRoute(runtime, counts, out, ScenarioMode::SinglePlayer, "single_player_result_mode_select", 3, modeSelectScreen);

    recordMatchResultRoute(runtime, counts, out, ScenarioMode::Versus, "versus_result_rematch", 0, fightViewScreen, true);
    recordMatchResultRoute(runtime, counts, out, ScenarioMode::Versus, "versus_result_fighter_select", 1, characterSelectScreen);
    recordMatchResultRoute(runtime, counts, out, ScenarioMode::Versus, "versus_result_stage_select", 2, stageSelectScreen);
    recordMatchResultRoute(runtime, counts, out, ScenarioMode::Versus, "versus_result_mode_select", 3, modeSelectScreen);

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
