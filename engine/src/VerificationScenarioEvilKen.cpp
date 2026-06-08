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
    bool sawFallingVelocity = false;
    int firstAirRecoveryState = 0;
    int firstAirRecoveryFrame = 0;
    float firstAirRecoveryY = 0.0f;
    bool leftTripShake = false;
    int firstAfterTripShakeState = 0;
    int firstAfterTripShakeFrame = 0;
    float firstAfterTripShakeY = 0.0f;
    float firstAfterTripShakeVy = 0.0f;
    float minTripY = 0.0f;
    int tripAirArcCount = 0;
    int tripFloorContactCount = 0;
    bool inTripAirArc = false;
    bool inTripFloorContact = false;
    FighterSnapshot finalP2;
    std::string lastHitText;
    for (int i = 0; i < 180; ++i) {
        const auto snap = runtime.snapshot();
        sawState420 = sawState420 || snap.p1.stateNo == 420 || snap.p1.action == 420;
        lastHitText = snap.lastHitText.empty() ? lastHitText : snap.lastHitText;
        if (snap.p2.stateNo == 5070) {
            sawTripShake = true;
        }
        if (sawTripShake && snap.p2.stateNo != 5070 && !leftTripShake) {
            leftTripShake = true;
            firstAfterTripShakeState = snap.p2.stateNo;
            firstAfterTripShakeFrame = i;
            firstAfterTripShakeY = snap.p2.y;
            firstAfterTripShakeVy = snap.p2.vy;
        }
        if (snap.p2.stateNo == 5071) {
            sawTripFall = true;
        }
        if (snap.p2.stateNo == 5070 || snap.p2.stateNo == 5071) {
            minTripY = std::min(minTripY, snap.p2.y);
            sawFallingVelocity = sawFallingVelocity || snap.p2.vy > 0.05f;
        }
        if (snap.p2.stateNo == 5071 && !snap.p2.onGround && snap.p2.vy < -0.05f) {
            if (!inTripAirArc) {
                ++tripAirArcCount;
            }
            inTripAirArc = true;
        }
        if (snap.p2.stateNo != 5071 || snap.p2.onGround) {
            inTripAirArc = false;
        }
        if (snap.p2.stateNo == 5071 && snap.p2.onGround) {
            if (!inTripFloorContact) {
                ++tripFloorContactCount;
            }
            inTripFloorContact = true;
        } else {
            inTripFloorContact = false;
        }
        const bool inAirRecovery = snap.p2.stateNo == 5040 || snap.p2.stateNo == 5140
            || snap.p2.stateNo == 5200 || snap.p2.stateNo == 5210
            || (snap.p2.stateNo >= 2004 && snap.p2.stateNo <= 2006);
        if (inAirRecovery && !sawAirRecovery) {
            firstAirRecoveryState = snap.p2.stateNo;
            firstAirRecoveryFrame = i;
            firstAirRecoveryY = snap.p2.y;
        }
        sawAirRecovery = sawAirRecovery || inAirRecovery;
        sawGrounded = sawGrounded || snap.p2.stateNo == 5110 || snap.p2.stateNo == 5120
            || (snap.p2.onGround && snap.p2.stateType == 'L');
        finalP2 = snap.p2;
        runtime.step({}, 1);
    }

    record(out, counts, sawState420 ? Status::Pass : Status::Fail, "evilken_crouch_roundhouse_started",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawTripShake ? Status::Pass : Status::Fail, "trip_shake_observed",
        "saw_5070=" + std::to_string(sawTripShake ? 1 : 0)
        + " min_trip_y=" + std::to_string(minTripY));
    const bool lowParabola = minTripY <= -2.0f && minTripY >= -16.0f && sawFallingVelocity;
    record(out, counts, lowParabola ? Status::Pass : Status::Fail, "trip_low_parabola_observed",
        "min_trip_y=" + std::to_string(minTripY)
        + " saw_falling_velocity=" + std::to_string(sawFallingVelocity ? 1 : 0));
    const bool threeParabolas = tripAirArcCount >= 3 && tripFloorContactCount >= 3;
    record(out, counts, threeParabolas ? Status::Pass : Status::Fail, "trip_three_parabolas_observed",
        "air_arcs=" + std::to_string(tripAirArcCount)
        + " floor_contacts=" + std::to_string(tripFloorContactCount)
        + " min_trip_y=" + std::to_string(minTripY));
    record(out, counts, sawTripFall || sawGrounded ? Status::Pass : Status::Fail, "trip_resolves_after_shake",
        "saw_5071=" + std::to_string(sawTripFall ? 1 : 0)
        + " grounded=" + std::to_string(sawGrounded ? 1 : 0)
        + " first_after_5070_state=" + std::to_string(firstAfterTripShakeState)
        + " first_after_5070_frame=" + std::to_string(firstAfterTripShakeFrame)
        + " first_after_5070_y=" + std::to_string(firstAfterTripShakeY)
        + " first_after_5070_vy=" + std::to_string(firstAfterTripShakeVy)
        + " min_trip_y=" + std::to_string(minTripY)
        + " final_state=" + std::to_string(finalP2.stateNo)
        + " final_y=" + std::to_string(finalP2.y));
    record(out, counts, !sawAirRecovery ? Status::Pass : Status::Fail, "trip_does_not_air_recover",
        "air_recovery=" + std::to_string(sawAirRecovery ? 1 : 0)
        + " first_air_recovery_state=" + std::to_string(firstAirRecoveryState)
        + " first_air_recovery_frame=" + std::to_string(firstAirRecoveryFrame)
        + " first_air_recovery_y=" + std::to_string(firstAirRecoveryY)
        + " final_state=" + std::to_string(finalP2.stateNo));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
