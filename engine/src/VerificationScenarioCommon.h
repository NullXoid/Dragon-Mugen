#pragma once

#include "VerificationScenario.h"

#include "dragon/Compatibility.h"
#include "dragon/MugenData.h"

#include "AppTypes.h"
#include "ControlsOptionsMenu.h"
#include "ControlsStore.h"
#include "Input.h"
#include "TrainingCommandInputRenderer.h"
#include "TrainingOptionsBehavior.h"
#include "TrainingOptionsOverlay.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace dragon::verification {
namespace {

enum class Status { Pass, Partial, Fail, Blocked };

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

std::string lowercaseAsciiCopy(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

const CharacterSlot* findCharacterById(const std::vector<CharacterSlot>& characters, std::string_view id) {
    const auto it = std::find_if(characters.begin(), characters.end(), [id](const CharacterSlot& character) {
        return character.id == id;
    });
    return it == characters.end() ? nullptr : &*it;
}

const StageSlot* findLegacyOpenBorStage(const std::vector<StageSlot>& stages) {
    const auto it = std::find_if(stages.begin(), stages.end(), [](const StageSlot& stage) {
        return stage.legacyOpenBorSection && stage.openborScrolling;
    });
    return it == stages.end() ? nullptr : &*it;
}

void header(std::ostream& out, RuntimeProbe& runtime, std::string_view scenario) {
    out << "VERIFY " << scenario << "\n" << "root: " << runtime.rootText() << "\n"
        << "stage: " << runtime.stageName() << "\n" << "p1: " << runtime.p1Name() << "\n";
}
SymbolicInput withButton(char button) {
    SymbolicInput input;
    if (button == 'x') input.x = true; if (button == 'y') input.y = true; if (button == 'z') input.z = true;
    if (button == 'a') input.a = true; if (button == 'b') input.b = true; if (button == 'c') input.c = true;
    return input;
}

SymbolicInput withDirection(std::string_view direction) {
    SymbolicInput input;
    if (direction == "D") {
        input.down = true;
    } else if (direction == "DF") {
        input.down = true;
        input.right = true;
    } else if (direction == "F") {
        input.right = true;
    } else if (direction == "DB") {
        input.down = true;
        input.left = true;
    } else if (direction == "B") {
        input.left = true;
    } else if (direction == "U") {
        input.up = true;
    }
    return input;
}

SymbolicInput withDirectionAndButton(std::string_view direction, char button) {
    SymbolicInput input = withDirection(direction);
    const SymbolicInput buttonInput = withButton(button);
    input.x = input.x || buttonInput.x;
    input.y = input.y || buttonInput.y;
    input.z = input.z || buttonInput.z;
    input.a = input.a || buttonInput.a;
    input.b = input.b || buttonInput.b;
    input.c = input.c || buttonInput.c;
    return input;
}

void performQcfButton(RuntimeProbe& runtime, char button) {
    runtime.step({}, 3);
    runtime.step(withDirection("D"), 2);
    runtime.step(withDirection("DF"), 2);
    runtime.step(withDirectionAndButton("F", button), 3);
}

void performQcbButton(RuntimeProbe& runtime, char button) {
    runtime.step({}, 3);
    runtime.step(withDirection("D"), 2);
    runtime.step(withDirection("DB"), 2);
    runtime.step(withDirectionAndButton("B", button), 3);
}

void performDpButton(RuntimeProbe& runtime, char button) {
    runtime.step({}, 3);
    runtime.step(withDirection("F"), 2);
    runtime.step(withDirection("D"), 2);
    runtime.step(withDirectionAndButton("DF", button), 3);
}

bool changedStateOrAction(const FighterSnapshot& before, const FighterSnapshot& after) { return before.stateNo != after.stateNo || before.action != after.action; }

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

bool waitForActiveFight(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight)) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight);
}

float horizontalDistance(const RuntimeSnapshot& snapshot) {
    return std::fabs(snapshot.p2.x - snapshot.p1.x);
}

std::string stateActionDetail(const FighterSnapshot& before, const FighterSnapshot& after, char command) {
    return "command=" + std::string(1, command) + " state_before=" + std::to_string(before.stateNo)
        + " state_after=" + std::to_string(after.stateNo)
        + " anim_before=" + std::to_string(before.action)
        + " anim_after=" + std::to_string(after.action);
}

bool tryNormal(RuntimeProbe& runtime, char& usedCommand, FighterSnapshot& before, FighterSnapshot& after, bool crouch) {
    constexpr std::array<char, 6> buttons{ 'x', 'y', 'z', 'a', 'b', 'c' };
    for (const char button : buttons) {
        runtime.step({}, 30);
        if (crouch) {
            runtime.step(SymbolicInput{ .down = true }, 30);
        }
        SymbolicInput input = withButton(button);
        if (crouch) {
            input.down = true;
        }
        before = runtime.snapshot().p1;
        runtime.step(input, 2);
        const auto immediate = runtime.snapshot().p1;
        if (changedStateOrAction(before, immediate) && immediate.moveType == 'A') {
            after = immediate;
            usedCommand = button;
            return true;
        }
        runtime.step(crouch ? SymbolicInput{ .down = true } : SymbolicInput{}, 12);
        after = runtime.snapshot().p1;
        if (changedStateOrAction(before, after) && after.moveType == 'A') {
            usedCommand = button;
            return true;
        }
    }
    return false;
}

struct TauntCtrlSetObservation {
    bool startedFromIdle = false;
    bool sawState195 = false;
    bool sawCtrlFalseInTaunt = false;
    bool attemptedCommandWhileCtrlFalse = false;
    bool commandBlockedWhileCtrlFalse = false;
    bool sawCtrlSetRestoreInTaunt = false;
    bool returnedToIdle = false;
    bool commandWorksAfterRestore = false;
    char restoreCommand = '?';
    FighterSnapshot ctrlFalse;
    FighterSnapshot blockedAttempt;
    FighterSnapshot ctrlRestore;
    FighterSnapshot final;
    FighterSnapshot commandAfterRestore;
};

TauntCtrlSetObservation observeTauntCtrlSetControlRestore(RuntimeProbe& runtime, char restoreCommand) {
    TauntCtrlSetObservation observation;
    observation.restoreCommand = restoreCommand;
    observation.startedFromIdle = waitForControllableIdle(runtime, 360);
    if (!observation.startedFromIdle) {
        observation.final = runtime.snapshot().p1;
        return observation;
    }

    runtime.step(SymbolicInput{ .s = true }, 2);
    for (int i = 0; i < 60; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        observation.sawState195 = observation.sawState195 || p1.stateNo == 195;
        if (p1.stateNo == 195 && !p1.ctrl) {
            observation.sawCtrlFalseInTaunt = true;
            observation.ctrlFalse = p1;

            runtime.step(withButton('x'), 2);
            runtime.step({}, 4);
            const auto afterBlockedInput = runtime.snapshot().p1;
            observation.attemptedCommandWhileCtrlFalse = true;
            observation.commandBlockedWhileCtrlFalse = afterBlockedInput.stateNo == 195
                && !afterBlockedInput.ctrl
                && afterBlockedInput.moveType == 'I';
            observation.blockedAttempt = afterBlockedInput;
            break;
        }
        observation.final = p1;
    }

    if (!observation.sawCtrlFalseInTaunt) {
        observation.final = runtime.snapshot().p1;
        return observation;
    }

    for (int i = 0; i < 120; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        observation.sawState195 = observation.sawState195 || p1.stateNo == 195;
        if (p1.stateNo == 195 && p1.ctrl) {
            observation.sawCtrlSetRestoreInTaunt = true;
            observation.ctrlRestore = p1;
            break;
        }
        observation.final = p1;
    }

    for (int i = 0; i < 180; ++i) {
        const auto p1 = runtime.snapshot().p1;
        if (p1.stateNo == 0 && p1.ctrl && p1.onGround && p1.moveType == 'I') {
            observation.returnedToIdle = true;
            observation.final = p1;
            break;
        }
        runtime.step({}, 1);
        observation.final = runtime.snapshot().p1;
    }

    if (observation.returnedToIdle) {
        runtime.step({}, 5);
        const auto beforeCommand = runtime.snapshot().p1;
        runtime.step(withButton(restoreCommand), 2);
        for (int i = 0; i < 18; ++i) {
            const auto afterCommand = runtime.snapshot().p1;
            observation.commandAfterRestore = afterCommand;
            if (changedStateOrAction(beforeCommand, afterCommand) && afterCommand.moveType == 'A') {
                observation.commandWorksAfterRestore = true;
                break;
            }
            runtime.step({}, 1);
        }
    }

    return observation;
}

bool tauntCtrlSetControlRestorePassed(const TauntCtrlSetObservation& observation) {
    return observation.startedFromIdle
        && observation.sawState195
        && observation.sawCtrlFalseInTaunt
        && observation.attemptedCommandWhileCtrlFalse
        && observation.commandBlockedWhileCtrlFalse
        && observation.sawCtrlSetRestoreInTaunt
        && observation.returnedToIdle
        && observation.commandWorksAfterRestore;
}

std::string tauntCtrlSetControlRestoreDetail(const TauntCtrlSetObservation& observation) {
    return "idle_before=" + std::to_string(observation.startedFromIdle ? 1 : 0)
        + " saw_195=" + std::to_string(observation.sawState195 ? 1 : 0)
        + " ctrl_false_in_195=" + std::to_string(observation.sawCtrlFalseInTaunt ? 1 : 0)
        + " ctrl_false_time=" + std::to_string(observation.ctrlFalse.stateTime)
        + " attempted_command_while_ctrl_false=" + std::to_string(observation.attemptedCommandWhileCtrlFalse ? 1 : 0)
        + " command_blocked_while_ctrl_false=" + std::to_string(observation.commandBlockedWhileCtrlFalse ? 1 : 0)
        + " blocked_state=" + std::to_string(observation.blockedAttempt.stateNo)
        + " blocked_ctrl=" + std::to_string(observation.blockedAttempt.ctrl ? 1 : 0)
        + " ctrlset_restore_in_195=" + std::to_string(observation.sawCtrlSetRestoreInTaunt ? 1 : 0)
        + " restore_time=" + std::to_string(observation.ctrlRestore.stateTime)
        + " returned_idle=" + std::to_string(observation.returnedToIdle ? 1 : 0)
        + " final_state=" + std::to_string(observation.final.stateNo)
        + " final_ctrl=" + std::to_string(observation.final.ctrl ? 1 : 0)
        + " restore_command=" + std::string(1, observation.restoreCommand)
        + " command_after_restore=" + std::to_string(observation.commandWorksAfterRestore ? 1 : 0)
        + " command_after_state=" + std::to_string(observation.commandAfterRestore.stateNo)
        + " command_after_move_type=" + std::string(1, observation.commandAfterRestore.moveType);
}

struct AirLandingObservation {
    bool sawAir = false;
    bool landed = false;
    bool reenteredAirAfterLanding = false;
    float yMin = 0.0f;
    FighterSnapshot final;
};

bool snapshotIsAirborne(const FighterSnapshot& fighter) {
    return !fighter.onGround || fighter.stateType == 'A' || fighter.y < -0.5f;
}

AirLandingObservation launchInputUntilLanding(RuntimeProbe& runtime, const SymbolicInput& input, int maxFrames) {
    AirLandingObservation observation;
    observation.yMin = runtime.snapshot().p1.y;
    runtime.step(input, 1);
    for (int i = 0; i < maxFrames; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        observation.yMin = std::min(observation.yMin, p1.y);
        if (snapshotIsAirborne(p1)) {
            if (observation.landed) {
                observation.reenteredAirAfterLanding = true;
            }
            observation.sawAir = true;
        } else if (observation.sawAir && p1.onGround) {
            observation.landed = true;
        }
        observation.final = p1;
    }
    return observation;
}

std::string airLandingDetail(const AirLandingObservation& observation) {
    return "saw_air=" + std::to_string(observation.sawAir ? 1 : 0)
        + " landed=" + std::to_string(observation.landed ? 1 : 0)
        + " reentered_air_after_landing=" + std::to_string(observation.reenteredAirAfterLanding ? 1 : 0)
        + " y_min=" + std::to_string(observation.yMin)
        + " final_y=" + std::to_string(observation.final.y)
        + " final_vy=" + std::to_string(observation.final.vy)
        + " final_state=" + std::to_string(observation.final.stateNo)
        + " final_anim=" + std::to_string(observation.final.action)
        + " final_state_type=" + std::string(1, observation.final.stateType)
        + " final_on_ground=" + std::to_string(observation.final.onGround ? 1 : 0);
}

bool airLandingPassed(const AirLandingObservation& observation) {
    return observation.sawAir
        && observation.landed
        && !observation.reenteredAirAfterLanding
        && observation.final.onGround
        && std::fabs(observation.final.y) <= 0.5f;
}

struct KungFuKneeGroundingObservation {
    bool startedFromIdle = false;
    bool sawState1050 = false;
    bool sawState1051 = false;
    bool sawState1052 = false;
    bool sawPosSetGrounding = false;
    bool returnedToIdle = false;
    float yMin = 0.0f;
    FighterSnapshot grounding;
    FighterSnapshot final;
};

void performForwardForwardA(RuntimeProbe& runtime) {
    runtime.step(SymbolicInput{ .right = true }, 1);
    runtime.step({}, 1);
    runtime.step(SymbolicInput{ .right = true }, 1);
    runtime.step(SymbolicInput{ .right = true, .a = true }, 1);
    runtime.step({}, 1);
}

KungFuKneeGroundingObservation observeKungFuKneePosSetGrounding(RuntimeProbe& runtime) {
    KungFuKneeGroundingObservation observation;
    runtime.positionFighters(-80.0f, 80.0f);
    observation.startedFromIdle = waitForControllableIdle(runtime, 360);
    observation.yMin = runtime.snapshot().p1.y;
    if (!observation.startedFromIdle) {
        observation.final = runtime.snapshot().p1;
        return observation;
    }

    performForwardForwardA(runtime);
    for (int i = 0; i < 360; ++i) {
        runtime.step({}, 1);
        const auto p1 = runtime.snapshot().p1;
        observation.yMin = std::min(observation.yMin, p1.y);
        observation.sawState1050 = observation.sawState1050 || p1.stateNo == 1050;
        observation.sawState1051 = observation.sawState1051 || p1.stateNo == 1051;
        observation.sawState1052 = observation.sawState1052 || p1.stateNo == 1052;
        if (observation.sawState1052 && p1.onGround && std::fabs(p1.y) <= 0.5f && !observation.sawPosSetGrounding) {
            observation.sawPosSetGrounding = true;
            observation.grounding = p1;
        }
        if (observation.sawState1052 && p1.stateNo == 0 && p1.ctrl && p1.onGround && p1.moveType == 'I') {
            observation.returnedToIdle = true;
            observation.final = p1;
            return observation;
        }
        observation.final = p1;
    }
    return observation;
}

bool kungFuKneeGroundingPassed(const KungFuKneeGroundingObservation& observation) {
    return observation.startedFromIdle
        && observation.sawState1050
        && observation.sawState1051
        && observation.sawState1052
        && observation.sawPosSetGrounding
        && observation.returnedToIdle
        && !snapshotIsAirborne(observation.final);
}

std::string kungFuKneeGroundingDetail(const KungFuKneeGroundingObservation& observation) {
    return "idle_before=" + std::to_string(observation.startedFromIdle ? 1 : 0)
        + " saw_1050=" + std::to_string(observation.sawState1050 ? 1 : 0)
        + " saw_1051=" + std::to_string(observation.sawState1051 ? 1 : 0)
        + " saw_1052=" + std::to_string(observation.sawState1052 ? 1 : 0)
        + " posset_grounding=" + std::to_string(observation.sawPosSetGrounding ? 1 : 0)
        + " grounding_y=" + std::to_string(observation.grounding.y)
        + " grounding_on_ground=" + std::to_string(observation.grounding.onGround ? 1 : 0)
        + " returned_idle=" + std::to_string(observation.returnedToIdle ? 1 : 0)
        + " final_state=" + std::to_string(observation.final.stateNo)
        + " final_y=" + std::to_string(observation.final.y)
        + " final_vy=" + std::to_string(observation.final.vy)
        + " final_state_type=" + std::string(1, observation.final.stateType)
        + " final_move_type=" + std::string(1, observation.final.moveType)
        + " final_ctrl=" + std::to_string(observation.final.ctrl ? 1 : 0)
        + " final_on_ground=" + std::to_string(observation.final.onGround ? 1 : 0)
        + " final_airborne=" + std::to_string(snapshotIsAirborne(observation.final) ? 1 : 0)
        + " y_min=" + std::to_string(observation.yMin);
}


} // namespace

} // namespace dragon::verification
