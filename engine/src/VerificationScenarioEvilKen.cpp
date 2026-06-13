#include "VerificationScenario.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

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

std::string moveFailureText(const TrainingMoveInfo& move, const FighterSnapshot& p2, std::string_view commands) {
    std::ostringstream text;
    text << move.label << " [" << move.input << " -> " << move.targetState << "]"
         << " final_state=" << p2.stateNo
         << " final_action=" << p2.action
         << " final_time=" << p2.stateTime
         << " anim_tick=" << p2.animTick
         << " y=" << p2.y
         << " vy=" << p2.vy
         << " state_type=" << p2.stateType
         << " physics=" << p2.physics
         << " move_type=" << p2.moveType
         << " hitpause=" << p2.hitPauseTicks
         << " ground=" << (p2.onGround ? 1 : 0)
         << " ctrl=" << (p2.ctrl ? 1 : 0)
         << " commands=" << commands;
    return text.str();
}

std::string joinLimited(const std::vector<std::string>& values, size_t limit = 80) {
    if (values.empty()) {
        return "-";
    }
    std::string text;
    const size_t count = std::min(values.size(), limit);
    for (size_t i = 0; i < count; ++i) {
        if (!text.empty()) {
            text += " | ";
        }
        text += values[i];
    }
    if (values.size() > limit) {
        text += " | +" + std::to_string(values.size() - limit) + " more";
    }
    return text;
}

bool commaListContains(std::string_view text, std::string_view needle) {
    size_t start = 0;
    while (start <= text.size()) {
        const size_t comma = text.find(',', start);
        const size_t end = comma == std::string_view::npos ? text.size() : comma;
        if (text.substr(start, end - start) == needle) {
            return true;
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return false;
}

bool commandListMatchesExpected(const TrainingMoveInfo& move, std::string_view commands) {
    for (const auto& command : move.commandNames) {
        if (commaListContains(commands, command)) {
            return true;
        }
    }
    return false;
}

void appendUniqueText(std::vector<std::string>& values, const std::string& value, size_t limit = 12) {
    if (value.empty() || values.size() >= limit) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

std::string stateTraceText(const FighterSnapshot& fighter) {
    if (fighter.stateNo == 0 || fighter.stateNo == 20) {
        return {};
    }
    return std::to_string(fighter.stateNo) + "/" + std::to_string(fighter.action);
}

bool snapshotMatchesTrainingMoveTarget(const TrainingMoveInfo& move, const FighterSnapshot& fighter) {
    if (move.targetState >= 0) {
        return fighter.stateNo == move.targetState;
    }
    return fighter.stateNo != 0
        && fighter.stateNo != 20
        && fighter.stateNo != 40
        && fighter.stateNo != 41
        && fighter.stateNo != 42
        && fighter.stateNo != 45
        && fighter.stateNo != 50
        && fighter.stateNo != 51
        && fighter.stateNo != 52
        && fighter.moveType != 'I';
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

int runEvilKenOverheadTripChain(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Versus, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Versus setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-overhead-trip-chain");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-18.0f, 18.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.step({}, 2);
    runtime.forceFighterState(0, 1832);
    runtime.setFighterPosition(0, -18.0f, 0.0f);
    runtime.setFighterPosition(1, 18.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool sawFirstHit = false;
    bool sawTripHit = false;
    bool sawAttackerFollowup = false;
    bool sawAttackerLanding = false;
    bool sawDefenderFall = false;
    bool sawDefenderGrounded = false;
    bool sawDefenderGetup = false;
    bool sawDefenderIdle = false;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    std::string lastHitText;
    std::string firstHitText;
    std::string tripHitText;
    for (int i = 0; i < 360; ++i) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        sawAttackerFollowup = sawAttackerFollowup || snap.p1.stateNo == 1834;
        sawAttackerLanding = sawAttackerLanding || (snap.p1.onGround && snap.p1.stateNo != 1832 && snap.p1.stateNo != 1834);
        sawDefenderFall = sawDefenderFall || snap.p2.stateNo == 5050 || snap.p2.stateNo == 5071 || snap.p2.stateNo == 5100 || snap.p2.stateNo == 5101;
        sawDefenderGrounded = sawDefenderGrounded || snap.p2.stateNo == 5110 || snap.p2.stateNo == 5120
            || (snap.p2.onGround && snap.p2.stateType == 'L');
        sawDefenderGetup = sawDefenderGetup || snap.p2.stateNo == 5120;
        sawDefenderIdle = sawDefenderIdle || (snap.p2.stateNo == 0 && snap.p2.onGround && snap.p2.moveType == 'I');
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
            if (snap.lastHitText.find("P1 hit 1832#") != std::string::npos) {
                sawFirstHit = true;
                firstHitText = snap.lastHitText;
            }
            if (snap.lastHitText.find("P1 hit 1834#") != std::string::npos) {
                sawTripHit = true;
                tripHitText = snap.lastHitText;
            }
        }
        runtime.step({}, 1);
    }

    record(out, counts, sawFirstHit ? Status::Pass : Status::Fail, "overhead_hit_connected",
        "first_hit=\"" + firstHitText + "\" last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawAttackerFollowup ? Status::Pass : Status::Fail, "attacker_reached_trip_followup",
        "final_p1_state=" + std::to_string(finalP1.stateNo)
        + " final_p1_action=" + std::to_string(finalP1.action)
        + " final_p1_y=" + std::to_string(finalP1.y));
    record(out, counts, sawTripHit ? Status::Pass : Status::Fail, "trip_followup_connected",
        "trip_hit=\"" + tripHitText + "\" last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawDefenderFall ? Status::Pass : Status::Fail, "defender_entered_fall",
        "final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_action=" + std::to_string(finalP2.action)
        + " final_p2_y=" + std::to_string(finalP2.y));
    record(out, counts, sawDefenderGrounded ? Status::Pass : Status::Fail, "defender_reached_ground_recovery",
        "grounded=" + std::to_string(sawDefenderGrounded ? 1 : 0)
        + " saw_getup=" + std::to_string(sawDefenderGetup ? 1 : 0)
        + " final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_time=" + std::to_string(finalP2.stateTime));
    record(out, counts, sawDefenderIdle ? Status::Pass : Status::Fail, "defender_exited_trip_recovery",
        "idle=" + std::to_string(sawDefenderIdle ? 1 : 0)
        + " final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_action=" + std::to_string(finalP2.action)
        + " final_p2_ground=" + std::to_string(finalP2.onGround ? 1 : 0));
    record(out, counts, sawAttackerLanding && finalP1.stateNo != 1834 ? Status::Pass : Status::Fail, "attacker_not_stuck_after_followup",
        "landed=" + std::to_string(sawAttackerLanding ? 1 : 0)
        + " final_p1_state=" + std::to_string(finalP1.stateNo)
        + " final_p1_action=" + std::to_string(finalP1.action)
        + " final_p1_ground=" + std::to_string(finalP1.onGround ? 1 : 0));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenOverheadTripChainStress(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-overhead-trip-chain-stress");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    int normalTripHits = 0;
    int downedTripHits = 0;
    int commonAirFallFrames = 0;
    int downedAirFall5090Frames = 0;
    int groundRecoveryFrames = 0;
    float maxP2Y = -100000.0f;
    float minP2Y = 100000.0f;
    std::string lastHitText;

    const auto sample = [&](const RuntimeSnapshot& snap) {
        maxP2Y = std::max(maxP2Y, snap.p2.y);
        minP2Y = std::min(minP2Y, snap.p2.y);
        if ((snap.p2.stateNo == 5030 || snap.p2.stateNo == 5035 || snap.p2.stateNo == 5050) && snap.p2.moveType == 'H') {
            ++commonAirFallFrames;
        }
        if (snap.p2.stateNo == 5030 && snap.p2.action == 5090) {
            ++downedAirFall5090Frames;
        }
        if (snap.p2.stateNo == 5110 || snap.p2.stateNo == 5120 || (snap.p2.onGround && snap.p2.stateType == 'L')) {
            ++groundRecoveryFrames;
        }
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
    };

    for (int cycle = 0; cycle < 4; ++cycle) {
        runtime.positionFighters(-18.0f, 18.0f);
        runtime.forceFighterState(0, 0);
        runtime.forceFighterState(1, 0);
        runtime.setFighterPosition(0, -18.0f, 0.0f);
        runtime.setFighterPosition(1, 18.0f, 0.0f);
        runtime.setFighterControl(0, false);
        runtime.setFighterControl(1, false);
        runtime.step({}, 2);
        runtime.forceFighterState(0, 1832);
        runtime.setFighterPosition(0, -18.0f, 0.0f);
        runtime.setFighterPosition(1, 18.0f, 0.0f);
        runtime.setFighterControl(0, false);
        runtime.setFighterControl(1, false);

        bool sawTripThisCycle = false;
        for (int frame = 0; frame < 150; ++frame) {
            const auto snap = runtime.snapshot();
            sample(snap);
            if (snap.lastHitText.find("P1 hit 1834#") != std::string::npos) {
                sawTripThisCycle = true;
            }
            runtime.step({}, 1);
        }
        if (sawTripThisCycle) {
            ++normalTripHits;
        }
    }

    for (int cycle = 0; cycle < 8; ++cycle) {
        runtime.forceFighterState(0, 0);
        runtime.forceFighterLiedown(1, 90);
        runtime.setFighterPosition(0, -18.0f, 0.0f);
        runtime.setFighterPosition(1, 18.0f, 0.0f);
        runtime.setFighterControl(0, false);
        runtime.setFighterControl(1, false);
        runtime.step({}, 2);
        runtime.forceFighterState(0, 1834);
        runtime.setFighterPosition(0, -18.0f, 0.0f);
        runtime.setFighterControl(0, false);
        runtime.setFighterControl(1, false);

        bool sawTripThisCycle = false;
        for (int frame = 0; frame < 120; ++frame) {
            const auto snap = runtime.snapshot();
            sample(snap);
            if (snap.lastHitText.find("P1 hit 1834#") != std::string::npos) {
                sawTripThisCycle = true;
            }
            runtime.step({}, 1);
        }
        if (sawTripThisCycle) {
            ++downedTripHits;
        }
    }

    bool finalRecovered = false;
    FighterSnapshot finalP2;
    for (int frame = 0; frame < 420; ++frame) {
        const auto snap = runtime.snapshot();
        sample(snap);
        finalP2 = snap.p2;
        if (snap.p2.stateNo == 0 && snap.p2.onGround && snap.p2.moveType == 'I') {
            finalRecovered = true;
            break;
        }
        runtime.step({}, 1);
    }
    finalP2 = runtime.snapshot().p2;

    record(out, counts, normalTripHits >= 3 ? Status::Pass : Status::Fail, "normal_chain_trip_hits_repeated",
        "normal_trip_hits=" + std::to_string(normalTripHits)
        + " last_hit=\"" + lastHitText + "\"");
    record(out, counts, downedTripHits >= 6 ? Status::Pass : Status::Fail, "downed_trip_hits_repeated",
        "downed_trip_hits=" + std::to_string(downedTripHits)
        + " common_airfall_frames=" + std::to_string(commonAirFallFrames)
        + " exact_5030_5090_frames=" + std::to_string(downedAirFall5090Frames)
        + " last_hit=\"" + lastHitText + "\"");
    record(out, counts, commonAirFallFrames > 0 ? Status::Pass : Status::Fail, "stress_exercised_common_airfall_path",
        "common_airfall_frames=" + std::to_string(commonAirFallFrames)
        + " exact_5030_5090_frames=" + std::to_string(downedAirFall5090Frames));
    record(out, counts, groundRecoveryFrames > 0 ? Status::Pass : Status::Fail, "defender_reached_ground_recovery_under_stress",
        "ground_recovery_frames=" + std::to_string(groundRecoveryFrames)
        + " final_state=" + std::to_string(finalP2.stateNo)
        + " final_y=" + std::to_string(finalP2.y));
    record(out, counts, maxP2Y <= 64.0f ? Status::Pass : Status::Fail, "defender_did_not_fall_offscreen",
        "max_p2_y=" + std::to_string(maxP2Y)
        + " min_p2_y=" + std::to_string(minP2Y)
        + " final_state=" + std::to_string(finalP2.stateNo)
        + " final_action=" + std::to_string(finalP2.action)
        + " final_y=" + std::to_string(finalP2.y));
    record(out, counts, finalRecovered ? Status::Pass : Status::Fail, "defender_recovered_after_stress",
        "recovered=" + std::to_string(finalRecovered ? 1 : 0)
        + " final_state=" + std::to_string(finalP2.stateNo)
        + " final_action=" + std::to_string(finalP2.action)
        + " final_ground=" + std::to_string(finalP2.onGround ? 1 : 0));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenThrow(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-throw");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-4.0f, 4.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -4.0f, 0.0f);
    runtime.setFighterPosition(1, 4.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.step({}, 2);
    const int p2LifeBefore = runtime.snapshot().p2.life;
    runtime.forceFighterState(0, 900);
    runtime.setFighterPosition(0, -4.0f, 0.0f);
    runtime.setFighterPosition(1, 4.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool sawThrowHit = false;
    bool sawThrowAttemptState = false;
    bool sawP1ThrowState = false;
    bool sawP2CustomGrabState = false;
    bool sawBoundTarget = false;
    bool sawTargetLifeAdd = false;
    bool sawTargetReleaseState = false;
    bool sawP2SelfStateFall = false;
    bool p1Recovered = false;
    bool p2Recovered = false;
    float closestBoundDistance = 100000.0f;
    float closestAttemptDistance = 100000.0f;
    int maxAttemptStateTime = -1;
    int maxAttemptAnimTick = -1;
    int activeP1AnimElem = -1;
    int activeP1Clsn1Count = -1;
    int activeP2Clsn2Count = -1;
    bool activeBoxesOverlap = false;
    bool activeHitDefFound = false;
    bool activeHitFlagAllows = false;
    bool activeDefenderHittable = false;
    float activeP2BodyDistX = -1.0f;
    bool sawThrowAction = false;
    bool sawThrowActiveWindow = false;
    char attemptP1MoveType = '-';
    char attemptP2StateType = '-';
    std::string lastHitText;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    for (int frame = 0; frame < 520; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
        sawThrowHit = sawThrowHit || snap.lastHitText.find("P1 hit 900#") != std::string::npos;
        sawThrowAttemptState = sawThrowAttemptState || snap.p1.stateNo == 900;
        if (snap.p1.stateNo == 900) {
            sawThrowAction = sawThrowAction || snap.p1.action == 900;
            sawThrowActiveWindow = sawThrowActiveWindow
                || (snap.p1.action == 900 && snap.p1.animTick >= 3 && snap.p1.animTick <= 4);
            if (snap.p1.action == 900 && snap.p1.animTick >= 3 && snap.p1.animTick <= 4) {
                activeP1AnimElem = snap.p1AnimElem;
                activeP1Clsn1Count = snap.p1Clsn1Count;
                activeP2Clsn2Count = snap.p2Clsn2Count;
                activeBoxesOverlap = activeBoxesOverlap || snap.p1P2BoxesOverlap;
                activeHitDefFound = activeHitDefFound || snap.p1ActiveHitDef;
                activeHitFlagAllows = activeHitFlagAllows || snap.p1HitFlagAllowsP2;
                activeDefenderHittable = activeDefenderHittable || snap.p2HittableByP1;
                activeP2BodyDistX = snap.p1P2BodyDistX;
            }
            maxAttemptStateTime = std::max(maxAttemptStateTime, snap.p1.stateTime);
            maxAttemptAnimTick = std::max(maxAttemptAnimTick, snap.p1.animTick);
            closestAttemptDistance = std::min(closestAttemptDistance, std::fabs(snap.p2.x - snap.p1.x));
            attemptP1MoveType = snap.p1.moveType;
            attemptP2StateType = snap.p2.stateType;
        }
        sawP1ThrowState = sawP1ThrowState || snap.p1.stateNo == 920;
        sawP2CustomGrabState = sawP2CustomGrabState || snap.p2.stateNo == 925;
        sawTargetReleaseState = sawTargetReleaseState || snap.p2.stateNo == 926;
        sawTargetLifeAdd = sawTargetLifeAdd || snap.p2.life < p2LifeBefore;
        if (snap.p1.stateNo == 920 && (snap.p2.stateNo == 925 || snap.p2.stateNo == 926)) {
            const float distance = std::fabs(snap.p2.x - snap.p1.x);
            closestBoundDistance = std::min(closestBoundDistance, distance);
            sawBoundTarget = sawBoundTarget || distance <= 90.0f;
        }
        sawP2SelfStateFall = sawP2SelfStateFall || snap.p2.stateNo == 5050 || snap.p2.stateNo == 5100
            || snap.p2.stateNo == 5110 || snap.p2.stateNo == 5120;
        p1Recovered = p1Recovered || (snap.p1.stateNo == 0 && snap.p1.onGround && snap.p1.moveType == 'I');
        p2Recovered = p2Recovered || (snap.p2.stateNo == 0 && snap.p2.onGround && snap.p2.moveType == 'I');
        runtime.step({}, 1);
    }

    record(out, counts, sawThrowHit ? Status::Pass : Status::Fail, "throw_hitdef_connected",
        "attempt_state_seen=" + std::to_string(sawThrowAttemptState ? 1 : 0)
        + " action900_seen=" + std::to_string(sawThrowAction ? 1 : 0)
        + " active_window_seen=" + std::to_string(sawThrowActiveWindow ? 1 : 0)
        + " max_state_time=" + std::to_string(maxAttemptStateTime)
        + " max_anim_tick=" + std::to_string(maxAttemptAnimTick)
        + " active_elem=" + std::to_string(activeP1AnimElem)
        + " active_clsn1=" + std::to_string(activeP1Clsn1Count)
        + " active_p2_clsn2=" + std::to_string(activeP2Clsn2Count)
        + " active_overlap=" + std::to_string(activeBoxesOverlap ? 1 : 0)
        + " active_hitdef=" + std::to_string(activeHitDefFound ? 1 : 0)
        + " active_hitflag=" + std::to_string(activeHitFlagAllows ? 1 : 0)
        + " active_hittable=" + std::to_string(activeDefenderHittable ? 1 : 0)
        + " active_p2bodydist=" + std::to_string(activeP2BodyDistX)
        + " closest_attempt_distance=" + std::to_string(closestAttemptDistance)
        + " p1_movetype=" + std::string(1, attemptP1MoveType)
        + " p2_statetype=" + std::string(1, attemptP2StateType)
        + " last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawP1ThrowState ? Status::Pass : Status::Fail, "p1_entered_p1stateno",
        "final_p1_state=" + std::to_string(finalP1.stateNo)
        + " final_p1_action=" + std::to_string(finalP1.action));
    record(out, counts, sawP2CustomGrabState ? Status::Pass : Status::Fail, "p2_entered_custom_throw_state",
        "final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_action=" + std::to_string(finalP2.action));
    record(out, counts, sawBoundTarget ? Status::Pass : Status::Fail, "targetbind_kept_victim_near_p1",
        "closest_bound_distance=" + std::to_string(closestBoundDistance));
    record(out, counts, sawTargetLifeAdd ? Status::Pass : Status::Fail, "targetlifeadd_applied_throw_damage",
        "life_before=" + std::to_string(p2LifeBefore)
        + " life_after=" + std::to_string(finalP2.life));
    record(out, counts, sawTargetReleaseState ? Status::Pass : Status::Fail, "targetstate_released_victim",
        "saw_926=" + std::to_string(sawTargetReleaseState ? 1 : 0));
    record(out, counts, sawP2SelfStateFall ? Status::Pass : Status::Fail, "selfstate_returned_to_common_fall",
        "final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_action=" + std::to_string(finalP2.action));
    record(out, counts, p1Recovered && p2Recovered ? Status::Pass : Status::Fail, "fighters_recovered_after_throw",
        "p1_recovered=" + std::to_string(p1Recovered ? 1 : 0)
        + " p2_recovered=" + std::to_string(p2Recovered ? 1 : 0)
        + " final_p1_state=" + std::to_string(finalP1.stateNo)
        + " final_p2_state=" + std::to_string(finalP2.stateNo));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenKuuchuuShakunetsu(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-kuuchuu-shakunetsu");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const bool selected = runtime.selectTrainingMove("Kuuchuu Shakunetsu Hadou Ken");
    record(out, counts, selected ? Status::Pass : Status::Blocked, "select_kuuchuu_shakunetsu_demo_move",
        "move=Kuuchuu Shakunetsu Hadou Ken");
    if (!selected) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.startTrainingCommandDemo();
    const auto demoStart = runtime.snapshot();
    record(out, counts, (!demoStart.p2.onGround || demoStart.p2.stateType == 'A') ? Status::Pass : Status::Fail,
        "demo_sets_air_prereq",
        "p2_state=" + std::to_string(demoStart.p2.stateNo)
        + " p2_action=" + std::to_string(demoStart.p2.action)
        + " p2_y=" + std::to_string(demoStart.p2.y)
        + " p2_ground=" + std::to_string(demoStart.p2.onGround ? 1 : 0)
        + " p2_type=" + std::string(1, demoStart.p2.stateType));

    bool sawDemoCommand = demoStart.p2Commands.find("QCB_QCB_k") != std::string::npos;
    bool demoEntered4020 = demoStart.p2.stateNo == 4020;
    bool demoRecovered = false;
    FighterSnapshot demoP2 = demoStart.p2;
    FighterSnapshot demoEntryP2 = demoStart.p2;
    FighterSnapshot demoRecoveryP2 = demoStart.p2;
    std::string demoCommands = demoStart.p2Commands;
    for (int frame = 0; frame < 260; ++frame) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        demoP2 = snap.p2;
        if (snap.p2Commands.find("QCB_QCB_k") != std::string::npos) {
            demoCommands = snap.p2Commands;
            sawDemoCommand = true;
        }
        if (!demoEntered4020 && snap.p2.stateNo == 4020) {
            demoEntered4020 = true;
            demoEntryP2 = snap.p2;
        }
        if (demoEntered4020 && !demoRecovered && snap.p2.stateNo == 0 && snap.p2.onGround && snap.p2.moveType == 'I' && snap.p2.ctrl) {
            demoRecovered = true;
            demoRecoveryP2 = snap.p2;
        }
        if (demoEntered4020 && demoRecovered) {
            break;
        }
    }

    record(out, counts, sawDemoCommand ? Status::Pass : Status::Fail, "demo_buffers_air_super_command",
        "commands=" + demoCommands
        + " p2_state=" + std::to_string(demoP2.stateNo)
        + " p2_action=" + std::to_string(demoP2.action));
    record(out, counts, demoEntered4020 ? Status::Pass : Status::Fail, "demo_enters_kuuchuu_shakunetsu",
        "p2_state=" + std::to_string(demoEntryP2.stateNo)
        + " p2_action=" + std::to_string(demoEntryP2.action)
        + " p2_time=" + std::to_string(demoEntryP2.stateTime));
    record(out, counts, demoRecovered ? Status::Pass : Status::Fail, "demo_recovers_after_air_super",
        "p2_state=" + std::to_string(demoRecoveryP2.stateNo)
        + " p2_action=" + std::to_string(demoRecoveryP2.action)
        + " p2_time=" + std::to_string(demoRecoveryP2.stateTime)
        + " p2_y=" + std::to_string(demoRecoveryP2.y)
        + " p2_ground=" + std::to_string(demoRecoveryP2.onGround ? 1 : 0)
        + " p2_ctrl=" + std::to_string(demoRecoveryP2.ctrl ? 1 : 0));

    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.positionFighters(-220.0f, 220.0f);
    runtime.setFighterPower(0, 1000);
    runtime.setFighterPosition(0, -80.0f, -96.0f);
    runtime.setFighterControl(0, true);
    runtime.forceFighterState(0, 4020);
    runtime.setFighterPosition(0, -80.0f, -96.0f);
    runtime.setFighterControl(0, false);

    bool manualEntered4020 = false;
    bool manualReachedLandingState = false;
    bool manualRecovered = false;
    float maxManualY = -100000.0f;
    FighterSnapshot manualP1 = runtime.snapshot().p1;
    FighterSnapshot manualEntryP1 = manualP1;
    FighterSnapshot manualLandingP1 = manualP1;
    FighterSnapshot manualRecoveryP1 = manualP1;
    for (int frame = 0; frame < 360; ++frame) {
        const auto snap = runtime.snapshot();
        manualP1 = snap.p1;
        maxManualY = std::max(maxManualY, snap.p1.y);
        if (!manualEntered4020 && snap.p1.stateNo == 4020) {
            manualEntered4020 = true;
            manualEntryP1 = snap.p1;
        }
        if (!manualReachedLandingState && (snap.p1.stateNo == 4044 || snap.p1.stateNo == 52)) {
            manualReachedLandingState = true;
            manualLandingP1 = snap.p1;
        }
        if (manualEntered4020 && !manualRecovered && snap.p1.stateNo == 0 && snap.p1.onGround && snap.p1.moveType == 'I' && snap.p1.ctrl) {
            manualRecovered = true;
            manualRecoveryP1 = snap.p1;
        }
        if (manualEntered4020 && manualRecovered) {
            break;
        }
        runtime.step({}, 1);
    }

    record(out, counts, manualEntered4020 ? Status::Pass : Status::Fail, "manual_enters_startup_state",
        "p1_state=" + std::to_string(manualEntryP1.stateNo)
        + " p1_action=" + std::to_string(manualEntryP1.action));
    record(out, counts, manualReachedLandingState ? Status::Pass : Status::Fail, "manual_reaches_landing_state",
        "p1_state=" + std::to_string(manualLandingP1.stateNo)
        + " p1_action=" + std::to_string(manualLandingP1.action)
        + " max_y=" + std::to_string(maxManualY));
    record(out, counts, manualRecovered ? Status::Pass : Status::Fail, "manual_recovers_from_whiffed_kick",
        "p1_state=" + std::to_string(manualRecoveryP1.stateNo)
        + " p1_action=" + std::to_string(manualRecoveryP1.action)
        + " p1_time=" + std::to_string(manualRecoveryP1.stateTime)
        + " p1_y=" + std::to_string(manualRecoveryP1.y)
        + " p1_ground=" + std::to_string(manualRecoveryP1.onGround ? 1 : 0)
        + " p1_ctrl=" + std::to_string(manualRecoveryP1.ctrl ? 1 : 0));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

bool recoveredGroundIdle(const FighterSnapshot& fighter) {
    return fighter.stateNo == 0
        && fighter.onGround
        && fighter.moveType == 'I'
        && fighter.hitPauseTicks <= 0;
}

std::string fighterBriefText(const FighterSnapshot& fighter) {
    std::ostringstream text;
    text << "state=" << fighter.stateNo
         << " action=" << fighter.action
         << " time=" << fighter.stateTime
         << " y=" << fighter.y
         << " vy=" << fighter.vy
         << " type=" << fighter.stateType
         << " move=" << fighter.moveType
         << " physics=" << fighter.physics
         << " ground=" << (fighter.onGround ? 1 : 0)
         << " ctrl=" << (fighter.ctrl ? 1 : 0)
         << " life=" << fighter.life
         << " target=" << fighter.targetIndex
         << "/" << fighter.targetTicks
         << "/" << fighter.targetHitId
         << " hitpause=" << fighter.hitPauseTicks
         << " hitstun=" << fighter.hitStunTicks;
    return text.str();
}

int runEvilKenShinryukenRecovery(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-shinryuken-recovery");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    bool allCyclesHit = true;
    bool allCyclesReachedFollowup = true;
    bool allCyclesP2Fall = true;
    bool allCyclesRecovered = true;
    std::string lastHitText;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    std::vector<std::string> finalP2Trace;

    for (int cycle = 0; cycle < 2; ++cycle) {
        runtime.forceFighterState(0, 0);
        runtime.forceFighterState(1, 0);
        runtime.setFighterPosition(0, -20.0f, 0.0f);
        runtime.setFighterPosition(1, 8.0f, 0.0f);
        runtime.setFighterControl(0, false);
        runtime.setFighterControl(1, false);
        runtime.setFighterPower(0, 3000);
        runtime.setFighterLife(1, 1000);
        runtime.setFighterVar(0, 28, cycle == 0 ? 0 : 3);
        runtime.forceFighterState(0, 3700);
        runtime.setFighterPosition(0, -20.0f, 0.0f);
        runtime.setFighterPosition(1, 8.0f, 0.0f);

        bool sawHit = false;
        bool sawFollowup = false;
        bool sawP2Fall = false;
        bool recovered = false;
        FighterSnapshot cycleP1 = runtime.snapshot().p1;
        FighterSnapshot cycleP2 = runtime.snapshot().p2;
        std::vector<std::string> p2Trace;
        int lastP2State = -100000;
        int lastP2Action = -100000;
        for (int frame = 0; frame < 1000; ++frame) {
            const auto snap = runtime.snapshot();
            cycleP1 = snap.p1;
            cycleP2 = snap.p2;
            finalP1 = cycleP1;
            finalP2 = cycleP2;
            if ((snap.p2.stateNo != lastP2State || snap.p2.action != lastP2Action) && p2Trace.size() < 28) {
                std::ostringstream trace;
                trace << "f" << frame
                      << " p2=" << snap.p2.stateNo << "/" << snap.p2.action
                      << " t=" << snap.p2.stateTime
                      << " y=" << snap.p2.y
                      << " vy=" << snap.p2.vy
                      << " hitstun=" << snap.p2.hitStunTicks;
                p2Trace.push_back(trace.str());
                lastP2State = snap.p2.stateNo;
                lastP2Action = snap.p2.action;
            }
            if (!snap.lastHitText.empty()) {
                lastHitText = snap.lastHitText;
            }
            sawHit = sawHit
                || snap.lastHitText.find("P1 hit 3700#") != std::string::npos
                || snap.lastHitText.find("P1 hit 3710#") != std::string::npos
                || snap.lastHitText.find("P1 hit 3711#") != std::string::npos
                || snap.lastHitText.find("P1 hit 3712#") != std::string::npos;
            sawFollowup = sawFollowup
                || snap.p1.stateNo == 3710
                || snap.p1.stateNo == 3711
                || snap.p1.stateNo == 3712
                || snap.p1.stateNo == 3713;
            sawP2Fall = sawP2Fall
                || snap.p2.stateNo == 5030
                || snap.p2.stateNo == 5050
                || snap.p2.stateNo == 5100
                || snap.p2.stateNo == 5101
                || snap.p2.stateNo == 5110
                || snap.p2.stateNo == 5120
                || snap.p2.stateNo == 5160
                || snap.p2.stateNo == 5170;
            recovered = recovered || (recoveredGroundIdle(snap.p1) && recoveredGroundIdle(snap.p2));
            if (recovered && sawHit && sawFollowup && sawP2Fall) {
                break;
            }
            runtime.step({}, 1);
        }

        allCyclesHit = allCyclesHit && sawHit;
        allCyclesReachedFollowup = allCyclesReachedFollowup && sawFollowup;
        allCyclesP2Fall = allCyclesP2Fall && sawP2Fall;
        allCyclesRecovered = allCyclesRecovered && recovered;
        finalP2Trace = p2Trace;
    }

    record(out, counts, allCyclesHit ? Status::Pass : Status::Fail, "shinryuken_hits_connect",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, allCyclesReachedFollowup ? Status::Pass : Status::Fail, "shinryuken_reaches_followup_states",
        "final_p1=" + fighterBriefText(finalP1));
    record(out, counts, allCyclesP2Fall ? Status::Pass : Status::Fail, "shinryuken_victim_enters_common_fall",
        "final_p2=" + fighterBriefText(finalP2));
    std::string recoveryDetails = "final_p1=" + fighterBriefText(finalP1)
        + " final_p2=" + fighterBriefText(finalP2);
    if (!allCyclesRecovered) {
        recoveryDetails += " p2_trace=" + joinLimited(finalP2Trace, 28);
    }
    record(out, counts, allCyclesRecovered ? Status::Pass : Status::Fail, "shinryuken_victim_recovers_after_last_hit",
        recoveryDetails);
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

struct ShunGokuProbeResult {
    bool sawDashHit = false;
    bool sawCapture = false;
    bool sawNormalEnder = false;
    bool sawLowLifeFinisher = false;
    bool sawVictimInitialCustom = false;
    bool sawVictimNormalRelease = false;
    bool sawVictimFinisherIntro = false;
    bool sawVictimFinisherLift = false;
    bool sawVictimFinisherDown = false;
    bool sawLifeAdd = false;
    bool p1Recovered = false;
    bool p2RecoveredOrKo = false;
    int maxCaptureStateTime = -1;
    FighterSnapshot captureP1;
    FighterSnapshot captureP2;
    std::string lastHitText;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
};

ShunGokuProbeResult runShunGokuProbe(RuntimeProbe& runtime, int p2Life, bool lowLifeFinisher) {
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -18.0f, 0.0f);
    runtime.setFighterPosition(1, 4.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.setFighterPower(0, 3000);
    runtime.setFighterLife(1, p2Life);
    runtime.setFighterVar(0, 28, 0);
    runtime.forceFighterState(0, 3890);
    runtime.setFighterPosition(0, -18.0f, 0.0f);
    runtime.setFighterPosition(1, 4.0f, 0.0f);

    ShunGokuProbeResult result;
    for (int frame = 0; frame < 820; ++frame) {
        const auto snap = runtime.snapshot();
        result.finalP1 = snap.p1;
        result.finalP2 = snap.p2;
        if (!snap.lastHitText.empty()) {
            result.lastHitText = snap.lastHitText;
        }
        result.sawDashHit = result.sawDashHit
            || snap.lastHitText.find("P1 hit 3890#") != std::string::npos
            || snap.p1.stateNo == 3891;
        if (snap.p1.stateNo == 3891) {
            result.sawCapture = true;
            if (snap.p1.stateTime > result.maxCaptureStateTime) {
                result.maxCaptureStateTime = snap.p1.stateTime;
                result.captureP1 = snap.p1;
                result.captureP2 = snap.p2;
            }
        }
        result.sawNormalEnder = result.sawNormalEnder || snap.p1.stateNo == 3892;
        result.sawLowLifeFinisher = result.sawLowLifeFinisher || snap.p1.stateNo == 3893;
        result.sawVictimInitialCustom = result.sawVictimInitialCustom || snap.p2.stateNo == 3895;
        result.sawVictimNormalRelease = result.sawVictimNormalRelease || snap.p2.stateNo == 3896;
        result.sawVictimFinisherIntro = result.sawVictimFinisherIntro || snap.p2.stateNo == 3897;
        result.sawVictimFinisherLift = result.sawVictimFinisherLift || snap.p2.stateNo == 3898;
        result.sawVictimFinisherDown = result.sawVictimFinisherDown || snap.p2.stateNo == 3899;
        const bool expectedPathComplete = lowLifeFinisher
            ? (result.sawLowLifeFinisher
                && result.sawVictimFinisherIntro
                && result.sawVictimFinisherLift
                && result.sawVictimFinisherDown)
            : (result.sawNormalEnder && result.sawVictimNormalRelease);
        result.sawLifeAdd = result.sawLifeAdd || snap.p2.life < p2Life;
        if (expectedPathComplete) {
            result.p1Recovered = result.p1Recovered || recoveredGroundIdle(snap.p1);
            result.p2RecoveredOrKo = result.p2RecoveredOrKo
                || recoveredGroundIdle(snap.p2)
                || snap.p2.stateNo == 5150
                || snap.roundWinner != 0;
        }
        if (result.sawDashHit
            && result.sawCapture
            && result.sawVictimInitialCustom
            && expectedPathComplete
            && result.p1Recovered
            && result.p2RecoveredOrKo) {
            break;
        }
        if (lowLifeFinisher && !result.sawLowLifeFinisher) {
            runtime.setFighterLife(1, p2Life);
        }
        runtime.step({}, 1);
    }
    return result;
}

int runEvilKenShunGokuSatsu(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-shun-goku-satsu");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto normal = runShunGokuProbe(runtime, 1000, false);
    record(out, counts, normal.sawDashHit ? Status::Pass : Status::Fail, "shun_goku_dash_hit_connects",
        "last_hit=\"" + normal.lastHitText + "\" final_p1=" + fighterBriefText(normal.finalP1));
    record(out, counts, normal.sawCapture && normal.sawVictimInitialCustom ? Status::Pass : Status::Fail,
        "shun_goku_captures_target",
        "saw_capture=" + std::to_string(normal.sawCapture ? 1 : 0)
        + " saw_3895=" + std::to_string(normal.sawVictimInitialCustom ? 1 : 0)
        + " max_capture_time=" + std::to_string(normal.maxCaptureStateTime)
        + " capture_p1=" + fighterBriefText(normal.captureP1)
        + " capture_p2=" + fighterBriefText(normal.captureP2)
        + " final_p2=" + fighterBriefText(normal.finalP2));
    record(out, counts, normal.sawNormalEnder && normal.sawVictimNormalRelease ? Status::Pass : Status::Fail,
        "shun_goku_normal_release_path_finishes",
        "lifeadd=" + std::to_string(normal.sawLifeAdd ? 1 : 0)
        + " p1_3892=" + std::to_string(normal.sawNormalEnder ? 1 : 0)
        + " p2_3896=" + std::to_string(normal.sawVictimNormalRelease ? 1 : 0)
        + " final_p2=" + fighterBriefText(normal.finalP2));
    record(out, counts, normal.p1Recovered && normal.p2RecoveredOrKo ? Status::Pass : Status::Fail,
        "shun_goku_normal_path_recovers",
        "p1_recovered=" + std::to_string(normal.p1Recovered ? 1 : 0)
        + " p2_recovered=" + std::to_string(normal.p2RecoveredOrKo ? 1 : 0)
        + " final_p1=" + fighterBriefText(normal.finalP1)
        + " final_p2=" + fighterBriefText(normal.finalP2));

    const auto finisher = runShunGokuProbe(runtime, 520, true);
    record(out, counts, finisher.sawLowLifeFinisher ? Status::Pass : Status::Fail,
        "shun_goku_low_life_routes_to_finisher",
        "lifeadd=" + std::to_string(finisher.sawLifeAdd ? 1 : 0)
        + " p1_3893=" + std::to_string(finisher.sawLowLifeFinisher ? 1 : 0)
        + " final_p1=" + fighterBriefText(finisher.finalP1)
        + " final_p2=" + fighterBriefText(finisher.finalP2));
    record(out, counts,
        finisher.sawVictimFinisherIntro && finisher.sawVictimFinisherLift && finisher.sawVictimFinisherDown
            ? Status::Pass
            : Status::Fail,
        "shun_goku_finisher_victim_sequence_completes",
        "p2_3897=" + std::to_string(finisher.sawVictimFinisherIntro ? 1 : 0)
        + " p2_3898=" + std::to_string(finisher.sawVictimFinisherLift ? 1 : 0)
        + " p2_3899=" + std::to_string(finisher.sawVictimFinisherDown ? 1 : 0)
        + " final_p2=" + fighterBriefText(finisher.finalP2));
    record(out, counts, finisher.p1Recovered && finisher.p2RecoveredOrKo ? Status::Pass : Status::Fail,
        "shun_goku_finisher_path_resolves",
        "p1_recovered=" + std::to_string(finisher.p1Recovered ? 1 : 0)
        + " p2_resolved=" + std::to_string(finisher.p2RecoveredOrKo ? 1 : 0)
        + " final_p1=" + fighterBriefText(finisher.finalP1)
        + " final_p2=" + fighterBriefText(finisher.finalP2));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenTrainingDemoAll(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-training-demo-all");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const std::vector<TrainingMoveInfo> moves = runtime.trainingMoves();
    record(out, counts, !moves.empty() ? Status::Pass : Status::Blocked, "training_move_list_loaded",
        "moves=" + std::to_string(moves.size()));
    if (moves.empty()) {
        summary(out, counts);
        return exitCode(counts);
    }

    int enteredTargets = 0;
    int recoveredTargets = 0;
    std::vector<std::string> missingTargets;
    std::vector<std::string> stuckRecoveries;
    for (int index = 0; index < static_cast<int>(moves.size()); ++index) {
        const TrainingMoveInfo& move = moves[static_cast<size_t>(index)];
        if (!runtime.selectTrainingMoveIndex(index)) {
            missingTargets.push_back(move.label + " [selection failed]");
            continue;
        }

        runtime.startTrainingCommandDemo();
        const int var34AfterStart = runtime.fighterVar(1, 34);
        bool sawTarget = false;
        bool recovered = false;
        FighterSnapshot targetP2 = runtime.snapshot().p2;
        FighterSnapshot finalP2 = targetP2;
        std::string commands = runtime.snapshot().p2Commands;
        bool sawExpectedCommand = commandListMatchesExpected(move, commands);
        std::vector<std::string> commandTrace;
        std::vector<std::string> stateTrace;
        appendUniqueText(commandTrace, commands);
        appendUniqueText(stateTrace, stateTraceText(targetP2));
        for (int frame = 0; frame < 520; ++frame) {
            const auto snap = runtime.snapshot();
            finalP2 = snap.p2;
            if (!snap.p2Commands.empty()) {
                commands = snap.p2Commands;
                sawExpectedCommand = sawExpectedCommand || commandListMatchesExpected(move, commands);
                appendUniqueText(commandTrace, commands);
            }
            appendUniqueText(stateTrace, stateTraceText(snap.p2));
            if (!sawTarget && snapshotMatchesTrainingMoveTarget(move, snap.p2)) {
                sawTarget = true;
                targetP2 = snap.p2;
            }
            if (sawTarget
                && snap.p2.onGround
                && snap.p2.moveType == 'I'
                && snap.p2.ctrl
                && (snap.p2.stateNo == 0 || snap.p2.stateNo == 20)) {
                recovered = true;
                finalP2 = snap.p2;
                break;
            }
            runtime.step({}, 1);
        }

        if (sawTarget) {
            ++enteredTargets;
        } else {
            missingTargets.push_back(moveFailureText(move, finalP2, commands)
                + " expected_cmd=" + joinLimited(move.commandNames)
                + " saw_expected=" + std::to_string(sawExpectedCommand ? 1 : 0)
                + " seen_cmds=" + joinLimited(commandTrace)
                + " states=" + joinLimited(stateTrace)
                + " var34=" + std::to_string(var34AfterStart));
            continue;
        }
        if (recovered) {
            ++recoveredTargets;
        } else {
            stuckRecoveries.push_back(moveFailureText(move, finalP2, commands)
                + " target_action=" + std::to_string(targetP2.action));
        }
    }

    record(out, counts, missingTargets.empty() ? Status::Pass : Status::Fail, "all_training_demos_enter_target_state",
        "entered=" + std::to_string(enteredTargets)
        + "/" + std::to_string(moves.size())
        + " failures=" + joinLimited(missingTargets));
    record(out, counts, stuckRecoveries.empty() ? Status::Pass : Status::Partial, "all_training_demos_recover",
        "recovered=" + std::to_string(recoveredTargets)
        + "/" + std::to_string(moves.size())
        + " stuck=" + joinLimited(stuckRecoveries));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
