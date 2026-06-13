#include "VerificationScenario.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dragon::verification {
namespace {

enum class Status {
    Pass,
    Fail,
    Blocked,
};

struct Counts {
    int pass = 0;
    int fail = 0;
    int blocked = 0;
};

const char* statusText(Status status) {
    switch (status) {
    case Status::Pass:
        return "PASS";
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
    out << "SUMMARY pass=" << counts.pass << " partial=0 fail=" << counts.fail
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

bool commandCsvContains(std::string_view commands, std::string_view command) {
    size_t start = 0;
    while (start <= commands.size()) {
        size_t end = commands.find(',', start);
        if (end == std::string_view::npos) {
            end = commands.size();
        }
        if (commands.substr(start, end - start) == command) {
            return true;
        }
        if (end == commands.size()) {
            break;
        }
        start = end + 1;
    }
    return false;
}

SymbolicInput directionInput(std::string_view direction) {
    SymbolicInput input;
    if (direction == "F") {
        input.right = true;
    } else if (direction == "B") {
        input.left = true;
    } else if (direction == "D") {
        input.down = true;
    } else if (direction == "DF") {
        input.down = true;
        input.right = true;
    } else if (direction == "DB") {
        input.down = true;
        input.left = true;
    }
    return input;
}

void setInputButton(SymbolicInput& input, char button) {
    if (button == 'x') {
        input.x = true;
    } else if (button == 'y') {
        input.y = true;
    } else if (button == 'z') {
        input.z = true;
    } else if (button == 'a') {
        input.a = true;
    } else if (button == 'b') {
        input.b = true;
    } else if (button == 'c') {
        input.c = true;
    }
}

SymbolicInput buttonsInput(std::initializer_list<char> buttons) {
    SymbolicInput input;
    for (const char button : buttons) {
        setInputButton(input, button);
    }
    return input;
}

SymbolicInput directionButtonInput(std::string_view direction, char button) {
    SymbolicInput input = directionInput(direction);
    setInputButton(input, button);
    return input;
}

SymbolicInput directionButtonsInput(std::string_view direction, std::initializer_list<char> buttons) {
    SymbolicInput input = directionInput(direction);
    for (const char button : buttons) {
        setInputButton(input, button);
    }
    return input;
}

bool snapshotInStateSet(const FighterSnapshot& fighter, std::initializer_list<int> states) {
    return std::find(states.begin(), states.end(), fighter.stateNo) != states.end();
}

bool snapshotInStateSet(const FighterSnapshot& fighter, const std::vector<int>& states) {
    return std::find(states.begin(), states.end(), fighter.stateNo) != states.end();
}

using InputSequence = std::vector<std::pair<SymbolicInput, int>>;

bool stepSequenceAndObserve(
    RuntimeProbe& runtime,
    const InputSequence& sequence,
    const std::vector<int>& states,
    FighterSnapshot& observed,
    std::string* observedCommands = nullptr,
    int neutralFrames = 30) {
    std::string lastCommands;
    for (const auto& [input, frames] : sequence) {
        for (int i = 0; i < frames; ++i) {
            runtime.step(input, 1);
            const auto snapshot = runtime.snapshot();
            const auto fighter = snapshot.p1;
            if (!snapshot.p1Commands.empty()) {
                lastCommands = snapshot.p1Commands;
            }
            if (snapshotInStateSet(fighter, states)) {
                observed = fighter;
                if (observedCommands) {
                    *observedCommands = snapshot.p1Commands.empty() ? lastCommands : snapshot.p1Commands;
                }
                return true;
            }
        }
    }
    for (int i = 0; i < neutralFrames; ++i) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        const auto fighter = snapshot.p1;
        if (!snapshot.p1Commands.empty()) {
            lastCommands = snapshot.p1Commands;
        }
        if (snapshotInStateSet(fighter, states)) {
            observed = fighter;
            if (observedCommands) {
                *observedCommands = snapshot.p1Commands.empty() ? lastCommands : snapshot.p1Commands;
            }
            return true;
        }
    }
    observed = runtime.snapshot().p1;
    if (observedCommands) {
        *observedCommands = lastCommands;
    }
    return false;
}

bool stepSequenceAndObserve(
    RuntimeProbe& runtime,
    const InputSequence& sequence,
    std::initializer_list<int> states,
    FighterSnapshot& observed,
    std::string* observedCommands = nullptr,
    int neutralFrames = 30) {
    return stepSequenceAndObserve(runtime, sequence, std::vector<int>(states), observed, observedCommands, neutralFrames);
}

InputSequence qcfSequence(std::initializer_list<char> buttons) {
    return {
        { {}, 3 },
        { directionInput("D"), 2 },
        { directionInput("DF"), 2 },
        { directionButtonsInput("F", buttons), 3 },
    };
}

InputSequence qcfSequence(char button) {
    return qcfSequence({ button });
}

InputSequence qcbSequence(std::initializer_list<char> buttons) {
    return {
        { {}, 3 },
        { directionInput("D"), 2 },
        { directionInput("DB"), 2 },
        { directionButtonsInput("B", buttons), 3 },
    };
}

InputSequence qcbSequence(char button) {
    return qcbSequence({ button });
}

InputSequence hcfSequence(std::initializer_list<char> buttons) {
    return {
        { {}, 3 },
        { directionInput("B"), 2 },
        { directionInput("DB"), 2 },
        { directionInput("D"), 2 },
        { directionInput("DF"), 2 },
        { directionButtonsInput("F", buttons), 3 },
    };
}

InputSequence hcfSequence(char button) {
    return hcfSequence({ button });
}

InputSequence dpSequence(std::initializer_list<char> buttons) {
    return {
        { {}, 3 },
        { directionInput("F"), 2 },
        { directionInput("D"), 2 },
        { directionButtonsInput("DF", buttons), 3 },
    };
}

InputSequence dpSequence(char button) {
    return dpSequence({ button });
}

InputSequence ffButtonSequence(std::initializer_list<char> buttons) {
    return {
        { {}, 3 },
        { directionInput("F"), 2 },
        { {}, 1 },
        { directionButtonsInput("F", buttons), 3 },
    };
}

InputSequence ffButtonSequence(char button) {
    return ffButtonSequence({ button });
}

InputSequence doubleQcfSequence(char button) {
    return {
        { {}, 3 },
        { directionInput("D"), 2 },
        { directionInput("DF"), 2 },
        { directionInput("F"), 2 },
        { {}, 1 },
        { directionInput("D"), 2 },
        { directionInput("DF"), 2 },
        { directionButtonInput("F", button), 3 },
    };
}

InputSequence simpleDoubleQcfSequence(char button) {
    return {
        { {}, 3 },
        { directionInput("D"), 2 },
        { directionInput("F"), 2 },
        { {}, 1 },
        { directionInput("D"), 2 },
        { directionButtonInput("F", button), 3 },
    };
}

bool resetSpecialProbe(RuntimeProbe& runtime, int power = 0) {
    if (!waitForControllableIdle(runtime, 300)) {
        return false;
    }
    runtime.positionFighters(-60.0f, 74.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.setFighterPower(0, power);
    runtime.step({}, 6);
    return waitForControllableIdle(runtime, 120);
}

struct SpecialProbeCase {
    std::string_view name;
    InputSequence sequence;
    std::vector<int> states;
    int power = 0;
};

void recordObservedState(
    std::ostream& out,
    Counts& counts,
    bool passed,
    std::string_view name,
    bool ready,
    const FighterSnapshot& observed,
    std::string_view commands = {}) {
    record(out, counts, passed ? Status::Pass : Status::Fail, name,
        "ready=" + std::to_string(ready ? 1 : 0)
        + " state=" + std::to_string(observed.stateNo)
        + " action=" + std::to_string(observed.action)
        + " move_type=" + std::string(1, observed.moveType)
        + " power=" + std::to_string(observed.power)
        + " commands=" + std::string(commands));
}

bool runSpecialProbeCase(RuntimeProbe& runtime, std::ostream& out, Counts& counts, const SpecialProbeCase& probe) {
    FighterSnapshot observed;
    std::string commands;
    const bool ready = resetSpecialProbe(runtime, probe.power);
    const bool entered = ready
        && stepSequenceAndObserve(runtime, probe.sequence, probe.states, observed, &commands, 45);
    recordObservedState(out, counts, entered, probe.name, ready, observed, commands);
    return entered;
}

bool observePowerConsumed(RuntimeProbe& runtime, int powerBefore, int& powerAfter) {
    powerAfter = runtime.snapshot().p1.power;
    for (int i = 0; i < 80 && powerAfter >= powerBefore; ++i) {
        runtime.step({}, 1);
        powerAfter = runtime.snapshot().p1.power;
    }
    return powerBefore >= 1000 && powerAfter < powerBefore;
}

int runShotoSpecialsSupers(
    RuntimeProbe& runtime,
    std::ostream& out,
    std::string_view characterId,
    std::string_view scenarioName,
    std::initializer_list<int> superStates,
    int forcedZeroMoveSuperPauseState = 0,
    int forcedIgnoreHitPauseState = 0,
    int forcedInfiniteAnimExitState = 0) {
    Counts counts;
    if (!runtime.setup(characterId, "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Training setup failed");
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

    if (forcedZeroMoveSuperPauseState > 0) {
        runtime.positionFighters(-60.0f, 74.0f);
        runtime.forceFighterState(0, forcedZeroMoveSuperPauseState);
        runtime.setFighterControl(0, false);
        const auto forced = runtime.snapshot();
        for (int i = 0; i < 90; ++i) {
            runtime.step({}, 1);
        }
        const auto afterPause = runtime.snapshot();
        const bool enteredForcedState = forced.p1.stateNo == forcedZeroMoveSuperPauseState;
        const bool advancedPastPause = enteredForcedState
            && (afterPause.p1.stateNo != forced.p1.stateNo || afterPause.p1.stateTime > forced.p1.stateTime + 4);
        record(out, counts, advancedPastPause ? Status::Pass : Status::Fail, "zero_movetime_superpause_advances",
            "entered=" + std::to_string(enteredForcedState ? 1 : 0)
            + " forced_state=" + std::to_string(forced.p1.stateNo)
            + " forced_time=" + std::to_string(forced.p1.stateTime)
            + " after_state=" + std::to_string(afterPause.p1.stateNo)
            + " after_time=" + std::to_string(afterPause.p1.stateTime)
            + " global_pause=" + std::to_string(afterPause.globalPauseTicks)
            + " super=" + std::to_string(afterPause.globalPauseIsSuper ? 1 : 0));

        runtime.positionFighters(-60.0f, 74.0f);
        runtime.forceFighterState(0, forcedZeroMoveSuperPauseState);
        runtime.setFighterControl(0, false);
        runtime.setFighterHitPause(0, 12);
        runtime.step({}, 4);
        const auto duringHitPause = runtime.snapshot();
        runtime.step({}, 90);
        const auto afterHitPause = runtime.snapshot();
        const bool skippedDuringHitPause = duringHitPause.p1.hitPauseTicks > 0
            && duringHitPause.p1.stateNo == forcedZeroMoveSuperPauseState
            && duringHitPause.p1.stateTime == 0
            && duringHitPause.globalPauseTicks == 0;
        const bool resumedAfterHitPause = afterHitPause.p1.hitPauseTicks == 0
            && (afterHitPause.p1.stateNo != forcedZeroMoveSuperPauseState || afterHitPause.p1.stateTime > 4);
        record(out, counts, skippedDuringHitPause ? Status::Pass : Status::Fail, "hitpause_skips_non_ignore_superpause",
            "during_state=" + std::to_string(duringHitPause.p1.stateNo)
            + " during_time=" + std::to_string(duringHitPause.p1.stateTime)
            + " during_hitpause=" + std::to_string(duringHitPause.p1.hitPauseTicks)
            + " global_pause=" + std::to_string(duringHitPause.globalPauseTicks));
        record(out, counts, resumedAfterHitPause ? Status::Pass : Status::Fail, "hitpause_then_state_resumes",
            "after_state=" + std::to_string(afterHitPause.p1.stateNo)
            + " after_time=" + std::to_string(afterHitPause.p1.stateTime)
            + " after_hitpause=" + std::to_string(afterHitPause.p1.hitPauseTicks)
            + " global_pause=" + std::to_string(afterHitPause.globalPauseTicks));
    }

    if (forcedIgnoreHitPauseState > 0) {
        runtime.positionFighters(-60.0f, 74.0f);
        runtime.forceFighterState(0, forcedIgnoreHitPauseState);
        runtime.setFighterControl(0, false);
        runtime.setFighterHitPause(0, 10);
        const auto before = runtime.snapshot();
        runtime.step({}, 1);
        const auto firstFrozenFrame = runtime.snapshot();
        runtime.step({}, 4);
        const auto laterFrozenFrame = runtime.snapshot();
        const int firstSpawnedEffects = firstFrozenFrame.activeEffects - before.activeEffects;
        const int laterSpawnedEffects = laterFrozenFrame.activeEffects - before.activeEffects;
        const bool ignoreRan = firstFrozenFrame.p1.hitPauseTicks > 0 && firstSpawnedEffects == 1;
        const bool oneShotDidNotClone = laterFrozenFrame.p1.hitPauseTicks > 0 && laterSpawnedEffects == 1;
        record(out, counts, ignoreRan ? Status::Pass : Status::Fail, "ignorehitpause_runs_during_hitpause",
            "before_effects=" + std::to_string(before.activeEffects)
            + " first_effects=" + std::to_string(firstFrozenFrame.activeEffects)
            + " hitpause=" + std::to_string(firstFrozenFrame.p1.hitPauseTicks));
        record(out, counts, oneShotDidNotClone ? Status::Pass : Status::Fail, "frozen_time_zero_spawner_does_not_clone",
            "before_effects=" + std::to_string(before.activeEffects)
            + " later_effects=" + std::to_string(laterFrozenFrame.activeEffects)
            + " hitpause=" + std::to_string(laterFrozenFrame.p1.hitPauseTicks));
    }

    if (forcedInfiniteAnimExitState > 0) {
        runtime.positionFighters(-60.0f, 74.0f);
        runtime.forceFighterState(0, forcedInfiniteAnimExitState);
        runtime.setFighterControl(0, false);
        const auto forced = runtime.snapshot();
        runtime.step({}, 80);
        const auto afterInfiniteFrame = runtime.snapshot();
        const bool exitedForcedState = forced.p1.stateNo == forcedInfiniteAnimExitState
            && afterInfiniteFrame.p1.stateNo != forcedInfiniteAnimExitState;
        record(out, counts, exitedForcedState ? Status::Pass : Status::Fail, "infinite_air_anim_exits_by_animelem_time",
            "forced_state=" + std::to_string(forced.p1.stateNo)
            + " forced_action=" + std::to_string(forced.p1.action)
            + " forced_anim_tick=" + std::to_string(forced.p1.animTick)
            + " after_state=" + std::to_string(afterInfiniteFrame.p1.stateNo)
            + " after_action=" + std::to_string(afterInfiniteFrame.p1.action)
            + " after_anim_tick=" + std::to_string(afterInfiniteFrame.p1.animTick));
    }

    const std::vector<SpecialProbeCase> groundSpecials = characterId == "EvilRyu"
        ? std::vector<SpecialProbeCase>{
            { "hadouken_lp", qcfSequence('x'), { 1000 }, 0 },
            { "hadouken_mp", qcfSequence('y'), { 1001 }, 0 },
            { "hadouken_hp", qcfSequence('z'), { 1002 }, 0 },
            { "charged_hadouken_lp", hcfSequence('x'), { 10000 }, 0 },
            { "charged_hadouken_mp", hcfSequence('y'), { 10001 }, 0 },
            { "charged_hadouken_hp", hcfSequence('z'), { 10002 }, 0 },
            { "shoryuken_lp", dpSequence('x'), { 1500 }, 0 },
            { "shoryuken_mp", dpSequence('y'), { 1600 }, 0 },
            { "shoryuken_hp", dpSequence('z'), { 1700 }, 0 },
            { "tatsumaki_lk", qcbSequence('a'), { 1800 }, 0 },
            { "tatsumaki_mk", qcbSequence('b'), { 1810 }, 0 },
            { "tatsumaki_hk", qcbSequence('c'), { 1820 }, 0 },
            { "joudan_lk", qcfSequence('a'), { 2800 }, 0 },
            { "joudan_mk", qcfSequence('b'), { 2810 }, 0 },
            { "joudan_hk", qcfSequence('c'), { 2820 }, 0 },
            { "zenpu_lp", qcbSequence('x'), { 1200 }, 0 },
            { "zenpu_mp", qcbSequence('y'), { 1210 }, 0 },
            { "zenpu_hp", qcbSequence('z'), { 1220 }, 0 },
        }
        : std::vector<SpecialProbeCase>{
            { "hadouken_lp", qcfSequence('x'), { 1000 }, 0 },
            { "hadouken_mp", qcfSequence('y'), { 1001 }, 0 },
            { "hadouken_hp", qcfSequence('z'), { 1002 }, 0 },
            { "shoryuken_lp", dpSequence('x'), { 1500 }, 0 },
            { "shoryuken_mp", dpSequence('y'), { 1600 }, 0 },
            { "shoryuken_hp", dpSequence('z'), { 1700 }, 0 },
            { "fujin_lk", dpSequence('a'), { 270 }, 0 },
            { "fujin_mk", dpSequence('b'), { 275 }, 0 },
            { "fujin_hk", dpSequence('c'), { 280 }, 0 },
            { "tatsumaki_lk", qcbSequence('a'), { 1800 }, 0 },
            { "tatsumaki_mk", qcbSequence('b'), { 1810 }, 0 },
            { "tatsumaki_hk", qcbSequence('c'), { 1820 }, 0 },
            { "tenshou_tatsumaki_lk", qcfSequence('a'), { 1830 }, 0 },
            { "tenshou_tatsumaki_mk", qcfSequence('b'), { 1835 }, 0 },
            { "tenshou_tatsumaki_hk", qcfSequence('c'), { 1840 }, 0 },
            { "zenpu_lp", qcbSequence('x'), { 1200 }, 0 },
            { "zenpu_mp", qcbSequence('y'), { 1210 }, 0 },
            { "zenpu_hp", qcbSequence('z'), { 1220 }, 0 },
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
        && stepSequenceAndObserve(runtime, simpleDoubleQcfSequence('x'), superStates, blockedObserved, &blockedCommands, 45);
    record(out, counts, blockedReady && !blockedSuper ? Status::Pass : Status::Fail, "double_qcf_super_blocked_without_power",
        "ready=" + std::to_string(blockedReady ? 1 : 0)
        + " state=" + std::to_string(blockedObserved.stateNo)
        + " action=" + std::to_string(blockedObserved.action)
        + " power=" + std::to_string(blockedObserved.power)
        + " commands=" + blockedCommands);

    const bool superReady = resetSpecialProbe(runtime, 1000);
    const int powerBefore = runtime.snapshot().p1.power;
    const bool super = superReady && stepSequenceAndObserve(runtime, simpleDoubleQcfSequence('x'), superStates, observed, &commands, 45);
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

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace

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

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenSpecialsSupers(RuntimeProbe& runtime, std::ostream& out) {
    return runShotoSpecialsSupers(runtime, out, "EvilKen", "evilken-specials-supers", { 3450 }, 60050, 0, 2211);
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

int runEvilRyuSpecialsSupers(RuntimeProbe& runtime, std::ostream& out) {
    return runShotoSpecialsSupers(runtime, out, "EvilRyu", "evilryu-specials-supers", { 3885 }, 0, 11164);
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
