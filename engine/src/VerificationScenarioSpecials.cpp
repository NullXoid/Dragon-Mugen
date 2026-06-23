#include "VerificationScenarioSpecialsCommon.h"

namespace dragon::verification {

int runKfmSpecialsSupers(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "kfm-specials-supers");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }
    recordCharacterDataLifeLoaded(runtime, out, counts, 1000);

    const std::vector<SpecialProbeCase> groundSpecials{
        { "light_kung_fu_palm", qcfSequence('x'), { 1000 }, 0 },
        { "strong_kung_fu_palm", qcfSequence('y'), { 1010 }, 0 },
        { "fast_kung_fu_palm", qcfSequence({ 'x', 'y' }), { 1020 }, 330 },
        { "light_kung_fu_upper", dpSequence('x'), { 1100 }, 0 },
        { "strong_kung_fu_upper", dpSequence('y'), { 1110 }, 0 },
        { "fast_kung_fu_upper", dpSequence({ 'x', 'y' }), { 1120 }, 330 },
        { "light_kung_fu_knee", ffButtonSequence('a'), { 1050 }, 0 },
        { "strong_kung_fu_knee", ffButtonSequence('b'), { 1060 }, 0 },
        { "fast_kung_fu_knee", ffButtonSequence({ 'a', 'b' }), { 1070 }, 330 },
    };
    for (const auto& probe : groundSpecials) {
        runSpecialProbeCase(runtime, out, counts, probe);
    }

    FighterSnapshot observed;
    std::string commands;

    FighterSnapshot blockedObserved;
    std::string blockedCommands;
    const bool blockedReady = resetSpecialProbe(runtime, 0);
    const bool blockedSuper = blockedReady
        && stepSequenceAndObserve(runtime, doubleQcfSequence('x'), { 3000 }, blockedObserved, &blockedCommands, 45);
    record(out, counts, blockedReady && !blockedSuper ? Status::Pass : Status::Fail, "double_qcf_super_blocked_without_power",
        "ready=" + std::to_string(blockedReady ? 1 : 0)
        + " state=" + std::to_string(blockedObserved.stateNo)
        + " action=" + std::to_string(blockedObserved.action)
        + " power=" + std::to_string(blockedObserved.power)
        + " commands=" + blockedCommands);

    const bool superReady = resetSpecialProbe(runtime, 1000);
    const int powerBefore = runtime.snapshot().p1.power;
    const bool super = superReady && stepSequenceAndObserve(runtime, doubleQcfSequence('x'), { 3000 }, observed, &commands, 45);
    int powerAfter = 0;
    const bool powerConsumed = super && observePowerConsumed(runtime, powerBefore, powerAfter);
    record(out, counts, super ? Status::Pass : Status::Fail, "double_qcf_super_enters_state",
        "ready=" + std::to_string(superReady ? 1 : 0)
        + " state=" + std::to_string(observed.stateNo)
        + " action=" + std::to_string(observed.action)
        + " power_before=" + std::to_string(powerBefore)
        + " power_after=" + std::to_string(powerAfter)
        + " commands=" + commands);
    record(out, counts, powerConsumed ? Status::Pass : Status::Fail, "super_consumes_power",
        "power_before=" + std::to_string(powerBefore)
        + " power_after=" + std::to_string(powerAfter));

    recordForcedDamageProbe(runtime, out, counts, {
        "super_first_hit_damage_matches_authored",
        3000,
        -32.0f,
        24.0f,
        72,
        120,
    });

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenSpecialsSupers(RuntimeProbe& runtime, std::ostream& out) {
    return runShotoSpecialsSupers(runtime, out, "EvilKen", "evilken-specials-supers", { 3450 }, 60050, 0, 2211, 900, {
        { "super_first_hit_damage_matches_authored", 3450, -24.0f, 26.0f, 30, 140 },
    });
}

int runEvilKenAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-air-special-contact-landing");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-24.0f, 28.0f);
    runtime.forceFighterState(0, 1862);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -24.0f, -58.0f);
    runtime.setFighterPosition(1, 28.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool sawContact = false;
    bool leftContactState = false;
    FighterSnapshot last = runtime.snapshot().p1;
    std::string hitText;
    for (int i = 0; i < 180; ++i) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        last = snapshot.p1;
        sawContact = sawContact
            || snapshot.p1.moveContact
            || snapshot.p1.moveHit
            || snapshot.p1.moveGuarded
            || snapshot.lastHitText.find("P1 hit") != std::string::npos
            || snapshot.lastHitText.find("P1 guard") != std::string::npos;
        if (!snapshot.lastHitText.empty()) {
            hitText = snapshot.lastHitText;
        }
        if (sawContact && snapshot.p1.stateNo != 1862) {
            leftContactState = true;
            break;
        }
    }

    record(out, counts, sawContact ? Status::Pass : Status::Fail, "air_special_contact_observed",
        "final_state=" + std::to_string(last.stateNo)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_y=" + std::to_string(last.y)
        + " hit=\"" + hitText + "\"");
    record(out, counts, leftContactState ? Status::Pass : Status::Fail, "air_special_exits_contact_landing",
        "final_state=" + std::to_string(last.stateNo)
        + " final_action=" + std::to_string(last.action)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_y=" + std::to_string(last.y));
    record(out, counts, last.stateNo != 1862 || last.stateTime < 90 ? Status::Pass : Status::Fail,
        "air_special_not_stuck_in_1862",
        "state=" + std::to_string(last.stateNo)
        + " time=" + std::to_string(last.stateTime)
        + " y=" + std::to_string(last.y));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenHelperLifecycle(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-helper-lifecycle");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-70.0f, 80.0f);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.spawnHelper(0, 101, 60060, 9999, 9999);
    runtime.step({}, 1);

    int peakHelpers = 0;
    int firstIdleHelperFrame = -1;
    for (int i = 0; i < 120; ++i) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        peakHelpers = std::max(peakHelpers, snapshot.activeHelpers);
        if (snapshot.idleHelpers > 0 && firstIdleHelperFrame < 0) {
            firstIdleHelperFrame = i;
        }
    }
    const auto final = runtime.snapshot();
    record(out, counts, peakHelpers > 0 ? Status::Pass : Status::Fail, "helpers_spawned",
        "peak_helpers=" + std::to_string(peakHelpers));
    record(out, counts, firstIdleHelperFrame < 0 ? Status::Pass : Status::Fail, "helpers_do_not_fall_to_idle_clone",
        "first_idle_helper_frame=" + std::to_string(firstIdleHelperFrame)
        + " final_idle_helpers=" + std::to_string(final.idleHelpers));
    record(out, counts, final.activeHelpers == 0 ? Status::Pass : Status::Fail, "helpers_destroy_after_authored_window",
        "final_helpers=" + std::to_string(final.activeHelpers)
        + " helper_state=" + std::to_string(final.firstHelperState)
        + " helper_action=" + std::to_string(final.firstHelperAction)
        + " helper_anim_tick=" + std::to_string(final.firstHelperAnimTick)
        + " final_p1_state=" + std::to_string(final.p1.stateNo)
        + " final_p1_action=" + std::to_string(final.p1.action));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenPowerChargeHelper(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-power-charge-helper");

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
    bool sawChargeHelper = false;
    bool droppedWhileHeld = false;
    int peakHelpers = 0;
    int idleHelperFrames = 0;
    int chargeLoopFrames = 0;
    int observedHelperState = 0;
    int observedHelperAction = 0;
    FighterSnapshot chargeFighter;
    for (int i = 0; i < 110; ++i) {
        runtime.step(charge, 1);
        const auto snapshot = runtime.snapshot();
        chargeFighter = snapshot.p1;
        sawChargeLoop = sawChargeLoop || snapshot.p1.stateNo == 1051;
        if (snapshot.p1.stateNo == 1051) {
            ++chargeLoopFrames;
        }
        if (i > 35 && snapshot.p1.stateNo != 1051) {
            droppedWhileHeld = true;
        }
        if (snapshot.firstHelperState == 94063 && snapshot.firstHelperAction == 12030) {
            sawChargeHelper = true;
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
        + " loop_frames=" + std::to_string(chargeLoopFrames)
        + " dropped=" + std::to_string(droppedWhileHeld ? 1 : 0));
    record(out, counts, sawChargeHelper && peakHelpers > 0 ? Status::Pass : Status::Fail, "charge_helper_uses_authored_state",
        "peak_helpers=" + std::to_string(peakHelpers)
        + " observed_helper_state=" + std::to_string(observedHelperState)
        + " observed_helper_action=" + std::to_string(observedHelperAction));
    record(out, counts, idleHelperFrames == 0 ? Status::Pass : Status::Fail, "charge_helper_not_idle_clone",
        "idle_helper_frames=" + std::to_string(idleHelperFrames));

    for (int i = 0; i < 100; ++i) {
        runtime.step({}, 1);
    }
    const auto released = runtime.snapshot();
    record(out, counts, released.activeHelpers == 0 ? Status::Pass : Status::Fail, "charge_helper_destroyed_on_release",
        "active_helpers=" + std::to_string(released.activeHelpers)
        + " first_helper_state=" + std::to_string(released.firstHelperState)
        + " p1_state=" + std::to_string(released.p1.stateNo)
        + " p1_action=" + std::to_string(released.p1.action)
        + " p1_time=" + std::to_string(released.p1.stateTime)
        + " commands=" + released.p1Commands);
    record(out, counts, released.p1.stateNo == 0 && released.p1.ctrl ? Status::Pass : Status::Fail, "charge_release_recovers_control",
        "state=" + std::to_string(released.p1.stateNo)
        + " ctrl=" + std::to_string(released.p1.ctrl ? 1 : 0)
        + " time=" + std::to_string(released.p1.stateTime)
        + " commands=" + released.p1Commands);

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}


} // namespace dragon::verification
