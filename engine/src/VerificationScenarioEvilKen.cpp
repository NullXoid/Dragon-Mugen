#include "VerificationScenarioEvilKenCommon.h"

namespace dragon::verification {

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
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
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

int runEvilKenTripJumpBuffer(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-trip-jump-buffer");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.forceFighterState(0, 1834);
    runtime.setFighterPosition(0, -40.0f, -64.0f);
    runtime.setFighterPosition(1, 180.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    SymbolicInput up;
    up.up = true;
    bool heldUpDuringRecovery = false;
    bool sawTripFollowup = false;
    bool sawLandingOrIdle = false;
    bool sawBufferedJump = false;
    int heldStartFrame = -1;
    int jumpFrame = -1;
    FighterSnapshot heldFrame;
    FighterSnapshot landingFrame;
    FighterSnapshot jumpSnapshot;
    FighterSnapshot finalP1;

    for (int frame = 0; frame < 260; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        sawTripFollowup = sawTripFollowup || snap.p1.stateNo == 1834 || snap.p1.stateNo == 1839;
        if (snap.p1.stateNo == 52 || (snap.p1.stateNo == 0 && snap.p1.onGround)) {
            sawLandingOrIdle = true;
            landingFrame = snap.p1;
        }
        if (frame >= 6 && (!snap.p1.ctrl || !snap.p1.onGround || snap.p1.stateNo != 0)) {
            heldUpDuringRecovery = true;
            if (heldStartFrame < 0) {
                heldStartFrame = frame;
                heldFrame = snap.p1;
            }
        }
        if (heldUpDuringRecovery && snap.p1.stateNo == 50 && !snap.p1.onGround && snap.p1.vy < -1.0f) {
            sawBufferedJump = true;
            jumpFrame = frame;
            jumpSnapshot = snap.p1;
            break;
        }
        runtime.step(frame >= 6 ? up : SymbolicInput{}, 1);
    }

    record(out, counts, sawTripFollowup ? Status::Pass : Status::Fail, "trip_followup_recovery_exercised",
        "held_start_frame=" + std::to_string(heldStartFrame)
        + " held_state=" + std::to_string(heldFrame.stateNo)
        + " held_y=" + std::to_string(heldFrame.y)
        + " held_ctrl=" + std::to_string(heldFrame.ctrl ? 1 : 0));
    record(out, counts, heldUpDuringRecovery ? Status::Pass : Status::Fail, "up_held_before_actionable",
        "held_start_frame=" + std::to_string(heldStartFrame)
        + " held_state=" + std::to_string(heldFrame.stateNo)
        + " held_ground=" + std::to_string(heldFrame.onGround ? 1 : 0)
        + " held_ctrl=" + std::to_string(heldFrame.ctrl ? 1 : 0));
    record(out, counts, sawLandingOrIdle ? Status::Pass : Status::Fail, "trip_recovery_reaches_ground_control_path",
        "landing_state=" + std::to_string(landingFrame.stateNo)
        + " landing_action=" + std::to_string(landingFrame.action)
        + " landing_ctrl=" + std::to_string(landingFrame.ctrl ? 1 : 0)
        + " final_state=" + std::to_string(finalP1.stateNo));
    record(out, counts, sawBufferedJump ? Status::Pass : Status::Fail, "early_up_starts_jump_after_trip",
        "jump_frame=" + std::to_string(jumpFrame)
        + " jump_state=" + std::to_string(jumpSnapshot.stateNo)
        + " jump_action=" + std::to_string(jumpSnapshot.action)
        + " jump_y=" + std::to_string(jumpSnapshot.y)
        + " jump_vy=" + std::to_string(jumpSnapshot.vy)
        + " final_state=" + std::to_string(finalP1.stateNo)
        + " final_ground=" + std::to_string(finalP1.onGround ? 1 : 0));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenAttackJumpBufferRelease(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-attack-jump-buffer-release");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setFighterPosition(0, -80.0f, 0.0f);
    runtime.setFighterPosition(1, 160.0f, 0.0f);
    runtime.setFighterControl(1, false);
    runtime.step({}, 4);

    SymbolicInput jab;
    jab.x = true;
    runtime.step(jab, 2);

    SymbolicInput heldCommandJump;
    heldCommandJump.up = true;
    heldCommandJump.right = true;
    heldCommandJump.x = true;

    bool jumpedWhileCommandHeld = false;
    FighterSnapshot commandHeldFinal;
    for (int frame = 0; frame < 90; ++frame) {
        runtime.step(heldCommandJump, 1);
        const auto snap = runtime.snapshot();
        commandHeldFinal = snap.p1;
        if (snap.p1.stateNo == 50 || (!snap.p1.onGround && snap.p1.vy < -1.0f)) {
            jumpedWhileCommandHeld = true;
            break;
        }
    }

    SymbolicInput releasedToJump;
    releasedToJump.up = true;
    releasedToJump.right = true;

    bool jumpedAfterRelease = false;
    int jumpFrame = -1;
    FighterSnapshot jumpSnapshot;
    for (int frame = 0; frame < 180; ++frame) {
        runtime.step(releasedToJump, 1);
        const auto snap = runtime.snapshot();
        if (snap.p1.stateNo == 50 && !snap.p1.onGround && snap.p1.vy < -1.0f) {
            jumpedAfterRelease = true;
            jumpFrame = frame;
            jumpSnapshot = snap.p1;
            break;
        }
    }

    record(out, counts, !jumpedWhileCommandHeld ? Status::Pass : Status::Fail, "held_command_suppresses_jump_consume",
        "state=" + std::to_string(commandHeldFinal.stateNo)
        + " action=" + std::to_string(commandHeldFinal.action)
        + " ground=" + std::to_string(commandHeldFinal.onGround ? 1 : 0)
        + " vx=" + std::to_string(commandHeldFinal.vx)
        + " vy=" + std::to_string(commandHeldFinal.vy));
    record(out, counts, jumpedAfterRelease ? Status::Pass : Status::Fail, "release_command_while_holding_up_jumps",
        "jump_frame=" + std::to_string(jumpFrame)
        + " state=" + std::to_string(jumpSnapshot.stateNo)
        + " action=" + std::to_string(jumpSnapshot.action)
        + " vx=" + std::to_string(jumpSnapshot.vx)
        + " vy=" + std::to_string(jumpSnapshot.vy));
    record(out, counts, jumpedAfterRelease && jumpSnapshot.vx > 0.1f ? Status::Pass : Status::Fail,
        "held_up_right_keeps_forward_jump_direction",
        "jump_action=" + std::to_string(jumpSnapshot.action)
        + " jump_vx=" + std::to_string(jumpSnapshot.vx));
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


} // namespace dragon::verification
