#include "VerificationScenario.h"

#include "AppTypes.h"

#include <SDL3/SDL_stdinc.h>

#include <cstdlib>
#include <filesystem>
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

void captureOptionalScreenshot(RuntimeProbe& runtime, std::ostream& out, Counts& counts) {
    const char* screenshotPath = SDL_getenv("DRAGON_LOADING_SCREENSHOT");
    if (!screenshotPath || !*screenshotPath) {
        screenshotPath = SDL_getenv("DRAGON_SCREENSHOT_PATH");
    }
    if (!screenshotPath || !*screenshotPath) {
        return;
    }
    const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
    record(out, counts, captured ? Status::Pass : Status::Fail, "loading_screenshot", screenshotPath);
}

} // namespace

int runVsLoadingProgressBar(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY vs-loading-progress-bar\n";
    if (!runtime.setupStageSelect("A.Ben", ScenarioMode::Story, out)) {
        record(out, counts, Status::Blocked, "setup_stage_select", "Story loading setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.pressKey("enter");
    const auto loadingSnapshot = runtime.snapshot();
    record(out, counts,
        loadingSnapshot.screen == static_cast<int>(Screen::VersusScreen)
            && loadingSnapshot.loadingProgressActive
            && !loadingSnapshot.loadingProgressFailed
            ? Status::Pass : Status::Fail,
        "loading_progress_active",
        "screen=" + std::to_string(loadingSnapshot.screen)
            + " active=" + std::to_string(loadingSnapshot.loadingProgressActive ? 1 : 0)
            + " failed=" + std::to_string(loadingSnapshot.loadingProgressFailed ? 1 : 0));
    captureOptionalScreenshot(runtime, out, counts);

    if (!runtime.setup("A.Ben", "TMNT OpenBOR Street", ScenarioMode::Story, out, 1)) {
        record(out, counts, Status::Blocked, "setup_prepared_loading", "Prepared story loading setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto readySnapshot = runtime.snapshot();
    record(out, counts,
        readySnapshot.loadingProgressFraction >= 0.999f ? Status::Pass : Status::Fail,
        "loading_progress_reaches_ready",
        "fraction=" + std::to_string(readySnapshot.loadingProgressFraction)
            + " phase=\"" + readySnapshot.loadingProgressPhase + "\"");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
