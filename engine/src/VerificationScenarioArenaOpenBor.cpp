#include "VerificationScenario.h"

#include "AppTypes.h"

#include <SDL3/SDL_stdinc.h>

#include <cstdlib>
#include <filesystem>
#include <cmath>
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

bool waitForControllableIdle(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        const auto p1 = runtime.snapshot().p1;
        if (p1.stateNo == 0 && p1.ctrl && p1.onGround && p1.moveType == 'I') {
            return true;
        }
        runtime.step({}, 1);
    }
    const auto p1 = runtime.snapshot().p1;
    return p1.stateNo == 0 && p1.ctrl && p1.onGround && p1.moveType == 'I';
}

bool setupArenaOpenBorStage(
    RuntimeProbe& runtime,
    std::ostream& out,
    Counts& counts,
    std::string_view scenarioName,
    std::string_view stageHint) {
    if (!runtime.setup("kfm", stageHint, ScenarioMode::Arena, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Arena setup failed");
        summary(out, counts);
        return false;
    }
    header(out, runtime, scenarioName);

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "arena_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " timer_ticks=" + std::to_string(runtime.snapshot().matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "arena_checks", "Arena fight phase was not active");
        summary(out, counts);
        return false;
    }
    return true;
}

int runArenaOpenBorScrollStageFixture(
    RuntimeProbe& runtime,
    std::ostream& out,
    std::string_view scenarioName,
    std::string_view stageHint,
    float expectedEndCameraX,
    float minInitialSeparation) {
    Counts counts;
    if (!setupArenaOpenBorStage(runtime, out, counts, scenarioName, stageHint)) {
        return exitCode(counts);
    }

    const auto loaded = runtime.snapshot();
    record(out, counts, loaded.selectedStageLegacyOpenBorSection ? Status::Pass : Status::Fail,
        "openbor_stage_metadata_loaded",
        "stage=\"" + runtime.stageName() + "\"");
    record(out, counts, std::fabs(loaded.p2.x - loaded.p1.x) >= minInitialSeparation ? Status::Pass : Status::Fail,
        "openbor_initial_spawn_spacing",
        "p1_x=" + std::to_string(loaded.p1.x)
        + " p2_x=" + std::to_string(loaded.p2.x)
        + " min_separation=" + std::to_string(minInitialSeparation));

    runtime.positionFighters(100.0f, 40.0f);
    runtime.setFighterControl(1, false);
    runtime.forceFighterLiedown(1, 9999);
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0));
    if (!idle) {
        record(out, counts, Status::Blocked, "openbor_scroll_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto start = runtime.snapshot();
    runtime.step(SymbolicInput{ .right = true }, 180);
    const auto forward = runtime.snapshot();
    const bool scrolledForward = forward.cameraX > start.cameraX + 30.0f
        && forward.p1.x > start.p1.x + 60.0f;
    record(out, counts, scrolledForward ? Status::Pass : Status::Fail, "openbor_camera_scrolls_forward",
        "camera_before=" + std::to_string(start.cameraX)
        + " camera_after=" + std::to_string(forward.cameraX)
        + " p1_x_before=" + std::to_string(start.p1.x)
        + " p1_x_after=" + std::to_string(forward.p1.x));

    if (const char* screenshotPath = SDL_getenv("DRAGON_SCREENSHOT_PATH"); screenshotPath && *screenshotPath) {
        const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
        record(out, counts, captured ? Status::Pass : Status::Fail, "screenshot_captured", screenshotPath);
    }

    runtime.step(SymbolicInput{ .left = true }, 90);
    const auto back = runtime.snapshot();
    record(out, counts, back.cameraX >= forward.cameraX - 0.5f ? Status::Pass : Status::Fail,
        "openbor_camera_does_not_scroll_backward",
        "camera_forward=" + std::to_string(forward.cameraX)
        + " camera_after_left=" + std::to_string(back.cameraX)
        + " p1_x_after_left=" + std::to_string(back.p1.x));

    runtime.step(SymbolicInput{ .right = true }, 1400);
    const auto end = runtime.snapshot();
    record(out, counts,
        end.cameraX >= expectedEndCameraX - 10.0f && end.cameraX <= expectedEndCameraX + 0.5f ? Status::Pass : Status::Fail,
        "openbor_camera_clamps_at_stage_end",
        "camera_forward=" + std::to_string(forward.cameraX)
        + " camera_end=" + std::to_string(end.cameraX));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace

int runArenaOpenBorScrollStage(RuntimeProbe& runtime, std::ostream& out) {
    return runArenaOpenBorScrollStageFixture(runtime, out, "arena-openbor-scroll-stage", "OpenBOR Scroll", 1760.0f, 80.0f);
}

int runArenaTmntOpenBorStage(RuntimeProbe& runtime, std::ostream& out) {
    return runArenaOpenBorScrollStageFixture(runtime, out, "arena-tmnt-openbor-stage", "TMNT OpenBOR Street", 2080.0f, 180.0f);
}

} // namespace dragon::verification
