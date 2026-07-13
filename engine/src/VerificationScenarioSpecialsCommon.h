#pragma once

#include "VerificationScenario.h"

#include <algorithm>
#include <array>
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

[[maybe_unused]] bool commandCsvContains(std::string_view commands, std::string_view command) {
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

[[maybe_unused]] SymbolicInput buttonsInput(std::initializer_list<char> buttons) {
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

[[maybe_unused]] bool snapshotInStateSet(const FighterSnapshot& fighter, std::initializer_list<int> states) {
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

[[maybe_unused]] InputSequence ffButtonSequence(char button) {
    return ffButtonSequence({ button });
}

[[maybe_unused]] InputSequence doubleQcfSequence(char button) {
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

struct ForcedDamageProbe {
    std::string_view name;
    int stateNo = 0;
    float p1X = -24.0f;
    float p2X = 28.0f;
    int expectedFirstHitDamage = 0;
    int maxFrames = 120;
    int setupVarIndex = -1;
    int setupVarValue = 0;
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

void recordCharacterDataLifeLoaded(RuntimeProbe& runtime, std::ostream& out, Counts& counts, int expectedMaxLife) {
    const auto snapshot = runtime.snapshot();
    const bool loaded = snapshot.p1.maxLife == expectedMaxLife && snapshot.p1.life == expectedMaxLife;
    record(out, counts, loaded ? Status::Pass : Status::Fail, "character_data_life_loaded",
        "life=" + std::to_string(snapshot.p1.life)
        + " max_life=" + std::to_string(snapshot.p1.maxLife)
        + " expected=" + std::to_string(expectedMaxLife));
}

int observeForcedStateFirstDamage(RuntimeProbe& runtime, const ForcedDamageProbe& probe, std::string& hitText) {
    runtime.positionFighters(probe.p1X, probe.p2X);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, probe.p1X, 0.0f);
    runtime.setFighterPosition(1, probe.p2X, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.setFighterLife(1, 1000);
    runtime.setFighterPower(0, 3000);
    runtime.setFighterVar(0, 17, 0);
    if (probe.setupVarIndex >= 0) {
        runtime.setFighterVar(0, probe.setupVarIndex, probe.setupVarValue);
    }
    runtime.forceFighterState(0, probe.stateNo);
    runtime.setFighterControl(0, false);

    int previousLife = runtime.snapshot().p2.life;
    for (int frame = 0; frame < probe.maxFrames; ++frame) {
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        if (snapshot.p2.life < previousLife) {
            hitText = snapshot.lastHitText;
            return previousLife - snapshot.p2.life;
        }
        previousLife = snapshot.p2.life;
    }
    hitText = runtime.snapshot().lastHitText;
    return 0;
}

void recordForcedDamageProbe(RuntimeProbe& runtime, std::ostream& out, Counts& counts, const ForcedDamageProbe& probe) {
    std::string hitText;
    const int damage = observeForcedStateFirstDamage(runtime, probe, hitText);
    const bool passed = damage == probe.expectedFirstHitDamage;
    record(out, counts, passed ? Status::Pass : Status::Fail, probe.name,
        "state=" + std::to_string(probe.stateNo)
        + " damage=" + std::to_string(damage)
        + " expected=" + std::to_string(probe.expectedFirstHitDamage)
        + " hit=\"" + hitText + "\"");
}

[[maybe_unused]] int runShotoSpecialsSupers(
    RuntimeProbe& runtime,
    std::ostream& out,
    std::string_view characterId,
    std::string_view scenarioName,
    std::initializer_list<int> superStates,
    int forcedZeroMoveSuperPauseState = 0,
    int forcedIgnoreHitPauseState = 0,
    int forcedInfiniteAnimExitState = 0,
    int expectedMaxLife = 1000,
    std::vector<ForcedDamageProbe> forcedDamageProbes = {}) {
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
    recordCharacterDataLifeLoaded(runtime, out, counts, expectedMaxLife);

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

    for (const auto& probe : forcedDamageProbes) {
        recordForcedDamageProbe(runtime, out, counts, probe);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace

} // namespace dragon::verification
