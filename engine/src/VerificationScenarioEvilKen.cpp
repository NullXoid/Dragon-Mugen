#include "VerificationScenario.h"

#include <algorithm>
#include <ostream>

namespace dragon::verification {
namespace {

enum class Status {
    Pass,
    Partial,
    Fail,
    Blocked,
};

struct Counts {
    int pass = 0;
    int partial = 0;
    int fail = 0;
    int blocked = 0;
};

const char* statusText(Status status) {
    switch (status) {
    case Status::Pass:
        return "PASS";
    case Status::Partial:
        return "PARTIAL";
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
    } else if (status == Status::Partial) {
        ++counts.partial;
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
    out << "SUMMARY pass=" << counts.pass << " partial=" << counts.partial << " fail=" << counts.fail
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

int runEvilKenTripGrounding(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-trip-grounding");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-12.0f, 14.0f);
    waitForControllableIdle(runtime, 120);
    runtime.setFighterControl(1, false);
    runtime.forceFighterState(0, 420);

    bool sawState420 = false;
    bool sawTripShake = false;
    bool sawTripFall = false;
    bool sawGrounded = false;
    bool sawAirRecovery = false;
    float minTripShakeY = 0.0f;
    float minTripFallY = 0.0f;
    FighterSnapshot finalP2;
    std::string lastHitText;
    for (int i = 0; i < 100; ++i) {
        const auto snap = runtime.snapshot();
        sawState420 = sawState420 || snap.p1.stateNo == 420 || snap.p1.action == 420;
        lastHitText = snap.lastHitText.empty() ? lastHitText : snap.lastHitText;
        if (snap.p2.stateNo == 5070) {
            sawTripShake = true;
            minTripShakeY = std::min(minTripShakeY, snap.p2.y);
        }
        if (snap.p2.stateNo == 5071) {
            sawTripFall = true;
            minTripFallY = std::min(minTripFallY, snap.p2.y);
        }
        sawAirRecovery = sawAirRecovery || snap.p2.stateNo == 5040 || snap.p2.stateNo == 5140
            || snap.p2.stateNo == 5200 || snap.p2.stateNo == 5210
            || (snap.p2.stateNo >= 2004 && snap.p2.stateNo <= 2006);
        sawGrounded = sawGrounded || snap.p2.stateNo == 5110 || snap.p2.stateNo == 5120
            || (snap.p2.onGround && snap.p2.stateType == 'L');
        finalP2 = snap.p2;
        runtime.step({}, 1);
    }

    record(out, counts, sawState420 ? Status::Pass : Status::Fail, "evilken_crouch_roundhouse_started",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawTripShake ? Status::Pass : Status::Fail, "trip_shake_observed",
        "saw_5070=" + std::to_string(sawTripShake ? 1 : 0)
        + " min_5070_y=" + std::to_string(minTripShakeY));
    record(out, counts, minTripShakeY >= -0.5f ? Status::Pass : Status::Fail, "trip_shake_stays_grounded",
        "min_5070_y=" + std::to_string(minTripShakeY));
    record(out, counts, sawTripFall || sawGrounded ? Status::Pass : Status::Fail, "trip_resolves_after_shake",
        "saw_5071=" + std::to_string(sawTripFall ? 1 : 0)
        + " grounded=" + std::to_string(sawGrounded ? 1 : 0)
        + " min_5071_y=" + std::to_string(minTripFallY)
        + " final_state=" + std::to_string(finalP2.stateNo)
        + " final_y=" + std::to_string(finalP2.y));
    record(out, counts, !sawAirRecovery ? Status::Pass : Status::Fail, "trip_does_not_air_recover",
        "air_recovery=" + std::to_string(sawAirRecovery ? 1 : 0)
        + " final_state=" + std::to_string(finalP2.stateNo));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
