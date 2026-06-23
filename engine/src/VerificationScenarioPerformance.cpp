#include "VerificationScenario.h"

#include "AppTypes.h"
#include "ControlsOptionsMenu.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <ostream>
#include <string>

namespace dragon::verification {
namespace {

enum class Status { Pass, Partial, Fail, Blocked };

struct Counts {
    int pass = 0;
    int partial = 0;
    int fail = 0;
    int blocked = 0;
};

const char* statusText(Status status) {
    switch (status) {
    case Status::Pass: return "PASS";
    case Status::Partial: return "PARTIAL";
    case Status::Fail: return "FAIL";
    case Status::Blocked:
    default: return "BLOCKED";
    }
}

void record(std::ostream& out, Counts& counts, Status status, const std::string& name, const std::string& detail = {}) {
    out << statusText(status) << ' ' << name << "\n";
    if (!detail.empty()) {
        out << "  " << detail << "\n";
    }
    switch (status) {
    case Status::Pass: ++counts.pass; break;
    case Status::Partial: ++counts.partial; break;
    case Status::Fail: ++counts.fail; break;
    case Status::Blocked: ++counts.blocked; break;
    }
}

void summary(std::ostream& out, const Counts& counts) {
    out << "SUMMARY pass=" << counts.pass
        << " partial=" << counts.partial
        << " fail=" << counts.fail
        << " blocked=" << counts.blocked << "\n";
}

int exitCode(const Counts& counts) {
    return counts.fail == 0 && counts.blocked == 0 ? 0 : 1;
}

std::string perfDetail(const RuntimePerformanceResult& result) {
    return "fps=" + std::to_string(static_cast<int>(result.fpsEquivalent))
        + " avg_ms=" + std::to_string(result.avgFrameMs)
        + " p95_ms=" + std::to_string(result.p95FrameMs)
        + " worst_ms=" + std::to_string(result.worstFrameMs)
        + " dominant=" + result.dominantSection
        + " draw=" + std::to_string(result.counters.drawCalls)
        + " skip=" + std::to_string(result.counters.skippedDraws)
        + " fighters=" + std::to_string(result.counters.fighters)
        + " helpers=" + std::to_string(result.counters.helpers)
        + " projectiles=" + std::to_string(result.counters.projectiles)
        + " effects=" + std::to_string(result.counters.effects)
        + " pause_frames=" + std::to_string(result.pauseFrames);
}

ControlsOptionsContext perfControlsOptionsContext(MainSettings settings = {}) {
    ControlsOptionsContext context;
    context.settings = settings;
    context.playerProfileIds = { "guest", "guest", "guest", "guest" };
    context.playerProfileNames = { "Guest", "Guest", "Guest", "Guest" };
    context.gamepadAssignmentText = { "AUTO", "AUTO", "AUTO", "AUTO" };
    context.padSummary = "PADS 0";
    context.promptStyle = settings.gamepadPromptStyle;
    return context;
}

bool setupStoryStress(RuntimeProbe& runtime, std::ostream& out) {
    if (!runtime.setup("Evil Ryu", "TMNT OpenBOR Street", ScenarioMode::Story, out)) {
        return runtime.setup("Leonardo", "TMNT OpenBOR Street", ScenarioMode::Story, out);
    }
    return true;
}

bool setupArenaStress(RuntimeProbe& runtime, std::ostream& out) {
    if (!runtime.setup("Leonardo", "TMNT OpenBOR Street", ScenarioMode::Arena, out, 3)) {
        return runtime.setup("Evil Ken", "TMNT OpenBOR Street", ScenarioMode::Arena, out, 3);
    }
    return true;
}

Status perfStatus(const RuntimePerformanceResult& result) {
    if (!result.ran || result.gameplayFrames <= 0 || result.fpsEquivalent <= 0.0) {
        return Status::Fail;
    }
    return result.fpsEquivalent >= 55.0 ? Status::Pass : Status::Fail;
}

} // namespace

int runRuntimePerformanceMetrics(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY runtime-performance-metrics\n";

    MainSettings settings;
    settings.optionsScreen = OptionsMenuScreen::Video;
    const auto rows = buildControlsOptionsRows(perfControlsOptionsContext(settings));
    const bool hasPerfRow = rows.size() == kOptionsVideoCount
        && rows.size() > 3
        && rows[3].label == "PERFORMANCE HUD"
        && rows[3].value == "FPS";
    record(out, counts, hasPerfRow ? Status::Pass : Status::Fail, "video_performance_hud_option");
    settings.performanceHudMode = cyclePerformanceHudMode(settings.performanceHudMode, 1);
    record(out, counts,
        settings.performanceHudMode == PerformanceHudMode::Perf ? Status::Pass : Status::Fail,
        "performance_hud_cycles_to_perf",
        performanceHudModeText(settings.performanceHudMode));

    if (!runtime.setup("Evil Ken", "", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "runtime_setup");
        summary(out, counts);
        return exitCode(counts);
    }
    RuntimePerformanceResult result = runtime.measurePerformance(12, 48, true, false);
    record(out, counts,
        result.ran && result.gameplayFrames > 0 && result.counters.drawCalls > 0 && !result.dominantSection.empty()
            ? Status::Pass
            : Status::Fail,
        "performance_counters_populate",
        perfDetail(result));
    summary(out, counts);
    return exitCode(counts);
}

int runStoryWave3Performance(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY story-wave3-performance\n";
    if (!setupStoryStress(runtime, out)) {
        record(out, counts, Status::Blocked, "story_setup");
        summary(out, counts);
        return exitCode(counts);
    }
    runtime.setStoryWave(2);
    runtime.setFighterPower(0, 3000);
    RuntimePerformanceResult result = runtime.measurePerformance(30, 150, true, true);
    const RuntimeSnapshot snapshot = runtime.snapshot();
    record(out, counts,
        snapshot.storyWaveIndex == 2 && snapshot.storyActiveEnemies >= 3 ? Status::Pass : Status::Fail,
        "story_wave3_active",
        "wave=" + std::to_string(snapshot.storyWaveIndex + 1)
            + " active=" + std::to_string(snapshot.storyActiveEnemies));
    record(out, counts, perfStatus(result), "story_wave3_sustains_55fps", perfDetail(result));
    record(out, counts,
        result.pauseFrames >= 0 ? Status::Pass : Status::Fail,
        "story_pause_frames_reported",
        "pause_avg_ms=" + std::to_string(result.pauseFrameAvgMs));
    summary(out, counts);
    return exitCode(counts);
}

int runArenaOpenBor4FighterPerformance(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY arena-openbor-4fighter-performance\n";
    if (!setupArenaStress(runtime, out)) {
        record(out, counts, Status::Blocked, "arena_setup");
        summary(out, counts);
        return exitCode(counts);
    }
    runtime.setFighterPower(0, 3000);
    RuntimePerformanceResult result = runtime.measurePerformance(30, 150, true, true);
    const RuntimeSnapshot snapshot = runtime.snapshot();
    record(out, counts,
        snapshot.fighterCount >= 4 ? Status::Pass : Status::Fail,
        "arena_four_fighters_active",
        "fighters=" + std::to_string(snapshot.fighterCount));
    record(out, counts, perfStatus(result), "arena_4fighter_sustains_55fps", perfDetail(result));
    summary(out, counts);
    return exitCode(counts);
}

int runRenderCullingPreservesRuntime(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY render-culling-preserves-runtime\n";

    SDL_setenv_unsafe("DRAGON_DISABLE_RENDER_CULLING", "1", 1);
    if (!setupStoryStress(runtime, out)) {
        SDL_unsetenv_unsafe("DRAGON_DISABLE_RENDER_CULLING");
        record(out, counts, Status::Blocked, "baseline_setup");
        summary(out, counts);
        return exitCode(counts);
    }
    runtime.setStoryWave(2);
    runtime.setFighterPosition(1, -1600.0f, 0.0f);
    runtime.setFighterPosition(2, 2400.0f, 0.0f);
    runtime.setFighterPosition(3, 2600.0f, 0.0f);
    RuntimePerformanceResult noCull = runtime.measurePerformance(5, 30, true, true);
    RuntimeSnapshot noCullSnapshot = runtime.snapshot();

    SDL_unsetenv_unsafe("DRAGON_DISABLE_RENDER_CULLING");
    if (!setupStoryStress(runtime, out)) {
        record(out, counts, Status::Blocked, "culling_setup");
        summary(out, counts);
        return exitCode(counts);
    }
    runtime.setStoryWave(2);
    runtime.setFighterPosition(1, -1600.0f, 0.0f);
    runtime.setFighterPosition(2, 2400.0f, 0.0f);
    runtime.setFighterPosition(3, 2600.0f, 0.0f);
    RuntimePerformanceResult cull = runtime.measurePerformance(5, 30, true, true);
    RuntimeSnapshot cullSnapshot = runtime.snapshot();

    const bool sameRuntime = noCullSnapshot.storyWaveIndex == cullSnapshot.storyWaveIndex
        && noCullSnapshot.storyActiveEnemies == cullSnapshot.storyActiveEnemies
        && noCullSnapshot.storyLivingEnemies == cullSnapshot.storyLivingEnemies
        && noCullSnapshot.p1.stateNo == cullSnapshot.p1.stateNo
        && noCullSnapshot.p1.life == cullSnapshot.p1.life
        && noCullSnapshot.activeHelpers == cullSnapshot.activeHelpers
        && noCullSnapshot.activeEffects == cullSnapshot.activeEffects;
    record(out, counts, sameRuntime ? Status::Pass : Status::Fail, "culling_preserves_runtime_state");
    record(out, counts,
        cull.counters.skippedDraws > noCull.counters.skippedDraws ? Status::Pass : Status::Fail,
        "culling_reports_skipped_draws",
        "no_cull=" + std::to_string(noCull.counters.skippedDraws)
            + " cull=" + std::to_string(cull.counters.skippedDraws));
    record(out, counts,
        cull.counters.drawCalls <= noCull.counters.drawCalls || cull.counters.skippedDraws > 0
            ? Status::Pass
            : Status::Fail,
        "culling_does_not_increase_draw_work",
        "no_cull=" + perfDetail(noCull) + " cull=" + perfDetail(cull));
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
