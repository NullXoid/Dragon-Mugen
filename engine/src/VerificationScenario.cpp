#include "VerificationScenario.h"

#include "dragon/Compatibility.h"
#include "dragon/MugenData.h"

#include "AppTypes.h"
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
int runTrainingOptionsMenuGeometry(RuntimeProbe& runtime, std::ostream& out);
int runTrainingMoveListGeometry(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTripGrounding(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenOverheadTripChain(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenOverheadTripChainStress(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTripJumpBuffer(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenAttackJumpBufferRelease(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenThrow(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenKuuchuuShakunetsu(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTrainingDemoAll(RuntimeProbe& runtime, std::ostream& out);
int runLiliTrainingDemoAll(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenShinryukenRecovery(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenShunGokuSatsu(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenShoukiHatsudouSpacing(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTrainingCommandPracticeAdvance(RuntimeProbe& runtime, std::ostream& out);
int runKfmDownHitProfile(RuntimeProbe& runtime, std::ostream& out);
int runKfmGuardRecovery(RuntimeProbe& runtime, std::ostream& out);
int runKfmSpecialsSupers(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenSpecialsSupers(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenHelperLifecycle(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenPowerChargeHelper(RuntimeProbe& runtime, std::ostream& out), runEvilKenAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out);
int runEvilKenTrainingCommandDemo(RuntimeProbe& runtime, std::ostream& out);
int runEvilRyuSpecialsSupers(RuntimeProbe& runtime, std::ostream& out), runEvilRyuShinShoryukenStun(RuntimeProbe& runtime, std::ostream& out), runEvilRyuSuperStress(RuntimeProbe& runtime, std::ostream& out), runEvilRyuAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out), runEvilRyuPowerChargeHelper(RuntimeProbe& runtime, std::ostream& out), runEvilRyuThrowBind(RuntimeProbe& runtime, std::ostream& out), runEvilRyuTrainingThrowDemo(RuntimeProbe& runtime, std::ostream& out);
int runKfmMovementDirectionAudit(RuntimeProbe& runtime, std::ostream& out);
int runEvilRyuHighJumpMovementAudit(RuntimeProbe& runtime, std::ostream& out);
int runEvilRyuDash(RuntimeProbe& runtime, std::ostream& out);
int runArenaZKeyboardControls(RuntimeProbe& runtime, std::ostream& out);
int runArenaZGamepadControls(RuntimeProbe& runtime, std::ostream& out);
int runArenaZHitDepth(RuntimeProbe& runtime, std::ostream& out), runArenaZPushDepth(RuntimeProbe& runtime, std::ostream& out), runArenaZDrawOrder(RuntimeProbe& runtime, std::ostream& out);
int runArenaCameraRotationToggle(RuntimeProbe& runtime, std::ostream& out), runArenaCameraRotationProjection(RuntimeProbe& runtime, std::ostream& out), runArenaCameraRotationDrawOrder(RuntimeProbe& runtime, std::ostream& out);
int runArenaZCpuAlign(RuntimeProbe& runtime, std::ostream& out);
int runArenaZModifierSidestep(RuntimeProbe& runtime, std::ostream& out);
int runArenaEvilKenForwardDashBounds(RuntimeProbe& runtime, std::ostream& out);
int runArenaPerFighterRuntime(RuntimeProbe& runtime, std::ostream& out);
int runArenaOpenBorScrollStage(RuntimeProbe& runtime, std::ostream& out), runArenaEvilRyuAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out);
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

AirLandingObservation holdInputUntilLanding(RuntimeProbe& runtime, const SymbolicInput& input, int maxFrames) {
    AirLandingObservation observation;
    observation.yMin = runtime.snapshot().p1.y;
    for (int i = 0; i < maxFrames; ++i) {
        runtime.step(input, 1);
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
    if (!runtime.setup("lili", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili/Mountainside Training setup failed");
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
        std::abs(idle.p1.scaleX - 0.41f) < 0.001f && std::abs(idle.p1.scaleY - 0.41f) < 0.001f
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
    if (!runtime.setup("lili", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili/Mountainside Training setup failed");
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
    if (!runtime.setup("lili", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili/Mountainside Training setup failed");
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
    if (!runtime.setup("lili", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Lili/Mountainside Training setup failed");
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

int runCpuBaseline(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside Single Player setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "cpu-baseline");

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "single_player_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " timer_ticks=" + std::to_string(runtime.snapshot().matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "cpu_checks", "Single Player fight phase was not active");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-110.0f, 110.0f);
    const auto moveBefore = runtime.snapshot();
    runtime.step({}, 90);
    const auto moveAfter = runtime.snapshot();
    const float distanceBefore = horizontalDistance(moveBefore);
    const float distanceAfter = horizontalDistance(moveAfter);
    const bool movedTowardP1 = distanceAfter < distanceBefore - 5.0f && moveAfter.p2.x < moveBefore.p2.x - 1.0f;
    record(out, counts, movedTowardP1 ? Status::Pass : Status::Fail, "cpu_moves_toward_p1",
        "p2_x_before=" + std::to_string(moveBefore.p2.x)
        + " p2_x_after=" + std::to_string(moveAfter.p2.x)
        + " distance_before=" + std::to_string(distanceBefore)
        + " distance_after=" + std::to_string(distanceAfter));

    runtime.positionFighters(-20.0f, 22.0f);
    bool sawCpuAttack = false;
    FighterSnapshot cpuAttackSnap;
    for (int i = 0; i < 135; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        if (snap.p2.moveType == 'A') {
            sawCpuAttack = true;
            cpuAttackSnap = snap.p2;
            break;
        }
    }
    record(out, counts, sawCpuAttack ? Status::Pass : Status::Fail, "cpu_attempts_normal_attack",
        sawCpuAttack
            ? "state=" + std::to_string(cpuAttackSnap.stateNo) + " anim=" + std::to_string(cpuAttackSnap.action)
            : "no CPU attack move observed");

    runtime.positionFighters(-18.0f, 22.0f);
    waitForControllableIdle(runtime, 120);
    runtime.step({}, 3);
    const auto cpuHitBefore = runtime.snapshot();
    bool sawCpuContact = false;
    bool sawCpuHitOrGuard = false;
    for (int i = 0; i < 180; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        sawCpuContact = sawCpuContact || snap.p2.moveContact || snap.p2.moveHit || snap.p2.moveGuarded;
        sawCpuHitOrGuard = sawCpuHitOrGuard || snap.p2.moveHit || snap.p2.moveGuarded || snap.p1.life < cpuHitBefore.p1.life;
    }
    const auto cpuHitAfter = runtime.snapshot();
    const bool cpuDamagedP1 = cpuHitAfter.p1.life < cpuHitBefore.p1.life;
    record(out, counts, (sawCpuContact && sawCpuHitOrGuard && cpuDamagedP1) ? Status::Pass : Status::Fail,
        "cpu_can_damage_p1",
        "contact=" + std::to_string(sawCpuContact ? 1 : 0)
        + " hit_or_guard=" + std::to_string(sawCpuHitOrGuard ? 1 : 0)
        + " p1_life_before=" + std::to_string(cpuHitBefore.p1.life)
        + " p1_life_after=" + std::to_string(cpuHitAfter.p1.life)
        + " last_hit=\"" + cpuHitAfter.lastHitText + "\"");

    runtime.positionFighters(-18.0f, 24.0f);
    waitForControllableIdle(runtime, 120);
    runtime.step({}, 3);
    const auto hitBefore = runtime.snapshot();
    runtime.step(withButton('y'), 2);
    bool sawContact = false;
    bool sawHitOrGuard = false;
    for (int i = 0; i < 60; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        sawContact = sawContact || snap.p1.moveContact || snap.p1.moveHit || snap.p1.moveGuarded;
        sawHitOrGuard = sawHitOrGuard || snap.p1.moveHit || snap.p1.moveGuarded || snap.p2.life < hitBefore.p2.life;
    }
    const auto hitAfter = runtime.snapshot();
    record(out, counts, (sawContact && sawHitOrGuard) ? Status::Pass : Status::Fail, "cpu_can_still_be_hit",
        "contact=" + std::to_string(sawContact ? 1 : 0)
        + " hit_or_guard=" + std::to_string(sawHitOrGuard ? 1 : 0)
        + " p2_life_before=" + std::to_string(hitBefore.p2.life)
        + " p2_life_after=" + std::to_string(hitAfter.p2.life)
        + " last_hit=\"" + hitAfter.lastHitText + "\"");

    const auto timerBefore = runtime.snapshot();
    runtime.step({}, 30);
    const auto timerAfter = runtime.snapshot();
    const bool timerStable = timerAfter.matchPhase == static_cast<int>(MatchPhase::Fight)
        && timerAfter.matchTimerTicks > 0
        && timerAfter.matchTimerTicks <= timerBefore.matchTimerTicks;
    record(out, counts, timerStable ? Status::Pass : Status::Fail, "single_player_timer_stability",
        "phase_before=" + std::to_string(timerBefore.matchPhase)
        + " phase_after=" + std::to_string(timerAfter.matchPhase)
        + " timer_before=" + std::to_string(timerBefore.matchTimerTicks)
        + " timer_after=" + std::to_string(timerAfter.matchTimerTicks));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaSmoke(RuntimeProbe& runtime, std::ostream& out, int cpuCount) {
    Counts counts;
    const std::string scenarioName = "arena-cpu-" + std::to_string(cpuCount);
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Arena, out, cpuCount)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside Arena setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, scenarioName);

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "arena_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " timer_ticks=" + std::to_string(runtime.snapshot().matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "arena_checks", "Arena fight phase was not active");
        summary(out, counts);
        return exitCode(counts);
    }

    const int expectedFighters = 1 + cpuCount;
    const auto start = runtime.snapshot();
    record(out, counts, start.fighterCount == expectedFighters ? Status::Pass : Status::Fail, "arena_fighter_count",
        "expected=" + std::to_string(expectedFighters)
        + " actual=" + std::to_string(start.fighterCount));
    record(out, counts, start.livingFighters == expectedFighters ? Status::Pass : Status::Fail, "arena_initial_living_count",
        "expected=" + std::to_string(expectedFighters)
        + " actual=" + std::to_string(start.livingFighters));
    record(out, counts, start.arenaRuntimeCount == expectedFighters ? Status::Pass : Status::Fail, "arena_runtime_count",
        "expected=" + std::to_string(expectedFighters)
        + " actual=" + std::to_string(start.arenaRuntimeCount));
    record(out, counts, start.matchTimerTicks == 0 ? Status::Pass : Status::Fail, "arena_timer_default_inf",
        "timer_ticks=" + std::to_string(start.matchTimerTicks));
    record(out, counts, !start.arenaDrawOrder.empty() ? Status::Pass : Status::Fail, "arena_draw_order_available",
        "draw_order=" + start.arenaDrawOrder);

    if (cpuCount == 1) {
        runtime.positionFighters(-18.0f, 24.0f); waitForControllableIdle(runtime, 120);
        runtime.setFighterControl(1, false);
        runtime.step(SymbolicInput{ .down = true, .b = true }, 4);
        bool sawTripFall = false, sawLieDown = false; FighterSnapshot tripAfter;
        for (int i = 0; i < 120; ++i) {
            runtime.step({}, 1); tripAfter = runtime.snapshot().p2;
            sawTripFall = sawTripFall || tripAfter.stateNo == 5070 || tripAfter.stateNo == 5071 || tripAfter.y < -0.5f;
            sawLieDown = sawLieDown || tripAfter.stateType == 'L' || tripAfter.stateNo == 5110 || tripAfter.stateNo == 5120;
        }
        record(out, counts, sawTripFall && sawLieDown ? Status::Pass : Status::Fail, "arena_trip_hit_knockdown",
            "trip_or_air=" + std::to_string(sawTripFall ? 1 : 0) + " liedown=" + std::to_string(sawLieDown ? 1 : 0)
            + " p2_state=" + std::to_string(tripAfter.stateNo) + " p2_y=" + std::to_string(tripAfter.y));
    }

    if (expectedFighters > 2) {
        for (int i = 1; i < expectedFighters - 1; ++i) {
            runtime.setFighterLife(i, 0);
        }
        runtime.step({}, 2);
        const auto partial = runtime.snapshot();
        const bool stillFighting = partial.livingFighters == 2
            && partial.matchPhase == static_cast<int>(MatchPhase::Fight)
            && partial.p2.stateNo >= 5000 && partial.p2.stateNo < 5200;
        record(out, counts, stillFighting ? Status::Pass : Status::Fail, "defeated_fighters_excluded",
            "living=" + std::to_string(partial.livingFighters)
            + " match_phase=" + std::to_string(partial.matchPhase)
            + " p2_state=" + std::to_string(partial.p2.stateNo));
    }

    for (int i = 1; i < expectedFighters; ++i) {
        runtime.setFighterLife(i, 0);
    }
    runtime.step({}, 2);
    const auto resolved = runtime.snapshot();
    const bool winnerPhase = resolved.matchPhase == static_cast<int>(MatchPhase::RoundFinish)
        || resolved.matchPhase == static_cast<int>(MatchPhase::RoundResult)
        || resolved.matchPhase == static_cast<int>(MatchPhase::MatchResult);
    record(out, counts, winnerPhase && resolved.roundWinner == 1 && resolved.livingFighters == 1
            && resolved.p2.stateNo >= 5000 && resolved.p2.stateNo < 5200
            ? Status::Pass
            : Status::Fail,
        "last_fighter_standing_winner",
        "phase=" + std::to_string(resolved.matchPhase)
        + " winner=" + std::to_string(resolved.roundWinner)
        + " living=" + std::to_string(resolved.livingFighters)
        + " p2_state=" + std::to_string(resolved.p2.stateNo)
        + " text=\"" + resolved.lastHitText + "\"");

    bool reachedEndScreen = false;
    for (int i = 0; i < 420; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::MatchResult)) {
            reachedEndScreen = true;
            break;
        }
        runtime.step({}, 1);
    }
    record(out, counts, reachedEndScreen ? Status::Pass : Status::Fail, "arena_end_screen",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runVsP2Runtime(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Versus, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside VS setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "vs-p2-runtime");

    const bool active = waitForActiveFight(runtime, 420);
    record(out, counts, active ? Status::Pass : Status::Fail, "fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase));
    if (!active) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto loaded = runtime.snapshot();
    const bool p2HasSeparateRuntime =
        loaded.p2RuntimeStates > loaded.p1RuntimeStates
        && loaded.p2RuntimeHitDefs > loaded.p1RuntimeHitDefs
        && loaded.p2RuntimeCommandEntries > loaded.p1RuntimeCommandEntries;
    record(out, counts, p2HasSeparateRuntime ? Status::Pass : Status::Fail, "p2_runtime_counts",
        "p1_states=" + std::to_string(loaded.p1RuntimeStates)
        + " p2_states=" + std::to_string(loaded.p2RuntimeStates)
        + " p1_hitdefs=" + std::to_string(loaded.p1RuntimeHitDefs)
        + " p2_hitdefs=" + std::to_string(loaded.p2RuntimeHitDefs)
        + " p1_command_entries=" + std::to_string(loaded.p1RuntimeCommandEntries)
        + " p2_command_entries=" + std::to_string(loaded.p2RuntimeCommandEntries));

    constexpr int kP2OnlyState = 3885;
    runtime.forceFighterState(0, kP2OnlyState);
    const auto p1Forced = runtime.snapshot();
    record(out, counts, p1Forced.p1.stateNo != kP2OnlyState ? Status::Pass : Status::Fail, "p1_rejects_p2_only_state",
        "p1_state=" + std::to_string(p1Forced.p1.stateNo)
        + " requested_state=" + std::to_string(kP2OnlyState));

    runtime.forceFighterState(1, kP2OnlyState);
    const auto p2Forced = runtime.snapshot();
    record(out, counts, p2Forced.p2.stateNo == kP2OnlyState ? Status::Pass : Status::Fail, "p2_enters_selected_runtime_state",
        "p2_state=" + std::to_string(p2Forced.p2.stateNo)
        + " p2_action=" + std::to_string(p2Forced.p2.action)
        + " requested_state=" + std::to_string(kP2OnlyState));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace

int runCompatibilityProfileResolver(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    out << "VERIFY compatibility-profile-resolver\n";

    record(out, counts,
        resolveCompatibilityProfile("") == CompatibilityProfile::Mugen11 ? Status::Pass : Status::Fail,
        "empty_version_defaults_mugen_11",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("")));
    record(out, counts,
        resolveCompatibilityProfile("04,14,2001") == CompatibilityProfile::Mugen2001 ? Status::Pass : Status::Fail,
        "legacy_version_resolves_mugen_2001",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("04,14,2001")));
    record(out, counts,
        resolveCompatibilityProfile("1.0") == CompatibilityProfile::Mugen10 ? Status::Pass : Status::Fail,
        "version_10_resolves_mugen_10",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("1.0")));
    record(out, counts,
        resolveCompatibilityProfile("1.1") == CompatibilityProfile::Mugen11 ? Status::Pass : Status::Fail,
        "version_11_resolves_mugen_11",
        std::string("profile=") + compatibilityProfileName(resolveCompatibilityProfile("1.1")));

    const LocalCoord parsedLocalCoord = parseLocalCoord("640, 480");
    record(out, counts,
        parsedLocalCoord.width == 640 && parsedLocalCoord.height == 480 ? Status::Pass : Status::Fail,
        "localcoord_parse",
        "width=" + std::to_string(parsedLocalCoord.width)
            + " height=" + std::to_string(parsedLocalCoord.height));

    const std::filesystem::path gameRoot(runtime.rootText());
    const auto characters = loadCharacters(gameRoot);
    const CharacterSlot* kfm = findCharacterById(characters, "kfm");
    const CharacterSlot* evilRyu = findCharacterById(characters, "EvilRyu");
    const CharacterSlot* evilKen = findCharacterById(characters, "EvilKen");
    record(out, counts, kfm ? Status::Pass : Status::Fail, "kfm_character_loaded",
        kfm ? "profile=" + std::string(compatibilityProfileName(kfm->compatibilityProfile)) : "missing");
    record(out, counts,
        kfm && kfm->compatibilityProfile == CompatibilityProfile::Mugen2001 ? Status::Pass : Status::Fail,
        "kfm_profile_mugen_2001",
        kfm ? "mugenversion=" + kfm->mugenVersion : "missing");
    record(out, counts,
        evilRyu && evilRyu->compatibilityProfile == CompatibilityProfile::Mugen10
            && evilRyu->localCoord.width == 320
            && evilRyu->localCoord.height == 240
            ? Status::Pass : Status::Fail,
        "evilryu_profile_localcoord",
        evilRyu ? "profile=" + std::string(compatibilityProfileName(evilRyu->compatibilityProfile))
                + " localcoord=" + std::to_string(evilRyu->localCoord.width)
                + "," + std::to_string(evilRyu->localCoord.height)
                : "missing");
    record(out, counts,
        evilKen && evilKen->compatibilityProfile == CompatibilityProfile::Mugen10
            && evilKen->localCoord.width == 320
            && evilKen->localCoord.height == 240
            ? Status::Pass : Status::Fail,
        "evilken_profile_localcoord",
        evilKen ? "profile=" + std::string(compatibilityProfileName(evilKen->compatibilityProfile))
                + " localcoord=" + std::to_string(evilKen->localCoord.width)
                + "," + std::to_string(evilKen->localCoord.height)
                : "missing");

    const auto stages = loadStages(gameRoot);
    const StageSlot* openBorStage = findLegacyOpenBorStage(stages);
    record(out, counts, openBorStage ? Status::Pass : Status::Fail, "legacy_openbor_stage_bridge",
        openBorStage ? "stage=" + openBorStage->displayName : "missing");

    if (runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        const RuntimeSnapshot snap = runtime.snapshot();
        record(out, counts, snap.runtimeMode == "Dragon" ? Status::Pass : Status::Fail, "runtime_mode_dragon_default",
            "mode=" + snap.runtimeMode);
        record(out, counts, snap.p1CompatibilityProfile == "Mugen2001" ? Status::Pass : Status::Fail,
            "runtime_p1_profile_visible",
            "profile=" + snap.p1CompatibilityProfile);
        record(out, counts,
            snap.p1LocalCoordWidth == 320 && snap.p1LocalCoordHeight == 240 && snap.p1UsesMugenSemantics
                ? Status::Pass : Status::Fail,
            "runtime_p1_context_visible",
            "localcoord=" + std::to_string(snap.p1LocalCoordWidth)
                + "," + std::to_string(snap.p1LocalCoordHeight)
                + " mugen_semantics=" + std::to_string(snap.p1UsesMugenSemantics ? 1 : 0));
    } else {
        record(out, counts, Status::Blocked, "runtime_setup", "KFM/Mountainside Training setup failed");
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runEvilKenCornerVisualBounds(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilken-corner-visual-bounds");
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-1000.0f, -950.0f);
    runtime.setFighterControl(1, false);
    runtime.step({}, 8);
    auto leftCorner = runtime.snapshot();
    float minLeftVisual = std::min(leftCorner.p1.visualScreenLeft, leftCorner.p2.visualScreenLeft);
    float maxLeftVisual = std::max(leftCorner.p1.visualScreenRight, leftCorner.p2.visualScreenRight);
    for (int i = 0; i < 45; ++i) {
        runtime.step(SymbolicInput{ .left = true }, 1);
        const auto snap = runtime.snapshot();
        minLeftVisual = std::min({ minLeftVisual, snap.p1.visualScreenLeft, snap.p2.visualScreenLeft });
        maxLeftVisual = std::max({ maxLeftVisual, snap.p1.visualScreenRight, snap.p2.visualScreenRight });
    }
    leftCorner = runtime.snapshot();
    const bool leftVisible = minLeftVisual >= -0.5f && maxLeftVisual <= static_cast<float>(leftCorner.logicalWidth) + 0.5f;
    record(out, counts, leftVisible ? Status::Pass : Status::Fail, "left_corner_visuals_stay_inside_screen",
        "min_left=" + std::to_string(minLeftVisual)
            + " max_right=" + std::to_string(maxLeftVisual)
            + " logical_width=" + std::to_string(leftCorner.logicalWidth)
            + " p1_x=" + std::to_string(leftCorner.p1.x)
            + " p2_x=" + std::to_string(leftCorner.p2.x));

    runtime.positionFighters(950.0f, 1000.0f);
    runtime.setFighterControl(1, false);
    runtime.step({}, 8);
    auto rightCorner = runtime.snapshot();
    float minRightVisual = std::min(rightCorner.p1.visualScreenLeft, rightCorner.p2.visualScreenLeft);
    float maxRightVisual = std::max(rightCorner.p1.visualScreenRight, rightCorner.p2.visualScreenRight);
    for (int i = 0; i < 45; ++i) {
        runtime.step(SymbolicInput{ .right = true }, 1);
        const auto snap = runtime.snapshot();
        minRightVisual = std::min({ minRightVisual, snap.p1.visualScreenLeft, snap.p2.visualScreenLeft });
        maxRightVisual = std::max({ maxRightVisual, snap.p1.visualScreenRight, snap.p2.visualScreenRight });
    }
    rightCorner = runtime.snapshot();
    const bool rightVisible = minRightVisual >= -0.5f && maxRightVisual <= static_cast<float>(rightCorner.logicalWidth) + 0.5f;
    record(out, counts, rightVisible ? Status::Pass : Status::Fail, "right_corner_visuals_stay_inside_screen",
        "min_left=" + std::to_string(minRightVisual)
            + " max_right=" + std::to_string(maxRightVisual)
            + " logical_width=" + std::to_string(rightCorner.logicalWidth)
            + " p1_x=" + std::to_string(rightCorner.p1.x)
            + " p2_x=" + std::to_string(rightCorner.p2.x));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingOptionsMenuGeometry(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY training-options-menu-geometry\n";

    const int pageCount = (kTrainingOptionCount + kTrainingOptionRows - 1) / kTrainingOptionRows;
    record(out, counts, pageCount == 2 ? Status::Pass : Status::Fail, "page_count",
        "page_count=" + std::to_string(pageCount)
        + " option_count=" + std::to_string(kTrainingOptionCount)
        + " page_rows=" + std::to_string(kTrainingOptionRows));

    for (int page = 0; page < pageCount; ++page) {
        TrainingOptions options;
        options.selectedOption = page * kTrainingOptionRows;
        options.dummyGuardMode = DummyGuardMode::Crouch;
        options.moveCategory = TrainingMoveCategory::Specials;

        std::vector<TrainingOptionRowView> rows;
        const int firstOption = page * kTrainingOptionRows;
        const int lastOption = std::min(kTrainingOptionCount, firstOption + kTrainingOptionRows);
        rows.reserve(kTrainingOptionRows);
        for (int option = firstOption; option < lastOption; ++option) {
            rows.push_back(TrainingOptionRowView{
                std::string(trainingOptionLabel(option)),
                trainingOptionStatus(options, option),
                option == options.selectedOption,
            });
        }

        TrainingOptionsMenuView view;
        view.rows = rows;
        view.pageLabel = std::to_string(page + 1) + "/" + std::to_string(pageCount);

        const auto selectedRows = std::count_if(rows.begin(), rows.end(), [](const TrainingOptionRowView& row) {
            return row.selected;
        });
        const auto geometry = verifyTrainingOptionsMenuGeometry(view);
        const std::string pageName = "page_" + std::to_string(page + 1);
        record(out, counts, rows.size() == static_cast<std::size_t>(kTrainingOptionRows) ? Status::Pass : Status::Fail,
            pageName + "_row_count",
            "rows=" + std::to_string(rows.size())
            + " first_option=" + std::to_string(firstOption)
            + " last_option=" + std::to_string(lastOption - 1));
        record(out, counts, selectedRows == 1 ? Status::Pass : Status::Fail,
            pageName + "_selected_row",
            "selected_rows=" + std::to_string(selectedRows)
            + " selected_option=" + std::to_string(options.selectedOption));
        record(out, counts, geometry.ok ? Status::Pass : Status::Fail,
            pageName + "_geometry_fits",
            geometry.detail);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingMoveListGeometry(RuntimeProbe&, std::ostream& out) {
    Counts counts;
    out << "VERIFY training-move-list-geometry\n";

    std::vector<TrainingMoveRowView> firstPageRows{
        { "01", "Ground Recovery Roll", "B+DB+D+A", true },
        { "02", "Shoryureppa", "D+DF+F+A", false },
        { "03", "Shinryuken", "D+F+D+F+Z", false },
        { "04", "Shippu Jinrai Kyaku", "D+DB+B+X", false },
        { "05", "Kyouja Renbu", "D+DF+F+C", false },
        { "06", "Kuuchuu Shakunetsu Hadouken", "D+B+D+B+Z", false },
        { "07", "Punch Throw", "A+S", false },
        { "08", "Stand Kick Throw", "Z+X", false },
        { "09", "Shouki Hatsudou", "B+D+F+A+S", false },
        { "10", "Rongou Kokuretsu Zan", "D+D+D+A", false },
    };
    TrainingMoveListView firstPage;
    firstPage.rows = firstPageRows;
    firstPage.detail = TrainingMoveDetailView{
        "Ground Recovery Roll",
        "5200",
        "B+DB+D+A",
        "SPECIALS",
        "LOADED",
        "B+DB+D+A",
        true,
    };
    firstPage.selectedCharacterLabel = "Evil Ken";
    firstPage.categoryLabel = "ALL";
    firstPage.pageLabel = "1/18";

    std::vector<TrainingMoveRowView> secondPageRows{
        { "11", "Hadouken", "D+DF+F+A", false },
        { "12", "Shakunetsu Hadouken", "B+DB+D+DF+F+Z", false },
        { "13", "Tatsumaki Senpuukyaku", "D+DB+B+X", false },
        { "14", "Air Tatsumaki Senpuukyaku", "D+DB+B+X", false },
        { "15", "Shin Shoryuken", "D+F+D+F+Z", false },
        { "16", "Raging Demon State 3890", "A+A+F+Z+D", false },
        { "17", "Long Compatibility Stress Command Entry", "D+DF+F+D+DF+F+A+S", false },
        { "18", "Arena Forward Dash Bounds Recovery", "F+F", false },
        { "19", "Power Charge Helper Lifecycle", "A+S+Z", false },
        { "20", "Command Training Demo Finish Advance", "D+DB+B+D+DB+B+C", true },
    };
    TrainingMoveListView secondPage;
    secondPage.rows = secondPageRows;
    secondPage.detail = TrainingMoveDetailView{
        "Command Training Demo Finish Advance",
        "3000",
        "D+DB+B+D+DB+B+C",
        "SUPERS",
        "POWER 3000",
        "D+DB+B+D+DB+B+C",
        true,
    };
    secondPage.selectedCharacterLabel = "Evil Ken";
    secondPage.categoryLabel = "SUPERS";
    secondPage.pageLabel = "20/20";

    const TrainingMoveListView pages[] = { firstPage, secondPage };
    for (int i = 0; i < 2; ++i) {
        const auto& view = pages[i];
        const auto selectedRows = std::count_if(view.rows.begin(), view.rows.end(), [](const TrainingMoveRowView& row) {
            return row.selected;
        });
        const auto geometry = verifyTrainingMoveListGeometry(view);
        const std::string pageName = "sample_" + std::to_string(i + 1);
        record(out, counts, view.rows.size() == static_cast<std::size_t>(kTrainingMoveListRows) ? Status::Pass : Status::Fail,
            pageName + "_row_count",
            "rows=" + std::to_string(view.rows.size())
            + " expected=" + std::to_string(kTrainingMoveListRows));
        record(out, counts, selectedRows == 1 ? Status::Pass : Status::Fail,
            pageName + "_selected_row",
            "selected_rows=" + std::to_string(selectedRows)
            + " page=" + view.pageLabel);
        record(out, counts, geometry.ok ? Status::Pass : Status::Fail,
            pageName + "_geometry_fits",
            geometry.detail);
    }

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runTrainingShowSelectHold(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilKen", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ken/Mountainside Training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "training-show-select-hold");

    const bool idle = waitForControllableIdle(runtime, 420);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!idle) {
        summary(out, counts);
        return exitCode(counts);
    }

    const auto moves = runtime.trainingMoves();
    record(out, counts, moves.size() >= 2 ? Status::Pass : Status::Fail, "training_moves_available",
        "moves=" + std::to_string(moves.size()));
    if (moves.size() < 2) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.selectTrainingMoveIndex(0);
    const std::string firstLabel = moves[0].label;
    const std::string secondLabel = moves[1].label;
    runtime.holdTrainingShowSelect(true, 30);
    runtime.holdTrainingShowSelect(false, 1);
    const auto tap = runtime.snapshot();
    record(out, counts, tap.selectedTrainingMoveLabel == secondLabel ? Status::Pass : Status::Fail,
        "short_select_tap_advances_move",
        "before=\"" + firstLabel + "\" after=\"" + tap.selectedTrainingMoveLabel + "\" expected=\"" + secondLabel + "\"");

    const bool selectedHadouken = runtime.selectTrainingMove("Hadouken");
    record(out, counts, selectedHadouken ? Status::Pass : Status::Fail, "selected_hadouken_for_hold_demo",
        "selected=\"" + runtime.snapshot().selectedTrainingMoveLabel + "\"");
    if (!selectedHadouken) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.holdTrainingShowSelect(true, 119);
    const auto beforeThreshold = runtime.snapshot();
    record(out, counts, beforeThreshold.p2.stateNo != 1000 ? Status::Pass : Status::Fail,
        "hold_before_two_seconds_does_not_start_demo",
        "p2_state=" + std::to_string(beforeThreshold.p2.stateNo)
        + " selected=\"" + beforeThreshold.selectedTrainingMoveLabel + "\"");

    runtime.holdTrainingShowSelect(true, 1);
    runtime.holdTrainingShowSelect(false, 1);

    bool sawHadouken = false;
    FighterSnapshot finalP2;
    std::string commands;
    for (int frame = 0; frame < 90; ++frame) {
        const auto snap = runtime.snapshot();
        finalP2 = snap.p2;
        commands = snap.p2Commands;
        sawHadouken = sawHadouken || snap.p2.stateNo == 1000 || snap.p2.action == 1000;
        if (sawHadouken) {
            break;
        }
        runtime.step({}, 1);
    }
    const auto afterHold = runtime.snapshot();
    record(out, counts, afterHold.selectedTrainingMoveLabel == "Hadouken" ? Status::Pass : Status::Fail,
        "long_select_hold_keeps_selected_move",
        "selected=\"" + afterHold.selectedTrainingMoveLabel + "\"");
    record(out, counts, sawHadouken ? Status::Pass : Status::Fail,
        "long_select_hold_starts_show_command_demo",
        "p2_state=" + std::to_string(finalP2.stateNo)
        + " p2_action=" + std::to_string(finalP2.action)
        + " commands=\"" + commands + "\"");

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out) {
    if (scenarioName == "compatibility-profile-resolver") return runCompatibilityProfileResolver(runtime, out);
    if (scenarioName == "training-options-menu-geometry") return runTrainingOptionsMenuGeometry(runtime, out);
    if (scenarioName == "training-move-list-geometry") return runTrainingMoveListGeometry(runtime, out);
    if (scenarioName == "training-show-select-hold") return runTrainingShowSelectHold(runtime, out);
    if (scenarioName == "character-auto-fit-scale") return runCharacterAutoFitScale(runtime, out);
    if (scenarioName == "lili-smoke") return runLiliSmoke(runtime, out);
    if (scenarioName == "lili-changeanim2-fallback") return runLiliChangeAnim2Fallback(runtime, out);
    if (scenarioName == "lili-kuuch-state-fallback") return runLiliKuuchStateFallback(runtime, out);
    if (scenarioName == "lili-hien-houou-kyaku-demo") return runLiliHienHououKyakuDemo(runtime, out);
    if (scenarioName == "kfm-baseline") return runKfmBaseline(runtime, out);
    if (scenarioName == "kfm-throw") return runKfmThrow(runtime, out);
    if (scenarioName == "kfm-air-state") return runKfmAirState(runtime, out);
    if (scenarioName == "kfm-movement-direction-audit") return runKfmMovementDirectionAudit(runtime, out);
    if (scenarioName == "evilryu-high-jump") return runEvilRyuHighJumpMovementAudit(runtime, out);
    if (scenarioName == "kfm-down-hit-profile") return runKfmDownHitProfile(runtime, out);
    if (scenarioName == "kfm-guard-recovery") return runKfmGuardRecovery(runtime, out);
    if (scenarioName == "kfm-specials-supers") return runKfmSpecialsSupers(runtime, out);
    if (scenarioName == "evilken-specials-supers") return runEvilKenSpecialsSupers(runtime, out);
    if (scenarioName == "evilken-helper-lifecycle") return runEvilKenHelperLifecycle(runtime, out);
    if (scenarioName == "evilken-power-charge-helper") return runEvilKenPowerChargeHelper(runtime, out);
    if (scenarioName == "evilken-air-special-contact-landing") return runEvilKenAirSpecialContactLanding(runtime, out);
    if (scenarioName == "evilken-training-demo-hit") return runEvilKenTrainingCommandDemo(runtime, out);
    if (scenarioName == "evilken-training-command-practice-advance") return runEvilKenTrainingCommandPracticeAdvance(runtime, out);
    if (scenarioName == "evilryu-specials-supers") return runEvilRyuSpecialsSupers(runtime, out);
    if (scenarioName == "evilryu-shin-shoryuken-stun") return runEvilRyuShinShoryukenStun(runtime, out);
    if (scenarioName == "evilryu-super-stress") return runEvilRyuSuperStress(runtime, out);
    if (scenarioName == "evilryu-air-special-contact-landing") return runEvilRyuAirSpecialContactLanding(runtime, out);
    if (scenarioName == "evilryu-power-charge-helper") return runEvilRyuPowerChargeHelper(runtime, out);
    if (scenarioName == "evilryu-throw-bind") return runEvilRyuThrowBind(runtime, out);
    if (scenarioName == "evilryu-training-throw-demo") return runEvilRyuTrainingThrowDemo(runtime, out);
    if (scenarioName == "evilken-smoke") return runEvilKenSmoke(runtime, out);
    if (scenarioName == "evilken-trip-grounding") return runEvilKenTripGrounding(runtime, out);
    if (scenarioName == "evilken-overhead-trip-chain") return runEvilKenOverheadTripChain(runtime, out);
    if (scenarioName == "evilken-overhead-trip-chain-stress") return runEvilKenOverheadTripChainStress(runtime, out);
    if (scenarioName == "evilken-trip-jump-buffer") return runEvilKenTripJumpBuffer(runtime, out);
    if (scenarioName == "evilken-attack-jump-buffer-release") return runEvilKenAttackJumpBufferRelease(runtime, out);
    if (scenarioName == "evilken-throw") return runEvilKenThrow(runtime, out);
    if (scenarioName == "evilken-corner-visual-bounds") return runEvilKenCornerVisualBounds(runtime, out);
    if (scenarioName == "evilken-kuuchuu-shakunetsu") return runEvilKenKuuchuuShakunetsu(runtime, out);
    if (scenarioName == "evilken-training-demo-all") return runEvilKenTrainingDemoAll(runtime, out);
    if (scenarioName == "lili-training-demo-all") return runLiliTrainingDemoAll(runtime, out);
    if (scenarioName == "evilken-shinryuken-recovery") return runEvilKenShinryukenRecovery(runtime, out);
    if (scenarioName == "evilken-shun-goku-satsu") return runEvilKenShunGokuSatsu(runtime, out);
    if (scenarioName == "evilken-shouki-hatsudou-spacing") return runEvilKenShoukiHatsudouSpacing(runtime, out);
    if (scenarioName == "cpu-baseline") return runCpuBaseline(runtime, out);
    if (scenarioName == "vs-p2-runtime") return runVsP2Runtime(runtime, out);
    if (scenarioName == "arena-cpu-1") return runArenaSmoke(runtime, out, 1);
    if (scenarioName == "arena-cpu-2") return runArenaSmoke(runtime, out, 2);
    if (scenarioName == "arena-cpu-3") return runArenaSmoke(runtime, out, 3);
    if (scenarioName == "arena-z-keyboard-controls") return runArenaZKeyboardControls(runtime, out);
    if (scenarioName == "arena-z-gamepad-controls") return runArenaZGamepadControls(runtime, out);
    if (scenarioName == "arena-z-hit-depth") return runArenaZHitDepth(runtime, out);
    if (scenarioName == "arena-z-push-depth") return runArenaZPushDepth(runtime, out);
    if (scenarioName == "arena-z-draw-order") return runArenaZDrawOrder(runtime, out);
    if (scenarioName == "arena-camera-rotation-toggle") return runArenaCameraRotationToggle(runtime, out);
    if (scenarioName == "arena-camera-rotation-projection") return runArenaCameraRotationProjection(runtime, out);
    if (scenarioName == "arena-camera-rotation-draw-order") return runArenaCameraRotationDrawOrder(runtime, out);
    if (scenarioName == "arena-z-cpu-align") return runArenaZCpuAlign(runtime, out);
    if (scenarioName == "arena-z-modifier-sidestep") return runArenaZModifierSidestep(runtime, out);
    if (scenarioName == "arena-evilken-forward-dash-bounds") return runArenaEvilKenForwardDashBounds(runtime, out);
    if (scenarioName == "arena-per-fighter-runtime") return runArenaPerFighterRuntime(runtime, out);
    if (scenarioName == "arena-openbor-scroll-stage") return runArenaOpenBorScrollStage(runtime, out);
    if (scenarioName == "arena-evilryu-air-special-contact-landing") return runArenaEvilRyuAirSpecialContactLanding(runtime, out);
    if (scenarioName == "evilryu-dash") return runEvilRyuDash(runtime, out);

    out << "VERIFY " << scenarioName << "\n"
        << "BLOCKED unknown_scenario\n"
        << "  supported: compatibility-profile-resolver, training-options-menu-geometry, training-move-list-geometry, training-show-select-hold, character-auto-fit-scale, lili-smoke, lili-changeanim2-fallback, lili-kuuch-state-fallback, lili-hien-houou-kyaku-demo, lili-training-demo-all, kfm-baseline, kfm-throw, kfm-air-state, kfm-movement-direction-audit, evilryu-high-jump, kfm-down-hit-profile, kfm-guard-recovery, kfm-specials-supers, evilken-specials-supers, evilken-helper-lifecycle, evilken-power-charge-helper, evilken-air-special-contact-landing, evilken-training-demo-hit, evilken-training-command-practice-advance, evilryu-specials-supers, evilryu-shin-shoryuken-stun, evilryu-super-stress, evilryu-air-special-contact-landing, evilryu-power-charge-helper, evilryu-throw-bind, evilryu-training-throw-demo, evilken-smoke, evilken-trip-grounding, evilken-overhead-trip-chain, evilken-overhead-trip-chain-stress, evilken-trip-jump-buffer, evilken-attack-jump-buffer-release, evilken-throw, evilken-corner-visual-bounds, evilken-kuuchuu-shakunetsu, evilken-training-demo-all, evilken-shinryuken-recovery, evilken-shun-goku-satsu, evilken-shouki-hatsudou-spacing, cpu-baseline, vs-p2-runtime, arena-cpu-1, arena-cpu-2, arena-cpu-3, arena-z-keyboard-controls, arena-z-gamepad-controls, arena-z-hit-depth, arena-z-push-depth, arena-z-draw-order, arena-camera-rotation-toggle, arena-camera-rotation-projection, arena-camera-rotation-draw-order, arena-z-cpu-align, arena-z-modifier-sidestep, arena-evilken-forward-dash-bounds, arena-per-fighter-runtime, arena-openbor-scroll-stage, arena-evilryu-air-special-contact-landing, evilryu-dash\n"
        << "SUMMARY pass=0 partial=0 fail=0 blocked=1\n";
    return 2;
}

} // namespace dragon::verification
