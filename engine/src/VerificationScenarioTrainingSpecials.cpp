#include "VerificationScenarioSpecialsCommon.h"

namespace dragon::verification {

int runEvilKenTrainingCommandDemo(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-training-demo-hit");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-260.0f, 260.0f);
    const bool selectedRoll = runtime.selectTrainingMove("Ground Recovery Roll");
    record(out, counts, selectedRoll ? Status::Pass : Status::Blocked, "select_recovery_roll_demo_move", "move=Ground Recovery Roll");
    if (!selectedRoll) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.startTrainingCommandDemo();
    const auto rollStart = runtime.snapshot();
    const bool rollPrereq =
        (rollStart.p2.stateNo == 5050 || rollStart.p2.stateNo == 5071)
        && rollStart.p2.moveType == 'H'
        && !rollStart.p2.onGround
        && rollStart.p2.vy > 0.0f
        && rollStart.p2.y >= -48.0f;
    record(out, counts, rollPrereq ? Status::Pass : Status::Fail, "demo_sets_recovery_roll_prereq",
        "p2_state=" + std::to_string(rollStart.p2.stateNo)
        + " p2_action=" + std::to_string(rollStart.p2.action)
        + " p2_move_type=" + std::string(1, rollStart.p2.moveType)
        + " p2_y=" + std::to_string(rollStart.p2.y)
        + " p2_vy=" + std::to_string(rollStart.p2.vy));

    bool sawRollCommand = commandCsvContains(rollStart.p2Commands, "BQCD_x");
    bool p2EnteredRoll = rollStart.p2.stateNo >= 2004 && rollStart.p2.stateNo <= 2006;
    bool p2UsedRollSprite = false;
    FighterSnapshot rollP2 = rollStart.p2;
    std::string observedRollCommands = rollStart.p2Commands;
    for (int i = 0; i < 120; ++i) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        if (!snapshot.p2Commands.empty()) {
            observedRollCommands = snapshot.p2Commands;
        }
        const bool commandActive = commandCsvContains(snapshot.p2Commands, "BQCD_x");
        if (commandActive) {
            observedRollCommands = snapshot.p2Commands;
            rollP2 = snapshot.p2;
        }
        sawRollCommand = sawRollCommand || commandActive;
        const bool inAirRoll = snapshot.p2.stateNo >= 2004 && snapshot.p2.stateNo <= 2006;
        const bool inGroundRoll = snapshot.p2.stateNo >= 2001 && snapshot.p2.stateNo <= 2003;
        p2EnteredRoll = p2EnteredRoll || inAirRoll || inGroundRoll;
        const bool rollAction =
            (inAirRoll && snapshot.p2.action >= 5050 && snapshot.p2.action <= 5069)
            || (inGroundRoll && snapshot.p2.action == 2001);
        if (inAirRoll || inGroundRoll || rollAction) {
            rollP2 = snapshot.p2;
        }
        if (rollAction) {
            p2UsedRollSprite = true;
            break;
        }
    }

    record(out, counts, sawRollCommand ? Status::Pass : Status::Fail, "demo_p2_buffers_recovery_roll_command",
        "commands=" + observedRollCommands
        + " p2_state=" + std::to_string(rollP2.stateNo)
        + " p2_action=" + std::to_string(rollP2.action));
    record(out, counts, p2EnteredRoll ? Status::Pass : Status::Fail, "demo_p2_enters_recovery_roll_state",
        "commands=" + observedRollCommands
        + " p2_state=" + std::to_string(rollP2.stateNo)
        + " p2_action=" + std::to_string(rollP2.action)
        + " p2_time=" + std::to_string(rollP2.stateTime));
    record(out, counts, p2UsedRollSprite ? Status::Pass : Status::Fail, "demo_p2_uses_recovery_roll_sprite",
        "commands=" + observedRollCommands
        + " p2_state=" + std::to_string(rollP2.stateNo)
        + " p2_action=" + std::to_string(rollP2.action)
        + " p2_move_type=" + std::string(1, rollP2.moveType));

    const bool selected = runtime.selectTrainingMove("S.Jab");
    record(out, counts, selected ? Status::Pass : Status::Blocked, "select_demo_move", "move=S.Jab");
    if (!selected) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.startTrainingCommandDemo();
    const auto start = runtime.snapshot();
    const float demoDistance = std::fabs(start.p2.x - start.p1.x);
    record(out, counts, demoDistance < 90.0f ? Status::Pass : Status::Fail, "demo_repositions_target",
        "p1_x=" + std::to_string(start.p1.x)
        + " p2_x=" + std::to_string(start.p2.x)
        + " distance=" + std::to_string(demoDistance));

    SymbolicInput retreat;
    retreat.left = true;
    bool p1Hit = false;
    FighterSnapshot hitSnapshot = start.p1;
    FighterSnapshot lastP2 = start.p2;
    bool p2EnteredMove = start.p2.stateNo != 0;
    std::string hitText;
    for (int i = 0; i < 120; ++i) {
        runtime.step(retreat, 1);
        const auto snapshot = runtime.snapshot();
        lastP2 = snapshot.p2;
        p2EnteredMove = p2EnteredMove || snapshot.p2.stateNo == 206;
        if (snapshot.p1.life < start.p1.life || snapshot.p1.moveType == 'H') {
            p1Hit = true;
            hitSnapshot = snapshot.p1;
            hitText = snapshot.lastHitText;
            break;
        }
    }

    record(out, counts, p1Hit ? Status::Pass : Status::Fail, "demo_hits_p1_player_target",
        "start_life=" + std::to_string(start.p1.life)
        + " hit_life=" + std::to_string(hitSnapshot.life)
        + " hit_state=" + std::to_string(hitSnapshot.stateNo)
        + " hit_move_type=" + std::string(1, hitSnapshot.moveType)
        + " p2_entered_206=" + std::to_string(p2EnteredMove ? 1 : 0)
        + " p2_state=" + std::to_string(lastP2.stateNo)
        + " p2_action=" + std::to_string(lastP2.action)
        + " p2_time=" + std::to_string(lastP2.stateTime)
        + " text=\"" + hitText + "\"");

    const bool selectedHadouken = runtime.selectTrainingMove("Hadouken");
    record(out, counts, selectedHadouken ? Status::Pass : Status::Blocked, "select_directional_demo_move", "move=Hadouken");
    if (!selectedHadouken) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.startTrainingCommandDemo();
    const auto directionalStart = runtime.snapshot();
    bool p2EnteredHadouken = false;
    bool p2FacedP1 = directionalStart.p2.x > directionalStart.p1.x;
    FighterSnapshot hadoukenP2 = directionalStart.p2;
    for (int i = 0; i < 150; ++i) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        p2FacedP1 = p2FacedP1 && snapshot.p2.x > snapshot.p1.x;
        if (snapshot.p2.stateNo == 1000 || snapshot.p2.stateNo == 1001 || snapshot.p2.stateNo == 1002) {
            p2EnteredHadouken = true;
            hadoukenP2 = snapshot.p2;
            break;
        }
    }

    record(out, counts, p2EnteredHadouken ? Status::Pass : Status::Fail, "demo_p2_enters_directional_special",
        "p2_right_of_p1=" + std::to_string(p2FacedP1 ? 1 : 0)
        + " p2_state=" + std::to_string(hadoukenP2.stateNo)
        + " p2_action=" + std::to_string(hadoukenP2.action)
        + " p2_time=" + std::to_string(hadoukenP2.stateTime));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilRyuTrainingThrowDemo(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-training-throw-demo");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-260.0f, 260.0f);
    const bool selectedThrow = runtime.selectTrainingMove("Stand Kick Throw");
    const auto selected = runtime.snapshot();
    record(out, counts, selectedThrow ? Status::Pass : Status::Blocked, "select_throw_demo_move",
        "requested=Stand Kick Throw selected=\"" + selected.selectedTrainingMoveLabel + "\"");
    if (!selectedThrow) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.startTrainingCommandDemo();
    const auto throwStart = runtime.snapshot();
    const float throwStartDistance = std::fabs(throwStart.p2.x - throwStart.p1.x);
    record(out, counts, throwStartDistance <= 12.0f ? Status::Pass : Status::Fail, "demo_throw_repositions_at_grab_range",
        "p1_x=" + std::to_string(throwStart.p1.x)
        + " p2_x=" + std::to_string(throwStart.p2.x)
        + " distance=" + std::to_string(throwStartDistance));

    bool sawThrowHit = false;
    bool sawThrowAttemptState = throwStart.p2.stateNo == 905;
    bool sawThrowFollowup = false;
    bool sawVictimCustomState = false;
    FighterSnapshot throwP1 = throwStart.p1;
    FighterSnapshot throwP2 = throwStart.p2;
    std::string throwHitText = throwStart.lastHitText;
    for (int i = 0; i < 180; ++i) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        throwP1 = snapshot.p1;
        throwP2 = snapshot.p2;
        if (!snapshot.lastHitText.empty()) {
            throwHitText = snapshot.lastHitText;
        }
        sawThrowAttemptState = sawThrowAttemptState || snapshot.p2.stateNo == 905;
        sawThrowHit = sawThrowHit || snapshot.lastHitText.find("P2 hit 905#") != std::string::npos;
        sawThrowFollowup = sawThrowFollowup || snapshot.p2.stateNo == 930;
        sawVictimCustomState = sawVictimCustomState || snapshot.p1.stateNo == 935;
        if (sawThrowHit && sawThrowFollowup && sawVictimCustomState) {
            break;
        }
    }

    record(out, counts, sawThrowHit ? Status::Pass : Status::Fail, "demo_throw_hitdef_connected",
        "attempt_state_seen=" + std::to_string(sawThrowAttemptState ? 1 : 0)
        + " p1_state=" + std::to_string(throwP1.stateNo)
        + " p2_state=" + std::to_string(throwP2.stateNo)
        + " text=\"" + throwHitText + "\"");
    record(out, counts, sawThrowFollowup && sawVictimCustomState ? Status::Pass : Status::Fail, "demo_throw_custom_states_entered",
        "p1_state=" + std::to_string(throwP1.stateNo)
        + " p1_action=" + std::to_string(throwP1.action)
        + " p2_state=" + std::to_string(throwP2.stateNo)
        + " p2_action=" + std::to_string(throwP2.action));
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenTrainingCommandPracticeAdvance(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-training-command-practice-advance");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-260.0f, 260.0f);
    const bool selected = runtime.selectTrainingMove("S.Jab");
    const auto selectedStart = runtime.snapshot();
    record(out, counts, selected ? Status::Pass : Status::Blocked, "select_practice_move",
        "requested=S.Jab selected=\"" + selectedStart.selectedTrainingMoveLabel + "\"");
    if (!selected) {
        summary(out, counts);
        return exitCode(counts);
    }

    constexpr std::array<char, 6> buttons{ 'x', 'y', 'z', 'a', 'b', 'c' };
    bool sawOk = false;
    bool selectedAdvanced = false;
    char workingButton = '?';
    std::string okText;
    std::string selectedAfter;
    FighterSnapshot observedP1;

    for (const char button : buttons) {
        waitForControllableIdle(runtime, 180);
        runtime.step({}, 6);
        runtime.step(buttonsInput({ button }), 2);
        for (int i = 0; i < 90; ++i) {
            const auto snapshot = runtime.snapshot();
            if (!snapshot.lastHitText.empty() && snapshot.lastHitText.rfind("OK:", 0) == 0) {
                sawOk = true;
                workingButton = button;
                okText = snapshot.lastHitText;
                selectedAfter = snapshot.selectedTrainingMoveLabel;
                selectedAdvanced = !selectedAfter.empty() && selectedAfter != "S.Jab";
                observedP1 = snapshot.p1;
                break;
            }
            runtime.step({}, 1);
        }
        if (sawOk) {
            break;
        }
    }

    record(out, counts, sawOk ? Status::Pass : Status::Fail, "practice_ok_notification",
        "button=" + std::string(1, workingButton)
        + " text=\"" + okText + "\""
        + " p1_state=" + std::to_string(observedP1.stateNo)
        + " p1_action=" + std::to_string(observedP1.action));
    record(out, counts, selectedAdvanced ? Status::Pass : Status::Fail, "practice_auto_advances_move",
        "before=S.Jab after=\"" + selectedAfter + "\" text=\"" + okText + "\"");
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}


} // namespace dragon::verification
