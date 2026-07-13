#include "VerificationScenarioCommon.h"

namespace dragon::verification {
namespace {

std::string stageSelectSnapshotDetail(const RuntimeSnapshot& snapshot, const RuntimeProbe& runtime) {
    return "screen=" + std::to_string(snapshot.screen)
        + " pending=" + std::to_string(snapshot.pendingMode)
        + " logical_width=" + std::to_string(snapshot.logicalWidth)
        + " stage_index=" + std::to_string(snapshot.selectedStageIndex)
        + " stages=" + std::to_string(snapshot.stageCount)
        + " backgrounds=" + std::to_string(snapshot.stageBackgroundCount)
        + " stage=\"" + runtime.stageName() + "\"";
}

void captureStageSelectScreenshot(RuntimeProbe& runtime, std::ostream& out, Counts& counts) {
    const char* screenshotPath = std::getenv("DRAGON_STAGE_SELECT_SCREENSHOT");
    if (!screenshotPath || !*screenshotPath) {
        screenshotPath = std::getenv("DRAGON_SCREENSHOT_PATH");
    }
    if (!screenshotPath || !*screenshotPath) {
        return;
    }

    const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
    record(out, counts, captured ? Status::Pass : Status::Fail, "stage_select_screenshot", screenshotPath);
}

void captureArenaSetupScreenshot(RuntimeProbe& runtime, std::ostream& out, Counts& counts) {
    const char* screenshotPath = std::getenv("DRAGON_ARENA_SETUP_SCREENSHOT");
    if (!screenshotPath || !*screenshotPath) {
        screenshotPath = std::getenv("DRAGON_SCREENSHOT_PATH");
    }
    if (!screenshotPath || !*screenshotPath) {
        return;
    }

    const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
    record(out, counts, captured ? Status::Pass : Status::Fail, "arena_setup_screenshot", screenshotPath);
}

} // namespace

int runStageSelectResponsiveLayout(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY stage-select-responsive-layout\n"
        << "root: " << runtime.rootText() << "\n";

    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Stage-select setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto initial = runtime.snapshot();
    record(out, counts,
        initial.screen == static_cast<int>(Screen::StageSelect)
            && initial.pendingMode == static_cast<int>(PendingMode::SinglePlayer)
            ? Status::Pass
            : Status::Fail,
        "stage_select_screen",
        stageSelectSnapshotDetail(initial, runtime));
    record(out, counts,
        initial.stageCount > 0 ? Status::Pass : Status::Fail,
        "stage_select_has_stages",
        stageSelectSnapshotDetail(initial, runtime));
    record(out, counts,
        initial.logicalWidth >= 1000 ? Status::Pass : Status::Partial,
        "stage_select_hd_output_probe",
        "logical_width=" + std::to_string(initial.logicalWidth)
            + " expected_hd_width_at_default=1280");

    captureStageSelectScreenshot(runtime, out, counts);

    const int beforeIndex = initial.selectedStageIndex;
    runtime.pressKey("right");
    const auto afterRight = runtime.snapshot();
    const bool canCycle = initial.stageCount > 1;
    record(out, counts,
        !canCycle || afterRight.selectedStageIndex != beforeIndex ? Status::Pass : Status::Fail,
        "stage_select_cycles_right",
        "before=" + std::to_string(beforeIndex)
            + " after=" + std::to_string(afterRight.selectedStageIndex)
            + " stages=" + std::to_string(afterRight.stageCount)
            + " stage=\"" + runtime.stageName() + "\"");
    runtime.pressKey("left");
    const auto afterLeft = runtime.snapshot();
    record(out, counts,
        !canCycle || afterLeft.selectedStageIndex == beforeIndex ? Status::Pass : Status::Fail,
        "stage_select_cycles_left",
        "before=" + std::to_string(beforeIndex)
            + " after=" + std::to_string(afterLeft.selectedStageIndex)
            + " stage=\"" + runtime.stageName() + "\"");

    summary(out, counts);
    return exitCode(counts);
}

int runArenaSetupResponsiveLayout(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY arena-setup-responsive-layout\n"
        << "root: " << runtime.rootText() << "\n";

    if (!runtime.setupArenaSetupScreen("A.Ben", out)) {
        record(out, counts, Status::Blocked, "setup_arena_setup_screen", "Arena setup screen setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto snapshot = runtime.snapshot();
    record(out, counts,
        snapshot.screen == static_cast<int>(Screen::ArenaSetup)
            && snapshot.pendingMode == static_cast<int>(PendingMode::Arena)
            ? Status::Pass
            : Status::Fail,
        "arena_setup_screen",
        "screen=" + std::to_string(snapshot.screen)
            + " pending=" + std::to_string(snapshot.pendingMode)
            + " logical_width=" + std::to_string(snapshot.logicalWidth)
            + " stage=\"" + runtime.stageName() + "\"");
    record(out, counts,
        snapshot.logicalWidth >= 1000 ? Status::Pass : Status::Partial,
        "arena_setup_hd_output_probe",
        "logical_width=" + std::to_string(snapshot.logicalWidth)
            + " expected_hd_width_at_default=1280");

    captureArenaSetupScreenshot(runtime, out, counts);

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
