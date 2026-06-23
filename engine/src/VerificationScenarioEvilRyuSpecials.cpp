#include "VerificationScenarioSpecialsCommon.h"

namespace dragon::verification {

int runEvilRyuPowerChargeHelper(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-power-charge-helper");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-70.0f, 80.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.setFighterPower(0, 0);

    SymbolicInput charge;
    charge.b = true;
    charge.y = true;

    bool sawChargeLoop = false;
    bool sawAuthoredHelper = false;
    bool droppedWhileHeld = false;
    int peakHelpers = 0;
    int idleHelperFrames = 0;
    int observedHelperState = 0;
    int observedHelperAction = 0;
    FighterSnapshot chargeFighter;
    for (int i = 0; i < 140; ++i) {
        runtime.step(charge, 1);
        const auto snapshot = runtime.snapshot();
        chargeFighter = snapshot.p1;
        sawChargeLoop = sawChargeLoop || snapshot.p1.stateNo == 1051;
        if (i > 40 && snapshot.p1.stateNo != 1051) {
            droppedWhileHeld = true;
        }
        const bool helperUsesAuthoredState =
            snapshot.firstHelperState == 94061
            || snapshot.firstHelperState == 94062
            || snapshot.firstHelperState == 94063;
        if (helperUsesAuthoredState) {
            sawAuthoredHelper = true;
            observedHelperState = snapshot.firstHelperState;
            observedHelperAction = snapshot.firstHelperAction;
        }
        peakHelpers = std::max(peakHelpers, snapshot.activeHelpers);
        if (snapshot.idleHelpers > 0) {
            ++idleHelperFrames;
        }
    }

    record(out, counts, sawChargeLoop && !droppedWhileHeld ? Status::Pass : Status::Fail, "charge_loop_held_stable",
        "state=" + std::to_string(chargeFighter.stateNo)
        + " action=" + std::to_string(chargeFighter.action)
        + " power=" + std::to_string(chargeFighter.power)
        + " dropped=" + std::to_string(droppedWhileHeld ? 1 : 0));
    record(out, counts, sawAuthoredHelper && peakHelpers > 0 ? Status::Pass : Status::Fail, "charge_helper_uses_authored_state",
        "peak_helpers=" + std::to_string(peakHelpers)
        + " observed_helper_state=" + std::to_string(observedHelperState)
        + " observed_helper_action=" + std::to_string(observedHelperAction));
    record(out, counts, idleHelperFrames == 0 ? Status::Pass : Status::Fail, "charge_helper_not_idle_clone",
        "idle_helper_frames=" + std::to_string(idleHelperFrames)
        + " observed_helper_state=" + std::to_string(observedHelperState)
        + " observed_helper_action=" + std::to_string(observedHelperAction));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilRyuThrowBind(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-throw-bind");

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
    runtime.forceFighterState(0, 900);
    runtime.setFighterPosition(0, -4.0f, 0.0f);
    runtime.setFighterPosition(1, 4.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool sawThrowHit = false;
    bool sawThrowState = false;
    bool sawVictimCustomState = false;
    bool sawReleaseFall = false;
    float maxBoundDistance = 0.0f;
    float closestBoundDistance = 100000.0f;
    std::string lastHitText;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    for (int frame = 0; frame < 180; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
        sawThrowHit = sawThrowHit || snap.lastHitText.find("P1 hit 900#") != std::string::npos;
        sawThrowState = sawThrowState || snap.p1.stateNo == 920;
        sawVictimCustomState = sawVictimCustomState || snap.p2.stateNo == 925;
        sawReleaseFall = sawReleaseFall || snap.p2.stateNo == 5050;
        if (snap.p1.stateNo == 920 && snap.p2.stateNo == 925) {
            const float distance = std::fabs(snap.p2.x - snap.p1.x);
            maxBoundDistance = std::max(maxBoundDistance, distance);
            closestBoundDistance = std::min(closestBoundDistance, distance);
        }
        runtime.step({}, 1);
    }

    record(out, counts, sawThrowHit ? Status::Pass : Status::Fail, "throw_hitdef_connected",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawThrowState && sawVictimCustomState ? Status::Pass : Status::Fail, "custom_throw_states_entered",
        "p1_state=" + std::to_string(finalP1.stateNo)
        + " p2_state=" + std::to_string(finalP2.stateNo));
    record(out, counts, maxBoundDistance > 0.0f && maxBoundDistance <= 45.0f ? Status::Pass : Status::Fail, "targetbind_kept_throw_spacing",
        "max_bound_distance=" + std::to_string(maxBoundDistance)
        + " closest_bound_distance=" + std::to_string(closestBoundDistance)
        + " final_p1_x=" + std::to_string(finalP1.x)
        + " final_p2_x=" + std::to_string(finalP2.x));
    record(out, counts, sawReleaseFall ? Status::Pass : Status::Fail, "throw_released_to_common_fall",
        "final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_action=" + std::to_string(finalP2.action)
        + " final_p2_y=" + std::to_string(finalP2.y));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}


int runEvilRyuSpecialsSupers(RuntimeProbe& runtime, std::ostream& out) {
    return runShotoSpecialsSupers(runtime, out, "EvilRyu", "evilryu-specials-supers", { 3885 }, 0, 11164, 0, 950, {
        { "super_first_hit_damage_matches_authored", 3885, 0.0f, 105.0f, 60, 180, 28, 3 },
    });
}

int runEvilRyuShinShoryukenStun(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-shin-shoryuken-stun");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setFighterPosition(0, -36.0f, 0.0f);
    runtime.setFighterPosition(1, -8.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.setFighterPower(0, 3000);
    runtime.setFighterLife(1, 1000);
    runtime.setFighterVar(0, 28, 0);
    runtime.forceFighterState(0, 3480);

    bool sawOpeningHit = false;
    bool heldHitRuntime = true;
    bool noCommandlessAirRecovery = true;
    bool recoveredGameplay = false;
    int firstHitFrame = -1;
    int observed3480Hits = 0;
    int maxP2HitStun = 0;
    int secondHitStun = 0;
    int minEarlyHitStunAfterPause = 100000;
    int earlyFramesChecked = 0;
    std::string lastHitText;
    std::string previousHitText;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;

    for (int frame = 0; frame < 760; ++frame) {
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }
        if (snap.lastHitText.find("P1 hit 3480#") != std::string::npos
            && snap.lastHitText != previousHitText) {
            ++observed3480Hits;
            previousHitText = snap.lastHitText;
        }
        if (!sawOpeningHit && snap.lastHitText.find("P1 hit 3480#") != std::string::npos) {
            sawOpeningHit = true;
            firstHitFrame = frame;
        }
        maxP2HitStun = std::max(maxP2HitStun, snap.p2.hitStunTicks);
        if (snap.lastHitText.find("P1 hit 3480#112") != std::string::npos) {
            secondHitStun = std::max(secondHitStun, snap.p2.hitStunTicks);
        }

        if (sawOpeningHit && frame - firstHitFrame <= 170) {
            ++earlyFramesChecked;
            heldHitRuntime = heldHitRuntime
                && snap.p2.moveType == 'H'
                && !snap.p2.ctrl
                && snap.p2.stateNo != 0;
            noCommandlessAirRecovery = noCommandlessAirRecovery
                && snap.p2.stateNo != 5040
                && snap.p2.stateNo != 5140
                && snap.p2.stateNo != 5200
                && snap.p2.stateNo != 5210;
            if (snap.p2.hitPauseTicks <= 0) {
                minEarlyHitStunAfterPause = std::min(minEarlyHitStunAfterPause, snap.p2.hitStunTicks);
            }
        }

        recoveredGameplay = frame > 220
            && snap.globalPauseTicks == 0
            && snap.p1.stateNo == 0
            && snap.p1.moveType == 'I'
            && snap.p2.stateNo == 0
            && snap.p2.moveType == 'I'
            && snap.p2.onGround;
        if (recoveredGameplay) {
            break;
        }

        runtime.step({}, 1);
    }

    record(out, counts, observed3480Hits >= 2 ? Status::Pass : Status::Fail, "opening_hits_observed",
        "hits=" + std::to_string(observed3480Hits)
        + " last_hit=\"" + lastHitText + "\"");
    record(out, counts, maxP2HitStun >= 180 ? Status::Pass : Status::Fail, "first_hit_uses_long_stun",
        "max_hitstun=" + std::to_string(maxP2HitStun));
    record(out, counts, secondHitStun >= 80 ? Status::Pass : Status::Fail, "second_hit_uses_stun",
        "second_hitstun=" + std::to_string(secondHitStun));
    record(out, counts, sawOpeningHit && heldHitRuntime && earlyFramesChecked > 0 ? Status::Pass : Status::Fail,
        "opening_hits_keep_defender_in_hit_runtime",
        "checked=" + std::to_string(earlyFramesChecked)
        + " min_hitstun_after_pause=" + std::to_string(minEarlyHitStunAfterPause)
        + " final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_ctrl=" + std::to_string(finalP2.ctrl ? 1 : 0));
    record(out, counts, noCommandlessAirRecovery ? Status::Pass : Status::Fail,
        "no_commandless_air_recovery_during_super",
        "final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_y=" + std::to_string(finalP2.y)
        + " final_p2_hitstun=" + std::to_string(finalP2.hitStunTicks));
    record(out, counts, recoveredGameplay ? Status::Pass : Status::Fail, "super_recovers_gameplay",
        "final_p1_state=" + std::to_string(finalP1.stateNo)
        + " final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_move=" + std::string(1, finalP2.moveType));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilRyuSuperStress(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-super-stress");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    bool allCyclesRecovered = true;
    bool sawAnySuperPause = false;
    bool sawAnyPauseClear = false;
    bool sawAnyHelper = false;
    bool sawAnyHit = false;
    bool sawAnyAirPop = false;
    int maxPause = 0;
    int maxHelpers = 0;
    std::string lastHitText;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;

    for (int cycle = 0; cycle < 4; ++cycle) {
        runtime.positionFighters(-32.0f, -16.0f);
        runtime.setFighterPosition(0, -32.0f, 0.0f);
        runtime.setFighterPosition(1, -16.0f, 0.0f);
        runtime.setFighterControl(0, false);
        runtime.setFighterControl(1, false);
        runtime.setFighterPower(0, 3000);
        runtime.setFighterVar(0, 28, 0);
        runtime.forceFighterLiedown(1, 180);
        runtime.forceFighterState(0, 4800);

        bool cycleSawSuperPause = false;
        bool cycleSawPauseClear = false;
        bool cycleSawHelper = false;
        bool cycleSawHit = false;
        bool cycleSawAirPop = false;
        bool cycleRecovered = false;
        for (int frame = 0; frame < 320; ++frame) {
            runtime.step({}, 1);
            const auto snapshot = runtime.snapshot();
            finalP1 = snapshot.p1;
            finalP2 = snapshot.p2;
            maxPause = std::max(maxPause, snapshot.globalPauseTicks);
            maxHelpers = std::max(maxHelpers, snapshot.activeHelpers);
            cycleSawSuperPause = cycleSawSuperPause || (snapshot.globalPauseIsSuper && snapshot.globalPauseTicks > 0);
            cycleSawPauseClear = cycleSawPauseClear || (cycleSawSuperPause && snapshot.globalPauseTicks == 0);
            cycleSawHelper = cycleSawHelper || snapshot.activeHelpers > 0;
            cycleSawHit = cycleSawHit
                || snapshot.p1.moveHit
                || snapshot.comboHits > 0
                || snapshot.lastHitText.find("P1 hit") != std::string::npos;
            cycleSawAirPop = cycleSawAirPop
                || snapshot.p2.stateNo == 5030
                || snapshot.p2.stateNo == 5050
                || snapshot.p2.stateNo == 5100
                || (!snapshot.p2.onGround && snapshot.p2.moveType == 'H');
            if (!snapshot.lastHitText.empty()) {
                lastHitText = snapshot.lastHitText;
            }
            cycleRecovered = snapshot.globalPauseTicks == 0
                && snapshot.p1.stateNo == 0
                && snapshot.p1.moveType == 'I'
                && snapshot.p2.stateNo == 0
                && snapshot.p2.moveType == 'I'
                && snapshot.p2.onGround
                && snapshot.activeHelpers <= 1;
            if (cycleRecovered) {
                break;
            }
        }

        sawAnySuperPause = sawAnySuperPause || cycleSawSuperPause;
        sawAnyPauseClear = sawAnyPauseClear || cycleSawPauseClear;
        sawAnyHelper = sawAnyHelper || cycleSawHelper;
        sawAnyHit = sawAnyHit || cycleSawHit;
        sawAnyAirPop = sawAnyAirPop || cycleSawAirPop;
        allCyclesRecovered = allCyclesRecovered && cycleRecovered;
    }

    record(out, counts, sawAnySuperPause ? Status::Pass : Status::Fail, "kongou_superpause_observed",
        "max_pause=" + std::to_string(maxPause));
    record(out, counts, sawAnyPauseClear ? Status::Pass : Status::Fail, "repeated_superpause_clears",
        "final_pause=" + std::to_string(runtime.snapshot().globalPauseTicks)
        + " max_pause=" + std::to_string(maxPause));
    record(out, counts, sawAnyHelper ? Status::Pass : Status::Fail, "kongou_helpers_spawn",
        "max_helpers=" + std::to_string(maxHelpers));
    record(out, counts, sawAnyHit ? Status::Pass : Status::Fail, "downed_dummy_gets_hit",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, sawAnyAirPop ? Status::Pass : Status::Fail, "downed_fall_velocity_enters_air_path",
        "final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_y=" + std::to_string(finalP2.y)
        + " final_p2_hitstun=" + std::to_string(finalP2.hitStunTicks));
    record(out, counts, allCyclesRecovered ? Status::Pass : Status::Fail, "repeated_kongou_recovers_gameplay",
        "final_p1_state=" + std::to_string(finalP1.stateNo)
        + " final_p1_time=" + std::to_string(finalP1.stateTime)
        + " final_p2_state=" + std::to_string(finalP2.stateNo)
        + " final_p2_time=" + std::to_string(finalP2.stateTime)
        + " final_pause=" + std::to_string(runtime.snapshot().globalPauseTicks)
        + " helpers=" + std::to_string(runtime.snapshot().activeHelpers));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilRyuAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-air-special-contact-landing");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-36.0f, 28.0f);
    runtime.forceFighterState(0, 4054);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -36.0f, -68.0f);
    runtime.setFighterPosition(1, 28.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool saw4055 = false;
    bool sawLanding = false;
    bool sawIdle = false;
    bool sawContact = false;
    bool sawStale4055 = false;
    int peakHelpers = 0;
    FighterSnapshot last = runtime.snapshot().p1;
    std::string hitText;
    for (int i = 0; i < 220; ++i) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        last = snapshot.p1;
        peakHelpers = std::max(peakHelpers, snapshot.activeHelpers);
        saw4055 = saw4055 || snapshot.p1.stateNo == 4055;
        sawLanding = sawLanding || snapshot.p1.stateNo == 4044;
        sawIdle = sawIdle || (snapshot.p1.stateNo == 0 && snapshot.p1.ctrl && snapshot.p1.onGround);
        sawContact = sawContact
            || snapshot.p1.moveContact
            || snapshot.p1.moveHit
            || snapshot.p1.moveGuarded
            || snapshot.lastHitText.find("P1 hit") != std::string::npos
            || snapshot.lastHitText.find("P1 guard") != std::string::npos;
        if (!snapshot.lastHitText.empty()) {
            hitText = snapshot.lastHitText;
        }
        sawStale4055 = sawStale4055 || (snapshot.p1.stateNo == 4055 && snapshot.p1.stateTime > 120);
        if (sawIdle) {
            break;
        }
    }

    const bool leftAirSpecial = last.stateNo != 4054 && last.stateNo != 4055;
    record(out, counts, saw4055 ? Status::Pass : Status::Fail, "air_chain_reaches_descent_4055",
        "final_state=" + std::to_string(last.stateNo)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_y=" + std::to_string(last.y)
        + " global_pause=" + std::to_string(runtime.snapshot().globalPauseTicks));
    record(out, counts, sawContact ? Status::Pass : Status::Fail, "air_special_contact_observed",
        "peak_helpers=" + std::to_string(peakHelpers)
        + " hit=\"" + hitText + "\"");
    record(out, counts, sawLanding ? Status::Pass : Status::Fail, "air_special_lands_via_4044",
        "final_state=" + std::to_string(last.stateNo)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_y=" + std::to_string(last.y));
    record(out, counts, sawIdle ? Status::Pass : Status::Fail, "air_special_recovers_idle",
        "final_state=" + std::to_string(last.stateNo)
        + " final_action=" + std::to_string(last.action)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_x=" + std::to_string(last.x)
        + " final_y=" + std::to_string(last.y));
    record(out, counts, leftAirSpecial && !sawStale4055 ? Status::Pass : Status::Fail, "air_special_not_stuck_in_4054_4055",
        "state=" + std::to_string(last.stateNo)
        + " time=" + std::to_string(last.stateTime)
        + " stale4055=" + std::to_string(sawStale4055 ? 1 : 0)
        + " helpers=" + std::to_string(runtime.snapshot().activeHelpers));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}


} // namespace dragon::verification
