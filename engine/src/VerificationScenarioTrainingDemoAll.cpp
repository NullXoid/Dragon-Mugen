#include "VerificationScenarioEvilKenCommon.h"

namespace dragon::verification {

int runTrainingDemoAllForCharacter(
    RuntimeProbe& runtime,
    std::ostream& out,
    std::string_view characterId,
    std::string_view setupLabel,
    std::string_view scenarioName) {
    Counts counts;
    if (!runtime.setup(characterId, "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", std::string(setupLabel) + "/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, scenarioName);

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
            if (!sawTarget && snapshotIndicatesTrainingMoveTarget(move, snap)) {
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

int runEvilKenTrainingDemoAll(RuntimeProbe& runtime, std::ostream& out) {
    return runTrainingDemoAllForCharacter(runtime, out, "EvilKen", "Evil Ken", "evilken-training-demo-all");
}

int runLiliTrainingDemoAll(RuntimeProbe& runtime, std::ostream& out) {
    return runTrainingDemoAllForCharacter(runtime, out, "Lili", "Lili", "lili-training-demo-all");
}


} // namespace dragon::verification
