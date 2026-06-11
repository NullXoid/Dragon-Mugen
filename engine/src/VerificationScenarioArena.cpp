#include "VerificationScenario.h"

#include "AppTypes.h"

#include <cmath>
#include <ostream>
#include <string>
#include <string_view>

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

bool waitForActiveFight(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight)) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight);
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

bool snapshotIsAirborne(const FighterSnapshot& fighter) {
    return !fighter.onGround || fighter.stateType == 'A' || fighter.y < -0.5f;
}

float horizontalDistance(const RuntimeSnapshot& snapshot) {
    return std::fabs(snapshot.p2.x - snapshot.p1.x);
}

float depthDistance(const RuntimeSnapshot& snapshot) {
    return std::fabs(snapshot.p2.depthZ - snapshot.p1.depthZ);
}

bool setupArenaFight(
    RuntimeProbe& runtime,
    std::ostream& out,
    Counts& counts,
    std::string_view scenarioName,
    std::string_view p1Id = "kfm") {
    if (!runtime.setup(p1Id, "Mountainside", ScenarioMode::Arena, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Arena setup failed");
        summary(out, counts);
        return false;
    }
    header(out, runtime, scenarioName);

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "arena_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " timer_ticks=" + std::to_string(runtime.snapshot().matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "arena_checks", "Arena fight phase was not active");
        summary(out, counts);
        return false;
    }
    return true;
}

} // namespace

int runArenaZKeyboardControls(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-z-keyboard-controls")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-60.0f, 60.0f);
    runtime.setFighterControl(1, false);
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0));
    if (!idle) {
        record(out, counts, Status::Blocked, "keyboard_depth_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    const auto depthBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .down = true, .depthModifier = true }, 16);
    const auto depthAfter = runtime.snapshot();
    const bool movedDepth = depthAfter.p1.depthZ > depthBefore.p1.depthZ + 4.0f;
    const bool stayedGrounded = depthAfter.p1.onGround
        && std::fabs(depthAfter.p1.y - depthBefore.p1.y) <= 0.5f
        && depthAfter.p1.stateType != 'C';
    record(out, counts, movedDepth && stayedGrounded ? Status::Pass : Status::Fail,
        "shift_down_moves_depth_without_crouch",
        "depth_before=" + std::to_string(depthBefore.p1.depthZ)
        + " depth_after=" + std::to_string(depthAfter.p1.depthZ)
        + " y_after=" + std::to_string(depthAfter.p1.y)
        + " state_type=" + std::string(1, depthAfter.p1.stateType)
        + " state=" + std::to_string(depthAfter.p1.stateNo));

    runtime.positionFighters(-60.0f, 60.0f);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, 0.0f);
    const auto jumpBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .up = true }, 4);
    bool sawAir = false;
    for (int i = 0; i < 90; ++i) {
        runtime.step({}, 1);
        sawAir = sawAir || snapshotIsAirborne(runtime.snapshot().p1);
    }
    const auto jumpAfter = runtime.snapshot();
    record(out, counts, sawAir && std::fabs(jumpAfter.p1.depthZ - jumpBefore.p1.depthZ) <= 0.5f
            ? Status::Pass
            : Status::Fail,
        "normal_up_still_jumps",
        "saw_air=" + std::to_string(sawAir ? 1 : 0)
        + " depth_before=" + std::to_string(jumpBefore.p1.depthZ)
        + " depth_after=" + std::to_string(jumpAfter.p1.depthZ)
        + " final_y=" + std::to_string(jumpAfter.p1.y));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaZGamepadControls(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-z-gamepad-controls")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-60.0f, 60.0f);
    runtime.setFighterControl(1, false);
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0));
    if (!idle) {
        record(out, counts, Status::Blocked, "gamepad_depth_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto startBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .start = true }, 12);
    const auto startAfter = runtime.snapshot();
    const bool startIgnoredByFighterButtons = startAfter.p1.stateNo == startBefore.p1.stateNo
        && startAfter.p1.action == startBefore.p1.action
        && std::fabs(startAfter.p1.depthZ - startBefore.p1.depthZ) <= 0.5f
        && startAfter.p1.moveType == 'I';
    record(out, counts, startIgnoredByFighterButtons ? Status::Pass : Status::Fail,
        "gamepad_start_not_fighter_button",
        "state_before=" + std::to_string(startBefore.p1.stateNo)
        + " state_after=" + std::to_string(startAfter.p1.stateNo)
        + " action_before=" + std::to_string(startBefore.p1.action)
        + " action_after=" + std::to_string(startAfter.p1.action)
        + " depth_after=" + std::to_string(startAfter.p1.depthZ));

    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    const auto depthBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .down = true, .depthModifier = true }, 16);
    const auto depthAfter = runtime.snapshot();
    const bool leftTriggerDepth = depthAfter.p1.depthZ > depthBefore.p1.depthZ + 4.0f
        && depthAfter.p1.onGround
        && depthAfter.p1.stateType != 'C';
    record(out, counts, leftTriggerDepth ? Status::Pass : Status::Fail,
        "left_trigger_down_moves_depth",
        "depth_before=" + std::to_string(depthBefore.p1.depthZ)
        + " depth_after=" + std::to_string(depthAfter.p1.depthZ)
        + " state_type=" + std::string(1, depthAfter.p1.stateType)
        + " y_after=" + std::to_string(depthAfter.p1.y));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaZHitDepth(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-z-hit-depth")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-18.0f, 24.0f);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, -48.0f);
    runtime.setFighterDepth(1, 48.0f);
    const auto farBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .down = true, .b = true }, 4);
    bool farContact = false;
    for (int i = 0; i < 120; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        farContact = farContact || snap.p1.moveContact || snap.p1.moveHit || snap.p1.moveGuarded;
    }
    const auto farAfter = runtime.snapshot();
    record(out, counts, !farContact && farAfter.p2.life == farBefore.p2.life ? Status::Pass : Status::Fail,
        "depth_separated_attack_whiffs",
        "contact=" + std::to_string(farContact ? 1 : 0)
        + " p2_life_before=" + std::to_string(farBefore.p2.life)
        + " p2_life_after=" + std::to_string(farAfter.p2.life)
        + " depth_distance=" + std::to_string(depthDistance(farBefore)));

    runtime.positionFighters(-18.0f, 24.0f);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    const auto closeBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .down = true, .b = true }, 4);
    bool closeContact = false;
    bool closeHitOrDamage = false;
    for (int i = 0; i < 120; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        closeContact = closeContact || snap.p1.moveContact || snap.p1.moveHit || snap.p1.moveGuarded;
        closeHitOrDamage = closeHitOrDamage || snap.p1.moveHit || snap.p1.moveGuarded || snap.p2.life < closeBefore.p2.life;
    }
    const auto closeAfter = runtime.snapshot();
    record(out, counts, closeContact && closeHitOrDamage ? Status::Pass : Status::Fail,
        "same_depth_attack_connects",
        "contact=" + std::to_string(closeContact ? 1 : 0)
        + " hit_or_damage=" + std::to_string(closeHitOrDamage ? 1 : 0)
        + " p2_life_before=" + std::to_string(closeBefore.p2.life)
        + " p2_life_after=" + std::to_string(closeAfter.p2.life)
        + " last_hit=\"" + closeAfter.lastHitText + "\"");

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaZPushDepth(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-z-push-depth")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-1.0f, 1.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    const float sameBefore = horizontalDistance(runtime.snapshot());
    runtime.step({}, 3);
    const float sameAfter = horizontalDistance(runtime.snapshot());
    record(out, counts, sameAfter > sameBefore + 1.0f ? Status::Pass : Status::Fail,
        "same_depth_push_separates_x",
        "distance_before=" + std::to_string(sameBefore)
        + " distance_after=" + std::to_string(sameAfter));

    runtime.positionFighters(-1.0f, 1.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(0, true);
    runtime.setFighterControl(1, false);
    runtime.setFighterDepth(0, -32.0f);
    runtime.setFighterDepth(1, 32.0f);
    const float splitBefore = horizontalDistance(runtime.snapshot());
    runtime.step({}, 3);
    const float splitAfter = horizontalDistance(runtime.snapshot());
    record(out, counts, std::fabs(splitAfter - splitBefore) <= 0.75f ? Status::Pass : Status::Fail,
        "separated_depth_allows_x_overlap",
        "distance_before=" + std::to_string(splitBefore)
        + " distance_after=" + std::to_string(splitAfter)
        + " depth_distance=" + std::to_string(depthDistance(runtime.snapshot())));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaZDrawOrder(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-z-draw-order")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-40.0f, 40.0f);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, -24.0f);
    runtime.setFighterDepth(1, 24.0f);
    const auto frontP2 = runtime.snapshot();
    record(out, counts, frontP2.arenaDrawOrder == "0,1" ? Status::Pass : Status::Fail,
        "larger_depth_draws_later",
        "draw_order=" + frontP2.arenaDrawOrder
        + " p1_depth=" + std::to_string(frontP2.p1.depthZ)
        + " p2_depth=" + std::to_string(frontP2.p2.depthZ));

    runtime.setFighterDepth(0, 24.0f);
    runtime.setFighterDepth(1, -24.0f);
    const auto frontP1 = runtime.snapshot();
    record(out, counts, frontP1.arenaDrawOrder == "1,0" ? Status::Pass : Status::Fail,
        "reversed_depth_reverses_order",
        "draw_order=" + frontP1.arenaDrawOrder
        + " p1_depth=" + std::to_string(frontP1.p1.depthZ)
        + " p2_depth=" + std::to_string(frontP1.p2.depthZ));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaZCpuAlign(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-z-cpu-align")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-24.0f, 24.0f);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, 40.0f);
    runtime.setFighterDepth(1, -40.0f);
    const auto before = runtime.snapshot();
    runtime.step({}, 90);
    const auto after = runtime.snapshot();
    const bool alignedCloser = depthDistance(after) < depthDistance(before) - 10.0f
        && after.p2.depthZ > before.p2.depthZ + 4.0f;
    record(out, counts, alignedCloser ? Status::Pass : Status::Fail,
        "cpu_aligns_depth_toward_target",
        "depth_distance_before=" + std::to_string(depthDistance(before))
        + " depth_distance_after=" + std::to_string(depthDistance(after))
        + " p2_depth_before=" + std::to_string(before.p2.depthZ)
        + " p2_depth_after=" + std::to_string(after.p2.depthZ));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaPerFighterRuntime(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-per-fighter-runtime", "EvilKen")) {
        return exitCode(counts);
    }

    const auto start = runtime.snapshot();
    record(out, counts, start.arenaRuntimeCount == start.fighterCount ? Status::Pass : Status::Fail,
        "arena_runtime_for_each_fighter",
        "runtime_count=" + std::to_string(start.arenaRuntimeCount)
        + " fighter_count=" + std::to_string(start.fighterCount));

    runtime.positionFighters(-70.0f, 80.0f);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.spawnHelper(0, 101, 60060, 9999, 9999);
    runtime.step({}, 8);
    const auto helper = runtime.snapshot();
    record(out, counts, helper.activeHelpers > 0 && helper.firstHelperState == 60060 ? Status::Pass : Status::Fail,
        "owner_runtime_spawns_helper_state",
        "active_helpers=" + std::to_string(helper.activeHelpers)
        + " first_helper_state=" + std::to_string(helper.firstHelperState)
        + " first_helper_action=" + std::to_string(helper.firstHelperAction));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
