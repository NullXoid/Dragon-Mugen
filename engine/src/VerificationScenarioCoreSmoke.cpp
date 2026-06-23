#include "VerificationScenarioCommon.h"

namespace dragon::verification {

int runKfmBaseline(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "kfm-baseline");

    const bool settled = waitForControllableIdle(runtime, 360);
    const auto settleSnap = runtime.snapshot().p1;
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(settleSnap.stateNo) + " anim=" + std::to_string(settleSnap.action)
        + " ctrl=" + std::to_string(settleSnap.ctrl ? 1 : 0));
    if (!settled) {
        record(out, counts, Status::Blocked, "downstream_combat_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto idleBefore = runtime.snapshot();
    runtime.step({}, 60);
    const auto idleAfter = runtime.snapshot();
    record(out, counts, idleAfter.p1.life > 0 ? Status::Pass : Status::Fail, "idle_stability",
        "p1_state_before=" + std::to_string(idleBefore.p1.stateNo)
        + " p1_state_after=" + std::to_string(idleAfter.p1.stateNo)
        + " p1_anim_before=" + std::to_string(idleBefore.p1.action)
        + " p1_anim_after=" + std::to_string(idleAfter.p1.action));

    const float xRightBefore = runtime.snapshot().p1.x;
    runtime.step(SymbolicInput{ .right = true }, 60);
    const float xRightAfter = runtime.snapshot().p1.x;
    const bool rightMoved = xRightAfter > xRightBefore + 1.0f;
    record(out, counts, rightMoved ? Status::Pass : Status::Fail, "hold_right_movement",
        "x_before=" + std::to_string(xRightBefore) + " x_after=" + std::to_string(xRightAfter)
        + " delta=" + std::to_string(xRightAfter - xRightBefore));
    if (!rightMoved) {
        record(out, counts, Status::Blocked, "downstream_combat_checks", "movement gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const float xLeftBefore = runtime.snapshot().p1.x;
    runtime.step(SymbolicInput{ .left = true }, 60);
    const float xLeftAfter = runtime.snapshot().p1.x;
    record(out, counts, xLeftAfter < xLeftBefore - 1.0f ? Status::Pass : Status::Fail, "hold_left_movement",
        "x_before=" + std::to_string(xLeftBefore) + " x_after=" + std::to_string(xLeftAfter)
        + " delta=" + std::to_string(xLeftAfter - xLeftBefore));

    runtime.step(SymbolicInput{ .down = true }, 30);
    const auto crouch = runtime.snapshot().p1;
    record(out, counts, (crouch.stateType == 'C' || crouch.stateNo == 10 || crouch.stateNo == 11) ? Status::Pass : Status::Fail,
        "crouch", "state=" + std::to_string(crouch.stateNo) + " anim=" + std::to_string(crouch.action));

    runtime.step({}, 30);
    float yMin = runtime.snapshot().p1.y;
    bool sawAir = false;
    runtime.step(SymbolicInput{ .up = true }, 4);
    for (int i = 0; i < 120; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        yMin = std::min(yMin, p1.y);
        sawAir = sawAir || !p1.onGround || p1.stateType == 'A';
    }
    const auto jumpAfter = runtime.snapshot().p1;
    record(out, counts, sawAir && jumpAfter.onGround ? Status::Pass : Status::Fail, "jump_and_land",
        "y_min=" + std::to_string(yMin) + " grounded_final=" + (jumpAfter.onGround ? std::string("true") : std::string("false")));

    char standCommand = '?';
    FighterSnapshot normalBefore;
    FighterSnapshot normalAfter;
    const bool standNormal = tryNormal(runtime, standCommand, normalBefore, normalAfter, false);
    record(out, counts, standNormal ? Status::Pass : Status::Fail, "standing_normal_state_change",
        standNormal ? stateActionDetail(normalBefore, normalAfter, standCommand) : "no x/y/z/a/b/c state or animation change");

    char crouchCommand = '?';
    FighterSnapshot crouchBefore;
    FighterSnapshot crouchAfter;
    const bool crouchNormal = tryNormal(runtime, crouchCommand, crouchBefore, crouchAfter, true);
    record(out, counts, crouchNormal ? Status::Pass : Status::Fail, "crouching_normal_state_change",
        crouchNormal ? stateActionDetail(crouchBefore, crouchAfter, crouchCommand) : "no down+x/y/z/a/b/c state or animation change");

    runtime.positionFighters(-80.0f, 80.0f);
    const auto tauntCtrlSet = observeTauntCtrlSetControlRestore(runtime, standCommand == '?' ? 'y' : standCommand);
    record(out, counts, tauntCtrlSetControlRestorePassed(tauntCtrlSet) ? Status::Pass : Status::Fail,
        "taunt_ctrlset_control_restore", tauntCtrlSetControlRestoreDetail(tauntCtrlSet));

    runtime.positionFighters(-18.0f, 24.0f);
    waitForControllableIdle(runtime, 360);
    runtime.step({}, 5);
    const auto hitBefore = runtime.snapshot();
    SymbolicInput hitInput = withButton(standCommand == '?' ? 'x' : standCommand);
    runtime.step(hitInput, 2);
    bool sawContact = false;
    bool sawHitPause = false;
    bool sawActiveEffect = false;
    bool sawActiveSound = false;
    int peakActiveEffects = 0;
    int peakActiveSounds = 0;
    for (int i = 0; i < 50; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        sawContact = sawContact || snap.p1.moveContact || snap.p1.moveHit || snap.p1.hitCount > hitBefore.p1.hitCount;
        sawHitPause = sawHitPause || snap.p1.hitPauseTicks > 0 || snap.p2.hitPauseTicks > 0;
        sawActiveEffect = sawActiveEffect || snap.activeEffects > 0;
        sawActiveSound = sawActiveSound || snap.activeSounds > 0;
        peakActiveEffects = std::max(peakActiveEffects, snap.activeEffects);
        peakActiveSounds = std::max(peakActiveSounds, snap.activeSounds);
    }
    const auto hitAfter = runtime.snapshot();
    record(out, counts, sawContact ? Status::Pass : Status::Fail, "hit_contact",
        "contact=" + std::to_string(sawContact ? 1 : 0) + " hit_count_before=" + std::to_string(hitBefore.p1.hitCount)
        + " hit_count_after=" + std::to_string(hitAfter.p1.hitCount) + " last_hit=\"" + hitAfter.lastHitText + "\"");
    record(out, counts, hitAfter.p2.life < hitBefore.p2.life ? Status::Pass : Status::Fail, "damage",
        "p2_life_before=" + std::to_string(hitBefore.p2.life) + " p2_life_after=" + std::to_string(hitAfter.p2.life)
        + " delta=" + std::to_string(hitAfter.p2.life - hitBefore.p2.life));
    record(out, counts, (sawHitPause && sawActiveEffect && sawActiveSound) ? Status::Pass : Status::Fail,
        "hitpause_spark_sound", "hitpause_observed=" + std::to_string(sawHitPause ? 1 : 0)
        + " active_effect_observed=" + std::to_string(sawActiveEffect ? 1 : 0)
        + " active_sound_observed=" + std::to_string(sawActiveSound ? 1 : 0)
        + " peak_active_effects=" + std::to_string(peakActiveEffects)
        + " peak_active_sounds=" + std::to_string(peakActiveSounds));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runKfmThrow(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "kfm-throw");

    const bool settled = waitForControllableIdle(runtime, 360);
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!settled) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-10.0f, 10.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -10.0f, 0.0f);
    runtime.setFighterPosition(1, 10.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.step({}, 2);
    const int p2LifeBefore = runtime.snapshot().p2.life;
    runtime.forceFighterState(0, 800);
    runtime.setFighterPosition(0, -10.0f, 0.0f);
    runtime.setFighterPosition(1, 10.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool sawThrowHit = false;
    bool sawP1ThrowState = false;
    bool sawP2CustomGrabState = false;
    bool sawBoundTarget = false;
    bool sawTargetLifeAdd = false;
    bool sawTargetReleaseState = false;
    bool sawP2SelfStateFall = false;
    bool p1Recovered = false;
    bool p2Recovered = false;
    float closestBoundDistance = 100000.0f;
    std::string lastHitText;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    for (int frame = 0; frame < 420; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
        sawThrowHit = sawThrowHit || snap.lastHitText.find("P1 hit 800#") != std::string::npos;
        sawP1ThrowState = sawP1ThrowState || snap.p1.stateNo == 810;
        sawP2CustomGrabState = sawP2CustomGrabState || snap.p2.stateNo == 820;
        sawTargetReleaseState = sawTargetReleaseState || snap.p2.stateNo == 821;
        sawTargetLifeAdd = sawTargetLifeAdd || snap.p2.life < p2LifeBefore;
        if (snap.p1.stateNo == 810 && (snap.p2.stateNo == 820 || snap.p2.stateNo == 821)) {
            const float distance = std::fabs(snap.p2.x - snap.p1.x);
            closestBoundDistance = std::min(closestBoundDistance, distance);
            sawBoundTarget = sawBoundTarget || distance <= 80.0f;
        }
        sawP2SelfStateFall = sawP2SelfStateFall || snap.p2.stateNo == 5100 || snap.p2.stateNo == 5110 || snap.p2.stateNo == 5120;
        p1Recovered = p1Recovered || (snap.p1.stateNo == 0 && snap.p1.onGround && snap.p1.moveType == 'I');
        p2Recovered = p2Recovered || (snap.p2.stateNo == 0 && snap.p2.onGround && snap.p2.moveType == 'I');
        runtime.step({}, 1);
    }

    record(out, counts, sawThrowHit ? Status::Pass : Status::Fail, "throw_hitdef_connected",
        "last_hit=\"" + lastHitText + "\"");
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
        "saw_821=" + std::to_string(sawTargetReleaseState ? 1 : 0));
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

int runKfmAirState(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "kfm-air-state");

    const bool settled = waitForControllableIdle(runtime, 360);
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " anim=" + std::to_string(runtime.snapshot().p1.action)
        + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0));
    if (!settled) {
        record(out, counts, Status::Blocked, "air_state_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.step({}, 20);
    const auto forwardJump = holdInputUntilLanding(runtime, SymbolicInput{ .right = true, .up = true }, 180);
    record(out, counts, airLandingPassed(forwardJump) ? Status::Pass : Status::Fail,
        "diagonal_jump_forward_lands", airLandingDetail(forwardJump));

    runtime.step({}, 60);
    const bool settledAfterForward = waitForControllableIdle(runtime, 240);
    record(out, counts, settledAfterForward ? Status::Pass : Status::Fail, "idle_after_forward_diagonal",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " on_ground=" + std::to_string(runtime.snapshot().p1.onGround ? 1 : 0));

    runtime.step(SymbolicInput{ .right = true }, 20);
    const auto forwardWalkJump = holdInputUntilLanding(runtime, SymbolicInput{ .right = true, .up = true }, 180);
    record(out, counts, airLandingPassed(forwardWalkJump) ? Status::Pass : Status::Fail,
        "diagonal_jump_forward_from_walk_lands", airLandingDetail(forwardWalkJump));

    runtime.step({}, 60);
    const bool settledAfterForwardWalk = waitForControllableIdle(runtime, 240);
    record(out, counts, settledAfterForwardWalk ? Status::Pass : Status::Fail, "idle_after_forward_walk_diagonal",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " on_ground=" + std::to_string(runtime.snapshot().p1.onGround ? 1 : 0));

    runtime.step({}, 20);
    const auto backJump = holdInputUntilLanding(runtime, SymbolicInput{ .left = true, .up = true }, 180);
    record(out, counts, airLandingPassed(backJump) ? Status::Pass : Status::Fail,
        "diagonal_jump_back_lands", airLandingDetail(backJump));

    runtime.step({}, 60);
    const bool settledAfterBack = waitForControllableIdle(runtime, 240);
    record(out, counts, settledAfterBack ? Status::Pass : Status::Fail, "idle_after_back_diagonal",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " on_ground=" + std::to_string(runtime.snapshot().p1.onGround ? 1 : 0));

    runtime.step(SymbolicInput{ .left = true }, 20);
    const auto backWalkJump = holdInputUntilLanding(runtime, SymbolicInput{ .left = true, .up = true }, 180);
    record(out, counts, airLandingPassed(backWalkJump) ? Status::Pass : Status::Fail,
        "diagonal_jump_back_from_walk_lands", airLandingDetail(backWalkJump));

    runtime.step({}, 60);
    const bool settledAfterBackWalk = waitForControllableIdle(runtime, 240);
    record(out, counts, settledAfterBackWalk ? Status::Pass : Status::Fail, "idle_after_back_walk_diagonal",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " on_ground=" + std::to_string(runtime.snapshot().p1.onGround ? 1 : 0));

    runtime.step({}, 20);
    bool sawAirAttack = false;
    bool sawAir = false;
    bool landedAfterAttack = false;
    float yMin = runtime.snapshot().p1.y;
    runtime.step(SymbolicInput{ .up = true }, 4);
    runtime.step({}, 4);
    runtime.step(withButton('x'), 2);
    for (int i = 0; i < 240; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        yMin = std::min(yMin, p1.y);
        sawAir = sawAir || snapshotIsAirborne(p1);
        sawAirAttack = sawAirAttack || (p1.moveType == 'A' && p1.stateNo != 0);
        landedAfterAttack = landedAfterAttack || (sawAirAttack && p1.onGround && std::fabs(p1.y) <= 0.5f);
    }
    const auto airAttackAfter = runtime.snapshot().p1;
    const bool airAttackLanded = sawAir
        && sawAirAttack
        && landedAfterAttack
        && airAttackAfter.onGround
        && std::fabs(airAttackAfter.y) <= 0.5f;
    record(out, counts, airAttackLanded ? Status::Pass : Status::Fail, "air_attack_lands",
        "saw_air=" + std::to_string(sawAir ? 1 : 0)
        + " saw_air_attack=" + std::to_string(sawAirAttack ? 1 : 0)
        + " landed_after_attack=" + std::to_string(landedAfterAttack ? 1 : 0)
        + " y_min=" + std::to_string(yMin)
        + " final_y=" + std::to_string(airAttackAfter.y)
        + " final_vy=" + std::to_string(airAttackAfter.vy)
        + " final_state=" + std::to_string(airAttackAfter.stateNo)
        + " final_anim=" + std::to_string(airAttackAfter.action)
        + " final_state_type=" + std::string(1, airAttackAfter.stateType)
        + " final_on_ground=" + std::to_string(airAttackAfter.onGround ? 1 : 0));

    const auto kungFuKnee = observeKungFuKneePosSetGrounding(runtime);
    record(out, counts, kungFuKneeGroundingPassed(kungFuKnee) ? Status::Pass : Status::Fail,
        "kung_fu_knee_posset_grounding", kungFuKneeGroundingDetail(kungFuKnee));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenSmoke(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-smoke");
    const bool activeFight = waitForActiveFight(runtime, 420);
    const auto fightReady = runtime.snapshot();
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "single_player_fight_phase_ready",
        "match_phase=" + std::to_string(fightReady.matchPhase)
        + " timer_ticks=" + std::to_string(fightReady.matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "evilken_smoke_checks", "Single Player fight phase was not active");
        summary(out, counts);
        return exitCode(counts);
    }

    const bool settled = waitForControllableIdle(runtime, 360);
    const auto idle = runtime.snapshot().p1;
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(idle.stateNo)
        + " anim=" + std::to_string(idle.action)
        + " ctrl=" + std::to_string(idle.ctrl ? 1 : 0));
    if (!settled) {
        record(out, counts, Status::Blocked, "evilken_smoke_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.step({}, 150);
    const auto stance = runtime.snapshot().p1;
    record(out, counts, (stance.life > 0 && stance.stateNo == 0) ? Status::Pass : Status::Fail, "load_idle_stance",
        "state=" + std::to_string(stance.stateNo)
        + " anim=" + std::to_string(stance.action)
        + " life=" + std::to_string(stance.life));
    const float xBefore = runtime.snapshot().p1.x;
    runtime.step(SymbolicInput{ .right = true }, 45);
    const float xAfter = runtime.snapshot().p1.x;
    record(out, counts, std::fabs(xAfter - xBefore) > 1.0f ? Status::Pass : Status::Fail, "movement",
        "x_before=" + std::to_string(xBefore) + " x_after=" + std::to_string(xAfter)
        + " delta=" + std::to_string(xAfter - xBefore));
    bool sawAir = false;
    runtime.step(SymbolicInput{ .up = true }, 4);
    for (int i = 0; i < 90; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        sawAir = sawAir || !p1.onGround || p1.stateType == 'A';
    }
    record(out, counts, sawAir ? Status::Pass : Status::Fail, "jump_airborne", "airborne_observed=" + std::to_string(sawAir ? 1 : 0));
    const std::string preProbeHitText = runtime.snapshot().lastHitText;
    const bool preProbeHitEvidence = preProbeHitText.find(" hit ") != std::string::npos
        || preProbeHitText.find(" guard ") != std::string::npos;
    runtime.positionFighters(-40.0f, 160.0f);
    waitForControllableIdle(runtime, 360);
    runtime.forceFighterLiedown(1, 999);
    runtime.forceFighterState(0, 0);
    runtime.setFighterControl(0, true);
    runtime.step({}, 10);
    runtime.forceFighterLiedown(1, 999);
    runtime.forceFighterState(0, 0);
    runtime.setFighterControl(0, true);
    char command = '?';
    FighterSnapshot before;
    FighterSnapshot after;
    const bool normal = tryNormal(runtime, command, before, after, false);
    record(out, counts, normal ? Status::Pass : Status::Fail, "normal_attack",
        normal ? stateActionDetail(before, after, command)
               : "no x/y/z/a/b/c state or animation change; before="
                    + stateActionDetail(before, before, command)
                    + " after=" + stateActionDetail(after, after, command)
                    + " before_ctrl=" + std::to_string(before.ctrl ? 1 : 0)
                    + " after_ctrl=" + std::to_string(after.ctrl ? 1 : 0)
                    + " before_movetype=" + std::string(1, before.moveType)
                    + " after_movetype=" + std::string(1, after.moveType));
    bool sawContactEvidence = preProbeHitEvidence;
    int peakComboHits = 0;
    std::string lastHitText = preProbeHitText;
    for (int i = 0; i < 90; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        peakComboHits = std::max(peakComboHits, snap.comboHits);
        if (snap.lastHitText.find(" hit ") != std::string::npos || snap.lastHitText.find(" guard ") != std::string::npos) {
            lastHitText = snap.lastHitText;
        }
        sawContactEvidence = sawContactEvidence
            || snap.comboHits > 0
            || snap.p1.moveContact
            || snap.p1.moveHit
            || snap.p1.moveGuarded
            || snap.lastHitText.find(" hit ") != std::string::npos
            || snap.lastHitText.find(" guard ") != std::string::npos;
    }
    record(out, counts, sawContactEvidence ? Status::Pass : Status::Fail, "combo_or_hit_evidence",
        "peak_combo_hits=" + std::to_string(peakComboHits) + " last_hit=\"" + lastHitText + "\"");

    const auto timerBefore = runtime.snapshot();
    runtime.step({}, 30);
    const auto timerAfter = runtime.snapshot();
    const bool timerStable = timerAfter.matchPhase == static_cast<int>(MatchPhase::Fight)
        && timerAfter.matchTimerTicks > 0
        && timerAfter.matchTimerTicks <= timerBefore.matchTimerTicks;
    record(out, counts, timerStable ? Status::Pass : Status::Fail, "round_timer_stability",
        "phase_before=" + std::to_string(timerBefore.matchPhase)
        + " phase_after=" + std::to_string(timerAfter.matchPhase)
        + " timer_before=" + std::to_string(timerBefore.matchTimerTicks)
        + " timer_after=" + std::to_string(timerAfter.matchTimerTicks));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runLiliSmoke(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("Lili_QYC_Normal", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili_QYC_Normal/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "lili-smoke");

    const bool settled = waitForControllableIdle(runtime, 420);
    const auto idle = runtime.snapshot();
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(idle.p1.stateNo)
        + " anim=" + std::to_string(idle.p1.action)
        + " ctrl=" + std::to_string(idle.p1.ctrl ? 1 : 0));
    record(out, counts,
        idle.p1CompatibilityProfile == "Mugen2001" && idle.p1UsesMugenSemantics
            ? Status::Pass : Status::Fail,
        "compatibility_context",
        "profile=" + idle.p1CompatibilityProfile
            + " localcoord=" + std::to_string(idle.p1LocalCoordWidth)
            + "," + std::to_string(idle.p1LocalCoordHeight)
            + " mugen_semantics=" + std::to_string(idle.p1UsesMugenSemantics ? 1 : 0));
    record(out, counts,
        idle.p1.scaleX > 0.0f && idle.p1.scaleY > 0.0f
            && idle.p1.scaleX < 1.0f && idle.p1.scaleY < 1.0f
            ? Status::Pass : Status::Fail,
        "size_scale_applied",
        "scale_x=" + std::to_string(idle.p1.scaleX)
            + " scale_y=" + std::to_string(idle.p1.scaleY));
    if (!settled) {
        record(out, counts, Status::Blocked, "lili_smoke_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.step({}, 90);
    const auto stance = runtime.snapshot().p1;
    record(out, counts, (stance.life > 0 && stance.stateNo == 0) ? Status::Pass : Status::Fail, "load_idle_stance",
        "state=" + std::to_string(stance.stateNo)
        + " anim=" + std::to_string(stance.action)
        + " life=" + std::to_string(stance.life));

    const float xBefore = runtime.snapshot().p1.x;
    runtime.step(SymbolicInput{ .right = true }, 45);
    const float xAfter = runtime.snapshot().p1.x;
    record(out, counts, std::fabs(xAfter - xBefore) > 1.0f ? Status::Pass : Status::Fail, "movement",
        "x_before=" + std::to_string(xBefore)
        + " x_after=" + std::to_string(xAfter)
        + " delta=" + std::to_string(xAfter - xBefore));

    bool sawAir = false;
    runtime.step(SymbolicInput{ .up = true }, 4);
    for (int i = 0; i < 90; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        sawAir = sawAir || !p1.onGround || p1.stateType == 'A';
    }
    record(out, counts, sawAir ? Status::Pass : Status::Fail, "jump_airborne",
        "airborne_observed=" + std::to_string(sawAir ? 1 : 0));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runCharacterAutoFitScale(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("Lili_QYC_Normal", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "oversized character fixture setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "character-auto-fit-scale");

    const bool oversizedSettled = waitForControllableIdle(runtime, 420);
    const auto oversized = runtime.snapshot();
    record(out, counts, oversizedSettled ? Status::Pass : Status::Fail, "oversized_fixture_idle_ready",
        "state=" + std::to_string(oversized.p1.stateNo)
        + " ctrl=" + std::to_string(oversized.p1.ctrl ? 1 : 0));
    record(out, counts,
        oversized.p1.scaleX > 0.0f && oversized.p1.scaleY > 0.0f
            && oversized.p1.scaleX < 1.0f && oversized.p1.scaleY < 1.0f
            ? Status::Pass : Status::Fail,
        "oversized_character_scaled_to_fit",
        "scale_x=" + std::to_string(oversized.p1.scaleX)
            + " scale_y=" + std::to_string(oversized.p1.scaleY));

    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "normal_fixture_setup", "KFM/Mountainside Training setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    const bool normalSettled = waitForControllableIdle(runtime, 240);
    const auto normal = runtime.snapshot();
    record(out, counts, normalSettled ? Status::Pass : Status::Fail, "normal_fixture_idle_ready",
        "state=" + std::to_string(normal.p1.stateNo)
        + " ctrl=" + std::to_string(normal.p1.ctrl ? 1 : 0));
    record(out, counts,
        std::abs(normal.p1.scaleX - 1.0f) < 0.001f && std::abs(normal.p1.scaleY - 1.0f) < 0.001f
            ? Status::Pass : Status::Fail,
        "normal_character_keeps_authored_scale",
        "scale_x=" + std::to_string(normal.p1.scaleX)
            + " scale_y=" + std::to_string(normal.p1.scaleY));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runLiliChangeAnim2Fallback(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("Lili_QYC_Normal", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili_QYC_Normal/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "lili-changeanim2-fallback");

    const bool settled = waitForControllableIdle(runtime, 420);
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!settled) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.forceFighterState(0, 950);
    runtime.setFighterHitStun(0, 30);
    runtime.step({}, 2);
    const auto snap = runtime.snapshot();
    const bool usesAuthoredThrowFallback = snap.p1.stateNo == 950 && snap.p1.action == 850;
    record(out, counts, usesAuthoredThrowFallback ? Status::Pass : Status::Fail, "missing_changeanim2_uses_authored_throw_fallback",
        "p1_state=" + std::to_string(snap.p1.stateNo)
            + " p1_action=" + std::to_string(snap.p1.action)
            + " clsn2=" + std::to_string(snap.p1Clsn2Count));
    record(out, counts, snap.p1.action != 5030 ? Status::Pass : Status::Fail, "missing_changeanim2_avoids_common_fall_box",
        "p1_action=" + std::to_string(snap.p1.action)
            + " clsn2=" + std::to_string(snap.p1Clsn2Count));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

std::string compactFighterStateText(const FighterSnapshot& fighter) {
    return "state=" + std::to_string(fighter.stateNo)
        + " action=" + std::to_string(fighter.action)
        + " time=" + std::to_string(fighter.stateTime)
        + " x=" + std::to_string(fighter.x)
        + " y=" + std::to_string(fighter.y)
        + " vx=" + std::to_string(fighter.vx)
        + " vy=" + std::to_string(fighter.vy)
        + " type=" + std::string(1, fighter.stateType)
        + " move=" + std::string(1, fighter.moveType)
        + " physics=" + std::string(1, fighter.physics)
        + " ground=" + std::to_string(fighter.onGround ? 1 : 0)
        + " ctrl=" + std::to_string(fighter.ctrl ? 1 : 0);
}

int runLiliKuuchStateFallback(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("Lili_QYC_Custom", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili_QYC_Custom/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "lili-kuuch-state-fallback");

    const bool settled = waitForControllableIdle(runtime, 420);
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!settled) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.forceFighterState(0, 637);
    runtime.setFighterPosition(0, -24.0f, -72.0f);
    runtime.setFighterControl(0, false);
    runtime.step({}, 1);
    const auto forced = runtime.snapshot();
    record(out, counts,
        forced.p1.stateNo == 637 && forced.p1.action == 635 ? Status::Pass : Status::Fail,
        "missing_state_anim_637_falls_back_to_action_635",
        "p1=" + compactFighterStateText(forced.p1)
            + " clsn1=" + std::to_string(forced.p1Clsn1Count)
            + " clsn2=" + std::to_string(forced.p1Clsn2Count));

    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -24.0f, -72.0f);
    runtime.setFighterPosition(1, 16.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.forceFighterState(0, 637);
    runtime.setFighterPosition(0, -24.0f, -72.0f);
    runtime.setFighterPosition(1, 16.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool sawHit = false;
    bool sawNormalHardHitstun = false;
    bool sawUnexpectedFallState = false;
    bool p1Recovered = false;
    bool p2Recovered = false;
    FighterSnapshot hitP2;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    std::string lastHitText;
    for (int frame = 0; frame < 220; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
        if (snap.lastHitText.find("P1 hit 637#") != std::string::npos) {
            sawHit = true;
        }
        if (snap.p2.moveType == 'H' && (snap.p2.stateNo == 5000 || snap.p2.stateNo == 5001)) {
            sawNormalHardHitstun = true;
            hitP2 = snap.p2;
        }
        sawUnexpectedFallState = sawUnexpectedFallState
            || snap.p2.stateNo == 5030
            || snap.p2.stateNo == 5050
            || snap.p2.stateNo == 5070
            || snap.p2.stateNo == 5090
            || snap.p2.stateNo == 5100
            || snap.p2.stateNo == 5110
            || snap.p2.stateType == 'L';
        if (sawNormalHardHitstun) {
            p1Recovered = p1Recovered || (snap.p1.stateNo == 0 && snap.p1.onGround && snap.p1.ctrl);
            p2Recovered = p2Recovered || (snap.p2.stateNo == 0 && snap.p2.onGround && snap.p2.ctrl);
        }
        if (p1Recovered && p2Recovered && sawNormalHardHitstun) {
            break;
        }
        runtime.step({}, 1);
    }

    record(out, counts, sawHit ? Status::Pass : Status::Fail, "kuuch_hitdef_connects",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawNormalHardHitstun ? Status::Pass : Status::Fail, "dummy_enters_normal_hard_hitstun",
        "hit_p2=" + compactFighterStateText(hitP2)
            + " final_p2=" + compactFighterStateText(finalP2));
    record(out, counts, !sawUnexpectedFallState ? Status::Pass : Status::Fail, "kuuch_does_not_force_dummy_fall_state",
        "saw_fall_state=" + std::to_string(sawUnexpectedFallState ? 1 : 0)
            + " final_p2=" + compactFighterStateText(finalP2));
    record(out, counts, p1Recovered && p2Recovered ? Status::Pass : Status::Fail, "fighters_recover_after_kuuch",
        "p1_recovered=" + std::to_string(p1Recovered ? 1 : 0)
            + " p2_recovered=" + std::to_string(p2Recovered ? 1 : 0)
            + " final_p1=" + compactFighterStateText(finalP1)
            + " final_p2=" + compactFighterStateText(finalP2));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runLiliHienHououKyakuDemo(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("Lili_QYC_Normal", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili_QYC_Normal/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "lili-hien-houou-kyaku-demo");

    const bool settled = waitForControllableIdle(runtime, 420);
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!settled) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto moves = runtime.trainingMoves();
    int hienIndex = -1;
    for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
        const std::string label = lowercaseAsciiCopy(moves[static_cast<size_t>(i)].label);
        if (label == "hien-houou-kyaku" || moves[static_cast<size_t>(i)].targetState == 3000) {
            hienIndex = i;
            break;
        }
    }
    const bool selected = hienIndex >= 0 && runtime.selectTrainingMoveIndex(hienIndex);
    record(out, counts, selected ? Status::Pass : Status::Fail, "selected_hien_houou_kyaku",
        "index=" + std::to_string(hienIndex)
            + " moves=" + std::to_string(moves.size()));
    if (!selected) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.startTrainingCommandDemo();

    bool sawOpeningState = false;
    bool sawMiddleState = false;
    bool sawFinisherState = false;
    bool sawActiveBoxes = false;
    bool sawHit = false;
    bool p1Recovered = false;
    bool p2Recovered = false;
    int maxP2Clsn1 = 0;
    int maxEffects = 0;
    int maxAfterImageTrails = 0;
    FighterSnapshot hitP1;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    std::string lastHitText;
    for (int frame = 0; frame < 420; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        maxP2Clsn1 = std::max(maxP2Clsn1, snap.p2Clsn1Count);
        maxEffects = std::max(maxEffects, snap.activeEffects);
        maxAfterImageTrails = std::max({ maxAfterImageTrails, snap.p1.afterImageTrailCount, snap.p2.afterImageTrailCount });
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
        sawOpeningState = sawOpeningState || (snap.p2.stateNo == 3000 && snap.p2.action == 2000);
        sawMiddleState = sawMiddleState || (snap.p2.stateNo == 2020 && snap.p2.action == 2020);
        sawFinisherState = sawFinisherState || (snap.p2.stateNo == 2030 && snap.p2.action == 2030);
        sawActiveBoxes = sawActiveBoxes
            || ((snap.p2.stateNo == 3000 || snap.p2.stateNo == 2020 || snap.p2.stateNo == 2030)
                && snap.p2Clsn1Count > 0);
        sawHit = sawHit
            || snap.lastHitText.find("P2 hit 3000#") != std::string::npos
            || snap.lastHitText.find("P2 hit 2020#") != std::string::npos
            || snap.lastHitText.find("P2 hit 2030#") != std::string::npos;
        if (snap.p1.moveType == 'H') {
            hitP1 = snap.p1;
        }
        if (sawHit) {
            p1Recovered = p1Recovered || (snap.p1.stateNo == 0 && snap.p1.onGround && snap.p1.ctrl);
            p2Recovered = p2Recovered || (snap.p2.stateNo == 0 && snap.p2.onGround && snap.p2.ctrl);
        }
        const bool afterImagesClear = !snap.p1.afterImageActive
            && !snap.p2.afterImageActive
            && snap.p1.afterImageTrailCount == 0
            && snap.p2.afterImageTrailCount == 0;
        if (sawOpeningState && sawMiddleState && sawFinisherState && sawActiveBoxes && p1Recovered && p2Recovered
            && snap.activeEffects == 0 && afterImagesClear) {
            break;
        }
        runtime.step({}, 1);
    }

    for (int i = 0; i < 90; ++i) {
        const auto snap = runtime.snapshot();
        const bool afterImagesClear = !snap.p1.afterImageActive
            && !snap.p2.afterImageActive
            && snap.p1.afterImageTrailCount == 0
            && snap.p2.afterImageTrailCount == 0;
        if (snap.activeEffects == 0 && afterImagesClear) {
            break;
        }
        runtime.step({}, 1);
    }
    const auto finalSnap = runtime.snapshot();
    finalP1 = finalSnap.p1;
    finalP2 = finalSnap.p2;

    record(out, counts, sawOpeningState ? Status::Pass : Status::Fail, "demo_enters_hien_opening",
        "p2=" + compactFighterStateText(finalP2));
    record(out, counts, sawMiddleState && sawFinisherState ? Status::Pass : Status::Fail, "demo_reaches_full_chain",
        "saw_2020=" + std::to_string(sawMiddleState ? 1 : 0)
            + " saw_2030=" + std::to_string(sawFinisherState ? 1 : 0));
    record(out, counts, sawActiveBoxes ? Status::Pass : Status::Fail, "demo_has_active_clsn1",
        "max_p2_clsn1=" + std::to_string(maxP2Clsn1));
    record(out, counts, sawHit ? Status::Pass : Status::Fail, "demo_hits_dummy",
        "last_hit=\"" + lastHitText + "\" hit_p1=" + compactFighterStateText(hitP1));
    record(out, counts, p1Recovered && p2Recovered ? Status::Pass : Status::Fail, "fighters_recover_after_demo",
        "p1_recovered=" + std::to_string(p1Recovered ? 1 : 0)
            + " p2_recovered=" + std::to_string(p2Recovered ? 1 : 0)
            + " final_p1=" + compactFighterStateText(finalP1)
            + " final_p2=" + compactFighterStateText(finalP2));
    record(out, counts, finalSnap.activeEffects == 0 ? Status::Pass : Status::Fail, "demo_effects_clear",
        "active_effects=" + std::to_string(finalSnap.activeEffects)
            + " max_effects=" + std::to_string(maxEffects));
    const bool finalAfterImagesClear = !finalSnap.p1.afterImageActive
        && !finalSnap.p2.afterImageActive
        && finalSnap.p1.afterImageTrailCount == 0
        && finalSnap.p2.afterImageTrailCount == 0;
    record(out, counts, finalAfterImagesClear ? Status::Pass : Status::Fail, "demo_afterimages_clear",
        "p1_active=" + std::to_string(finalSnap.p1.afterImageActive ? 1 : 0)
            + " p1_trail=" + std::to_string(finalSnap.p1.afterImageTrailCount)
            + " p2_active=" + std::to_string(finalSnap.p2.afterImageActive ? 1 : 0)
            + " p2_trail=" + std::to_string(finalSnap.p2.afterImageTrailCount)
            + " max_trail=" + std::to_string(maxAfterImageTrails));
    if (const char* screenshotPath = std::getenv("DRAGON_SCREENSHOT_PATH"); screenshotPath && *screenshotPath) {
        const bool captured = runtime.captureScreenshot(std::filesystem::path(screenshotPath));
        record(out, counts, captured ? Status::Pass : Status::Fail, "screenshot_captured", screenshotPath);
    }
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}


} // namespace dragon::verification
