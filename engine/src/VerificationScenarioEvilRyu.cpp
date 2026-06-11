#include "VerificationScenario.h"

#include <algorithm>
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

} // namespace

int runEvilRyuDash(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "EvilRyu/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-dash");
    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));

    runtime.positionFighters(-260.0f, 520.0f);
    SymbolicInput forward;
    forward.right = true;
    const float startX = runtime.snapshot().p1.x;
    runtime.step(forward, 2);
    runtime.step({}, 2);
    runtime.step(forward, 2);
    bool sawDash = false;
    bool sawMovingDash = false;
    int state101Frames = 0;
    int state102Frames = 0;
    int startupRun = 0;
    int maxStartupRun = 0;
    int maxForwardMoveStateTime = 0;
    float maxX = startX;
    float minForwardDisplayOffsetY = 0.0f;
    for (int i = 0; i < 45; ++i) {
        const auto p1 = runtime.snapshot().p1;
        sawDash = sawDash || p1.stateNo == 100 || p1.stateNo == 101 || p1.stateNo == 102;
        sawMovingDash = sawMovingDash || p1.stateNo == 101 || p1.stateNo == 102;
        if (p1.stateNo == 101) {
            ++state101Frames;
            maxForwardMoveStateTime = std::max(maxForwardMoveStateTime, p1.stateTime);
        }
        if (p1.stateNo == 102) {
            ++state102Frames;
        }
        startupRun = p1.stateNo == 100 ? startupRun + 1 : 0;
        maxStartupRun = std::max(maxStartupRun, startupRun);
        maxX = std::max(maxX, p1.x);
        minForwardDisplayOffsetY = std::min(minForwardDisplayOffsetY, p1.displayOffsetY);
        runtime.step(forward, 1);
    }
    const auto end = runtime.snapshot().p1;
    const bool progressed = sawDash
        && sawMovingDash
        && maxStartupRun <= 18
        && maxX > startX + 20.0f
        && minForwardDisplayOffsetY < -6.0f;
    record(out, counts, progressed ? Status::Pass : Status::Fail, "forward_dash_progresses",
        "start_x=" + std::to_string(startX) + " max_x=" + std::to_string(maxX)
        + " final_state=" + std::to_string(end.stateNo)
        + " max_startup_frames=" + std::to_string(maxStartupRun)
        + " state101_frames=" + std::to_string(state101Frames)
        + " state102_frames=" + std::to_string(state102Frames)
        + " max_move_state_time=" + std::to_string(maxForwardMoveStateTime)
        + " min_display_offset_y=" + std::to_string(minForwardDisplayOffsetY));

    runtime.positionFighters(-260.0f, 520.0f);
    waitForControllableIdle(runtime, 240);
    SymbolicInput back;
    back.left = true;
    const float backStartX = runtime.snapshot().p1.x;
    runtime.step(back, 2);
    runtime.step({}, 2);
    runtime.step(back, 2);
    bool sawBackDash = false;
    bool sawMovingBackDash = false;
    int state106Frames = 0;
    int state107Frames = 0;
    int startupBack = 0;
    int maxBackStartup = 0;
    int maxBackMoveStateTime = 0;
    float minX = backStartX;
    float minBackDisplayOffsetY = 0.0f;
    for (int i = 0; i < 45; ++i) {
        const auto p1 = runtime.snapshot().p1;
        sawBackDash = sawBackDash || p1.stateNo == 105 || p1.stateNo == 106 || p1.stateNo == 107;
        sawMovingBackDash = sawMovingBackDash || p1.stateNo == 106 || p1.stateNo == 107;
        if (p1.stateNo == 106) {
            ++state106Frames;
            maxBackMoveStateTime = std::max(maxBackMoveStateTime, p1.stateTime);
        }
        if (p1.stateNo == 107) {
            ++state107Frames;
        }
        startupBack = p1.stateNo == 105 ? startupBack + 1 : 0;
        maxBackStartup = std::max(maxBackStartup, startupBack);
        minX = std::min(minX, p1.x);
        minBackDisplayOffsetY = std::min(minBackDisplayOffsetY, p1.displayOffsetY);
        runtime.step(back, 1);
    }
    const auto backEnd = runtime.snapshot().p1;
    const bool backedUp = sawBackDash
        && sawMovingBackDash
        && maxBackStartup <= 18
        && minX < backStartX - 20.0f
        && minBackDisplayOffsetY < -8.0f;
    record(out, counts, backedUp ? Status::Pass : Status::Fail, "back_dash_progresses",
        "start_x=" + std::to_string(backStartX) + " min_x=" + std::to_string(minX)
        + " final_state=" + std::to_string(backEnd.stateNo)
        + " max_startup_frames=" + std::to_string(maxBackStartup)
        + " state106_frames=" + std::to_string(state106Frames)
        + " state107_frames=" + std::to_string(state107Frames)
        + " max_move_state_time=" + std::to_string(maxBackMoveStateTime)
        + " min_display_offset_y=" + std::to_string(minBackDisplayOffsetY));
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
