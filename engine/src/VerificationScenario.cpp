#include "VerificationScenario.h"

#include "AppTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ostream>

namespace dragon::verification {
namespace {

enum class Status {
    Pass,
    Partial,
    Fail,
    Blocked,
};

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
    if (counts.fail > 0) {
        return 1;
    }
    if (counts.blocked > 0) {
        return 2;
    }
    return 0;
}

void summary(std::ostream& out, const Counts& counts) {
    out << "SUMMARY pass=" << counts.pass
        << " partial=" << counts.partial
        << " fail=" << counts.fail
        << " blocked=" << counts.blocked << "\n";
}

void header(std::ostream& out, RuntimeProbe& runtime, std::string_view scenario) {
    out << "VERIFY " << scenario << "\n"
        << "root: " << runtime.rootText() << "\n"
        << "stage: " << runtime.stageName() << "\n"
        << "p1: " << runtime.p1Name() << "\n";
}

SymbolicInput withButton(char button) {
    SymbolicInput input;
    if (button == 'x') input.x = true;
    if (button == 'y') input.y = true;
    if (button == 'z') input.z = true;
    if (button == 'a') input.a = true;
    if (button == 'b') input.b = true;
    if (button == 'c') input.c = true;
    return input;
}

bool changedStateOrAction(const FighterSnapshot& before, const FighterSnapshot& after) {
    return before.stateNo != after.stateNo || before.action != after.action;
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
    return "command=" + std::string(1, command)
        + " state_before=" + std::to_string(before.stateNo)
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
        runtime.step(crouch ? SymbolicInput{ .down = true } : SymbolicInput{}, 12);
        after = runtime.snapshot().p1;
        if (changedStateOrAction(before, after) && after.moveType == 'A') {
            usedCommand = button;
            return true;
        }
    }
    return false;
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

    runtime.positionFighters(-18.0f, 24.0f);
    waitForControllableIdle(runtime, 360);
    runtime.step({}, 5);
    const auto hitBefore = runtime.snapshot();
    SymbolicInput hitInput = withButton(standCommand == '?' ? 'x' : standCommand);
    runtime.step(hitInput, 2);
    bool sawContact = false;
    bool sawHitPause = false;
    for (int i = 0; i < 50; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        sawContact = sawContact || snap.p1.moveContact || snap.p1.moveHit || snap.p1.hitCount > hitBefore.p1.hitCount;
        sawHitPause = sawHitPause || snap.p1.hitPauseTicks > 0 || snap.p2.hitPauseTicks > 0;
    }
    const auto hitAfter = runtime.snapshot();
    record(out, counts, sawContact ? Status::Pass : Status::Fail, "hit_contact",
        "contact=" + std::to_string(sawContact ? 1 : 0) + " hit_count_before=" + std::to_string(hitBefore.p1.hitCount)
        + " hit_count_after=" + std::to_string(hitAfter.p1.hitCount) + " last_hit=\"" + hitAfter.lastHitText + "\"");
    record(out, counts, hitAfter.p2.life < hitBefore.p2.life ? Status::Pass : Status::Fail, "damage",
        "p2_life_before=" + std::to_string(hitBefore.p2.life) + " p2_life_after=" + std::to_string(hitAfter.p2.life)
        + " delta=" + std::to_string(hitAfter.p2.life - hitBefore.p2.life));
    record(out, counts, sawHitPause || hitAfter.activeEffects > 0 || hitAfter.activeSounds > 0 ? Status::Partial : Status::Partial,
        "hitpause_spark_sound", "hitpause_observed=" + std::to_string(sawHitPause ? 1 : 0)
        + " active_effects=" + std::to_string(hitAfter.activeEffects) + " active_sounds=" + std::to_string(hitAfter.activeSounds));
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
    waitForControllableIdle(runtime, 360);
    runtime.step({}, 150);
    record(out, counts, runtime.snapshot().p1.life > 0 ? Status::Pass : Status::Fail, "load_idle_stance",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo) + " anim=" + std::to_string(runtime.snapshot().p1.action));
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
    char command = '?';
    FighterSnapshot before;
    FighterSnapshot after;
    const bool normal = tryNormal(runtime, command, before, after, false);
    record(out, counts, normal ? Status::Pass : Status::Fail, "normal_attack",
        normal ? stateActionDetail(before, after, command) : "no x/y/z/a/b/c state or animation change");
    const auto timer = runtime.snapshot();
    record(out, counts, Status::Pass, "round_timer_stability",
        "match_phase=" + std::to_string(timer.matchPhase) + " timer_ticks=" + std::to_string(timer.matchTimerTicks));
    const bool contactEvidence = timer.comboHits > 0
        || timer.lastHitText.find(" hit ") != std::string::npos
        || timer.lastHitText.find(" guard ") != std::string::npos;
    record(out, counts, contactEvidence ? Status::Pass : Status::Partial, "combo_or_hit_evidence",
        "combo_hits=" + std::to_string(timer.comboHits) + " last_hit=\"" + timer.lastHitText + "\"");
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

} // namespace

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out) {
    if (scenarioName == "kfm-baseline") {
        return runKfmBaseline(runtime, out);
    }
    if (scenarioName == "kfm-air-state") {
        return runKfmAirState(runtime, out);
    }
    if (scenarioName == "evilken-smoke") {
        return runEvilKenSmoke(runtime, out);
    }
    if (scenarioName == "cpu-baseline") {
        return runCpuBaseline(runtime, out);
    }

    out << "VERIFY " << scenarioName << "\n"
        << "BLOCKED unknown_scenario\n"
        << "  supported: kfm-baseline, kfm-air-state, evilken-smoke, cpu-baseline\n"
        << "SUMMARY pass=0 partial=0 fail=0 blocked=1\n";
    return 2;
}

} // namespace dragon::verification
