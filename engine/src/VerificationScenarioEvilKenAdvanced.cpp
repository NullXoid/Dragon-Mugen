#include "VerificationScenarioEvilKenCommon.h"

namespace dragon::verification {

std::string fighterBriefText(const FighterSnapshot& fighter);

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

    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -4.0f, 0.0f);
    runtime.setFighterPosition(1, 4.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.step({}, 2);
    runtime.forceFighterState(0, 900);
    runtime.setFighterPosition(0, -4.0f, 0.0f);
    runtime.setFighterPosition(1, 4.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    SymbolicInput holdForward;
    holdForward.right = true;
    bool forwardP1ThrowState = false;
    bool forwardP1FacingApplied = false;
    bool forwardP2FacesP1 = false;
    bool forwardVictimTracksFlippedSide = false;
    FighterSnapshot forwardP1;
    FighterSnapshot forwardP2;
    for (int frame = 0; frame < 180; ++frame) {
        runtime.step(holdForward, 1);
        const auto snap = runtime.snapshot();
        if (snap.p1.stateNo == 920) {
            forwardP1ThrowState = true;
            forwardP1 = snap.p1;
            forwardP2 = snap.p2;
            forwardP1FacingApplied = forwardP1FacingApplied || snap.p1.facing < 0;
            forwardP2FacesP1 = forwardP2FacesP1
                || ((snap.p2.stateNo == 925 || snap.p2.stateNo == 926) && snap.p1.facing < 0 && snap.p2.facing > 0);
            forwardVictimTracksFlippedSide = forwardVictimTracksFlippedSide
                || (snap.p1.facing < 0 && snap.p2.x < snap.p1.x);
        }
        if (forwardP1FacingApplied && forwardP2FacesP1 && forwardVictimTracksFlippedSide) {
            break;
        }
    }

    record(out, counts, forwardP1ThrowState ? Status::Pass : Status::Fail, "forward_throw_enters_throw_state",
        "p1=" + fighterBriefText(forwardP1)
        + " p2=" + fighterBriefText(forwardP2));
    record(out, counts, forwardP1FacingApplied ? Status::Pass : Status::Fail, "forward_throw_applies_p1facing",
        "p1=" + fighterBriefText(forwardP1));
    record(out, counts, forwardP2FacesP1 ? Status::Pass : Status::Fail, "forward_throw_p2_faces_p1",
        "p1=" + fighterBriefText(forwardP1)
        + " p2=" + fighterBriefText(forwardP2));
    record(out, counts, forwardVictimTracksFlippedSide ? Status::Pass : Status::Fail, "forward_throw_victim_tracks_flipped_side",
        "p1=" + fighterBriefText(forwardP1)
        + " p2=" + fighterBriefText(forwardP2));
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
         << " x=" << fighter.x
         << " y=" << fighter.y
         << " vx=" << fighter.vx
         << " vy=" << fighter.vy
         << " facing=" << fighter.facing
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

int runEvilKenShoukiHatsudouSpacing(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-shouki-hatsudou-spacing");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -18.0f, 0.0f);
    runtime.setFighterPosition(1, 8.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.setFighterPower(0, 3000);
    runtime.setFighterLife(1, 1000);
    runtime.step({}, 2);
    runtime.forceFighterState(0, 4000);
    runtime.setFighterPosition(0, -18.0f, 0.0f);
    runtime.setFighterPosition(1, 8.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool sawStartup = false;
    bool sawSecondState = false;
    bool sawFinisherState = false;
    bool sawFirstHit = false;
    bool sawSecondHit = false;
    bool sawThirdHit = false;
    bool p1Recovered = false;
    bool p2RecoveredOrDown = false;
    float firstHitDistance = 0.0f;
    float secondHitDistance = 0.0f;
    float thirdHitDistance = 0.0f;
    float maxDistanceAfterFirstBeforeSecond = 0.0f;
    float maxChainDistance = 0.0f;
    float minP2X = 100000.0f;
    float maxP2X = -100000.0f;
    int maxP2HitstunAfterFirst = 0;
    int framesAfterFirstBeforeSecond = 0;
    FighterSnapshot firstHitP1;
    FighterSnapshot firstHitP2;
    FighterSnapshot secondHitP1;
    FighterSnapshot secondHitP2;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    std::string firstHitText;
    std::string secondHitText;
    std::string thirdHitText;
    std::string lastHitText;

    for (int frame = 0; frame < 520; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        sawStartup = sawStartup || snap.p1.stateNo == 4000;
        sawSecondState = sawSecondState || snap.p1.stateNo == 4005;
        sawFinisherState = sawFinisherState || snap.p1.stateNo == 4019 || snap.p1.stateNo == 4001;
        const float distance = std::fabs(snap.p2.x - snap.p1.x);
        maxChainDistance = std::max(maxChainDistance, distance);
        minP2X = std::min(minP2X, snap.p2.x);
        maxP2X = std::max(maxP2X, snap.p2.x);
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
        if (!sawFirstHit && snap.lastHitText.find("P1 hit 4000#155") != std::string::npos) {
            sawFirstHit = true;
            firstHitText = snap.lastHitText;
            firstHitDistance = distance;
            firstHitP1 = snap.p1;
            firstHitP2 = snap.p2;
        }
        if (sawFirstHit && !sawSecondHit) {
            ++framesAfterFirstBeforeSecond;
            maxDistanceAfterFirstBeforeSecond = std::max(maxDistanceAfterFirstBeforeSecond, distance);
            maxP2HitstunAfterFirst = std::max(maxP2HitstunAfterFirst, snap.p2.hitStunTicks);
        }
        if (!sawSecondHit && snap.lastHitText.find("P1 hit 4005#") != std::string::npos) {
            sawSecondHit = true;
            secondHitText = snap.lastHitText;
            secondHitDistance = distance;
            secondHitP1 = snap.p1;
            secondHitP2 = snap.p2;
        }
        if (!sawThirdHit && snap.lastHitText.find("P1 hit 4019#") != std::string::npos) {
            sawThirdHit = true;
            thirdHitText = snap.lastHitText;
            thirdHitDistance = distance;
        }
        p1Recovered = p1Recovered || recoveredGroundIdle(snap.p1);
        p2RecoveredOrDown = p2RecoveredOrDown
            || recoveredGroundIdle(snap.p2)
            || snap.p2.stateNo == 5110
            || snap.p2.stateNo == 5120
            || (snap.p2.onGround && snap.p2.stateType == 'L');
        if (sawThirdHit && p1Recovered && p2RecoveredOrDown) {
            break;
        }
        runtime.step({}, 1);
    }

    record(out, counts, sawStartup ? Status::Pass : Status::Fail, "shouki_startup_state_entered",
        "final_p1=" + fighterBriefText(finalP1));
    record(out, counts, sawFirstHit ? Status::Pass : Status::Fail, "first_hit_connected",
        "first_hit=\"" + firstHitText + "\" first_distance=" + std::to_string(firstHitDistance)
        + " p1=" + fighterBriefText(firstHitP1)
        + " p2=" + fighterBriefText(firstHitP2));
    const bool firstHitStayedInRange = sawFirstHit
        && maxP2HitstunAfterFirst >= 30
        && maxDistanceAfterFirstBeforeSecond <= 96.0f;
    record(out, counts, firstHitStayedInRange ? Status::Pass : Status::Fail, "first_hit_stays_in_combo_range",
        "max_distance_after_first_before_second=" + std::to_string(maxDistanceAfterFirstBeforeSecond)
        + " frames_after_first_before_second=" + std::to_string(framesAfterFirstBeforeSecond)
        + " max_p2_hitstun_after_first=" + std::to_string(maxP2HitstunAfterFirst)
        + " p2_x_min=" + std::to_string(minP2X)
        + " p2_x_max=" + std::to_string(maxP2X));
    record(out, counts, sawSecondState ? Status::Pass : Status::Fail, "second_state_reached",
        "final_p1=" + fighterBriefText(finalP1));
    record(out, counts, sawSecondHit ? Status::Pass : Status::Fail, "second_hit_connected",
        "second_hit=\"" + secondHitText + "\" second_distance=" + std::to_string(secondHitDistance)
        + " p1=" + fighterBriefText(secondHitP1)
        + " p2=" + fighterBriefText(secondHitP2)
        + " last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawFinisherState ? Status::Pass : Status::Fail, "finisher_state_reached",
        "final_p1=" + fighterBriefText(finalP1));
    record(out, counts, sawThirdHit ? Status::Pass : Status::Fail, "third_hit_connected",
        "third_hit=\"" + thirdHitText + "\" third_distance=" + std::to_string(thirdHitDistance)
        + " max_chain_distance=" + std::to_string(maxChainDistance)
        + " last_hit=\"" + lastHitText + "\"");
    record(out, counts, p1Recovered && p2RecoveredOrDown ? Status::Pass : Status::Fail, "sequence_resolves",
        "p1_recovered=" + std::to_string(p1Recovered ? 1 : 0)
        + " p2_recovered_or_down=" + std::to_string(p2RecoveredOrDown ? 1 : 0)
        + " final_p1=" + fighterBriefText(finalP1)
        + " final_p2=" + fighterBriefText(finalP2));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
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


} // namespace dragon::verification
