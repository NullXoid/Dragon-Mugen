#include "VerificationScenario.h"

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

} // namespace

int runVsLoadingProgressBar(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY vs-loading-progress-bar\n";
    if (!runtime.setup("Dcat_Leo", "TMNT OpenBOR Street", ScenarioMode::Story, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Story loading setup failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto snapshot = runtime.snapshot();
    record(out, counts,
        snapshot.loadingProgressActive && !snapshot.loadingProgressFailed ? Status::Pass : Status::Fail,
        "loading_progress_active",
        "active=" + std::to_string(snapshot.loadingProgressActive ? 1 : 0)
            + " failed=" + std::to_string(snapshot.loadingProgressFailed ? 1 : 0));
    record(out, counts,
        snapshot.loadingProgressFraction >= 0.999f ? Status::Pass : Status::Fail,
        "loading_progress_reaches_ready",
        "fraction=" + std::to_string(snapshot.loadingProgressFraction)
            + " phase=\"" + snapshot.loadingProgressPhase + "\"");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
