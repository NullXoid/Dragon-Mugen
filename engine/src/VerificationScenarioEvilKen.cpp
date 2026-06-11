#include "VerificationScenario.h"

#include <algorithm>
#include <cmath>
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
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.step({}, 2);
    waitForControllableIdle(runtime, 120);
    runtime.setFighterControl(1, false);
    runtime.forceFighterState(0, 420);

    bool sawState420 = false;
    bool sawTripShake = false;
    bool sawTripShakeState = false;
    bool sawTripShakeAction = false;
    bool sawTripFall = false;
    bool sawGrounded = false;
    bool sawAirRecovery = false;
    bool sawUpwardVelocity = false;
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
    float maxTripY = 0.0f;
    float maxTripShakeAbsY = 0.0f;
    float maxTripShakeAbsVx = 0.0f;
    float maxTripShakeAbsVy = 0.0f;
    int tripShakeFrames = 0;
    int tripFallFrames = 0;
    int firstLiedownFrame = -1;
    int firstLiedownImpactFrame = -1;
    int firstLiedownRestFrame = -1;
    int firstGetupFrame = -1;
    FighterSnapshot finalP2;
    std::string lastHitText;
    for (int i = 0; i < 240; ++i) {
        const auto snap = runtime.snapshot();
        sawState420 = sawState420 || snap.p1.stateNo == 420 || snap.p1.action == 420;
        lastHitText = snap.lastHitText.empty() ? lastHitText : snap.lastHitText;
        const bool tripShakeVisual = snap.p2.stateNo == 5070
            || (snap.p2.action == 5070 && !sawTripFall && snap.p2.stateNo != 5071);
        if (tripShakeVisual) {
            sawTripShake = true;
            sawTripShakeState = sawTripShakeState || snap.p2.stateNo == 5070;
            sawTripShakeAction = sawTripShakeAction || snap.p2.action == 5070;
            ++tripShakeFrames;
            maxTripShakeAbsY = std::max(maxTripShakeAbsY, std::abs(snap.p2.y));
            maxTripShakeAbsVx = std::max(maxTripShakeAbsVx, std::abs(snap.p2.vx));
            maxTripShakeAbsVy = std::max(maxTripShakeAbsVy, std::abs(snap.p2.vy));
        }
        if (sawTripShake && !tripShakeVisual && !leftTripShake) {
            leftTripShake = true;
            firstAfterTripShakeState = snap.p2.stateNo;
            firstAfterTripShakeFrame = i;
            firstAfterTripShakeY = snap.p2.y;
            firstAfterTripShakeVy = snap.p2.vy;
        }
        if (snap.p2.stateNo == 5071) {
            sawTripFall = true;
            ++tripFallFrames;
            minTripY = std::min(minTripY, snap.p2.y);
            maxTripY = std::max(maxTripY, snap.p2.y);
            sawUpwardVelocity = sawUpwardVelocity || snap.p2.vy < -0.05f;
            sawFallingVelocity = sawFallingVelocity || snap.p2.vy > 0.05f;
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
        if (snap.p2.stateNo == 5110 && firstLiedownFrame < 0) {
            firstLiedownFrame = i;
        }
        if (snap.p2.stateNo == 5110 && snap.p2.action >= 5170 && snap.p2.action <= 5179 && firstLiedownImpactFrame < 0) {
            firstLiedownImpactFrame = i;
        }
        if (snap.p2.stateNo == 5110 && snap.p2.action >= 5110 && snap.p2.action <= 5119 && firstLiedownRestFrame < 0) {
            firstLiedownRestFrame = i;
        }
        if (snap.p2.stateNo == 5120 && firstGetupFrame < 0) {
            firstGetupFrame = i;
        }
        finalP2 = snap.p2;
        runtime.step({}, 1);
    }

    record(out, counts, sawState420 ? Status::Pass : Status::Fail, "evilken_crouch_roundhouse_started",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawTripShake ? Status::Pass : Status::Fail, "trip_shake_observed",
        "saw_5070_visual=" + std::to_string(sawTripShake ? 1 : 0)
        + " saw_5070_state=" + std::to_string(sawTripShakeState ? 1 : 0)
        + " saw_5070_action=" + std::to_string(sawTripShakeAction ? 1 : 0)
        + " shake_frames=" + std::to_string(tripShakeFrames)
        + " max_shake_abs_y=" + std::to_string(maxTripShakeAbsY));
    const bool tripShakeHeldStill = sawTripShake
        && maxTripShakeAbsY <= 0.05f
        && maxTripShakeAbsVx <= 0.05f
        && maxTripShakeAbsVy <= 0.05f;
    record(out, counts, tripShakeHeldStill ? Status::Pass : Status::Fail, "trip_shake_holds_common_pose",
        "shake_frames=" + std::to_string(tripShakeFrames)
        + " max_abs_y=" + std::to_string(maxTripShakeAbsY)
        + " max_abs_vx=" + std::to_string(maxTripShakeAbsVx)
        + " max_abs_vy=" + std::to_string(maxTripShakeAbsVy));
    record(out, counts, sawTripFall ? Status::Pass : Status::Fail, "trip_fall_state_observed",
        "saw_5071=" + std::to_string(sawTripFall ? 1 : 0)
        + " fall_frames=" + std::to_string(tripFallFrames));
    const bool authoredFallMotion = sawTripFall
        && sawUpwardVelocity
        && sawFallingVelocity
        && minTripY <= -1.0f
        && minTripY >= -80.0f;
    record(out, counts, authoredFallMotion ? Status::Pass : Status::Fail, "trip_uses_hit_velocity_and_yaccel",
        "min_trip_y=" + std::to_string(minTripY)
        + " max_trip_y=" + std::to_string(maxTripY)
        + " saw_upward_velocity=" + std::to_string(sawUpwardVelocity ? 1 : 0)
        + " saw_falling_velocity=" + std::to_string(sawFallingVelocity ? 1 : 0));
    record(out, counts, (sawTripFall || sawGrounded) ? Status::Pass : Status::Fail, "trip_resolves_after_shake",
        "saw_5071=" + std::to_string(sawTripFall ? 1 : 0)
        + " grounded=" + std::to_string(sawGrounded ? 1 : 0)
        + " first_after_5070_state=" + std::to_string(firstAfterTripShakeState)
        + " first_after_5070_frame=" + std::to_string(firstAfterTripShakeFrame)
        + " first_after_5070_y=" + std::to_string(firstAfterTripShakeY)
        + " first_after_5070_vy=" + std::to_string(firstAfterTripShakeVy)
        + " min_trip_y=" + std::to_string(minTripY)
        + " final_state=" + std::to_string(finalP2.stateNo)
        + " final_y=" + std::to_string(finalP2.y));
    record(out, counts, sawGrounded ? Status::Pass : Status::Fail, "trip_reaches_ground_state",
        "grounded=" + std::to_string(sawGrounded ? 1 : 0)
        + " final_state=" + std::to_string(finalP2.stateNo)
        + " final_y=" + std::to_string(finalP2.y)
        + " final_on_ground=" + std::to_string(finalP2.onGround ? 1 : 0));
    const bool usedLiedownImpactAnim = firstLiedownImpactFrame >= 0
        && (firstLiedownRestFrame < 0 || firstLiedownImpactFrame <= firstLiedownRestFrame);
    record(out, counts, usedLiedownImpactAnim ? Status::Pass : Status::Fail, "trip_plays_liedown_impact_anim",
        "first_517x_frame=" + std::to_string(firstLiedownImpactFrame)
        + " first_511x_frame=" + std::to_string(firstLiedownRestFrame)
        + " first_getup_frame=" + std::to_string(firstGetupFrame));
    const int liedownFramesBeforeGetup = firstLiedownFrame >= 0 && firstGetupFrame >= 0
        ? firstGetupFrame - firstLiedownFrame
        : -1;
    const bool usedLiedownTimer = firstLiedownFrame >= 0
        && firstGetupFrame >= 0
        && liedownFramesBeforeGetup <= 45;
    record(out, counts, usedLiedownTimer ? Status::Pass : Status::Fail, "trip_uses_liedown_time_for_getup",
        "first_liedown_frame=" + std::to_string(firstLiedownFrame)
        + " first_getup_frame=" + std::to_string(firstGetupFrame)
        + " liedown_frames_before_getup=" + std::to_string(liedownFramesBeforeGetup));
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
