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
    std::string_view p1Id = "kfm",
    std::string_view stageHint = "Mountainside") {
    if (!runtime.setup(p1Id, stageHint, ScenarioMode::Arena, out, 1)) {
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
    runtime.setArenaCameraRotationEnabled(true);

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
    const bool walkingAction = depthAfter.p1.action == 20 || depthAfter.p1.action == 21;
    record(out, counts, movedDepth && stayedGrounded && walkingAction && depthAfter.arenaCameraRotationActive
            ? Status::Pass
            : Status::Fail,
        "shift_down_moves_depth_without_crouch",
        "depth_before=" + std::to_string(depthBefore.p1.depthZ)
        + " depth_after=" + std::to_string(depthAfter.p1.depthZ)
        + " yaw=" + std::to_string(depthAfter.arenaCameraYawDeg)
        + " y_after=" + std::to_string(depthAfter.p1.y)
        + " state_type=" + std::string(1, depthAfter.p1.stateType)
        + " state=" + std::to_string(depthAfter.p1.stateNo)
        + " action=" + std::to_string(depthAfter.p1.action)
        + " anim_tick=" + std::to_string(depthAfter.p1.animTick));

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
    runtime.setArenaCameraRotationEnabled(true);

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
    const bool walkingAction = depthAfter.p1.action == 20 || depthAfter.p1.action == 21;
    record(out, counts, leftTriggerDepth && walkingAction && depthAfter.arenaCameraRotationActive
            ? Status::Pass
            : Status::Fail,
        "left_trigger_down_moves_depth",
        "depth_before=" + std::to_string(depthBefore.p1.depthZ)
        + " depth_after=" + std::to_string(depthAfter.p1.depthZ)
        + " yaw=" + std::to_string(depthAfter.arenaCameraYawDeg)
        + " state_type=" + std::string(1, depthAfter.p1.stateType)
        + " y_after=" + std::to_string(depthAfter.p1.y)
        + " action=" + std::to_string(depthAfter.p1.action)
        + " anim_tick=" + std::to_string(depthAfter.p1.animTick));

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

int runArenaCameraRotationToggle(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-camera-rotation-toggle")) {
        return exitCode(counts);
    }

    const auto defaults = runtime.snapshot();
    record(out, counts,
        defaults.arenaZAxisEnabled
            && !defaults.arenaCameraRotationSelected
            && !defaults.arenaCameraRotationActive
            && std::fabs(defaults.arenaCameraYawDeg) <= 0.01f
            ? Status::Pass
            : Status::Fail,
        "camera_rotation_defaults_off",
        "z_axis=" + std::to_string(defaults.arenaZAxisEnabled ? 1 : 0)
        + " selected=" + std::to_string(defaults.arenaCameraRotationSelected ? 1 : 0)
        + " active=" + std::to_string(defaults.arenaCameraRotationActive ? 1 : 0)
        + " yaw=" + std::to_string(defaults.arenaCameraYawDeg));

    runtime.setFighterDepth(0, 48.0f);
    runtime.setArenaCameraRotationEnabled(true);
    runtime.step({}, 24);
    const auto enabled = runtime.snapshot();
    record(out, counts,
        enabled.arenaCameraRotationSelected
            && enabled.arenaCameraRotationActive
            && enabled.arenaCameraTargetYawDeg < -10.0f
            && enabled.arenaCameraYawDeg < -10.0f
            ? Status::Pass
            : Status::Fail,
        "camera_rotation_toggle_enters_match",
        "selected=" + std::to_string(enabled.arenaCameraRotationSelected ? 1 : 0)
        + " active=" + std::to_string(enabled.arenaCameraRotationActive ? 1 : 0)
        + " yaw=" + std::to_string(enabled.arenaCameraYawDeg)
        + " target=" + std::to_string(enabled.arenaCameraTargetYawDeg));

    runtime.setArenaZAxisEnabled(false);
    const auto zOff = runtime.snapshot();
    record(out, counts,
        !zOff.arenaZAxisEnabled
            && zOff.arenaCameraRotationSelected
            && !zOff.arenaCameraRotationActive
            && std::fabs(zOff.arenaCameraYawDeg) <= 0.01f
            && std::fabs(zOff.arenaCameraTargetYawDeg) <= 0.01f
            && std::fabs(zOff.p1.viewDepth) <= 0.01f
            ? Status::Pass
            : Status::Fail,
        "z_axis_off_forces_neutral_projection",
        "z_axis=" + std::to_string(zOff.arenaZAxisEnabled ? 1 : 0)
        + " selected=" + std::to_string(zOff.arenaCameraRotationSelected ? 1 : 0)
        + " active=" + std::to_string(zOff.arenaCameraRotationActive ? 1 : 0)
        + " yaw=" + std::to_string(zOff.arenaCameraYawDeg)
        + " view_depth=" + std::to_string(zOff.p1.viewDepth));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaCameraRotationProjection(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-camera-rotation-projection")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-80.0f, 80.0f);
    runtime.setFighterLife(1, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, 48.0f);
    runtime.setFighterDepth(1, 0.0f);
    runtime.setArenaCameraRotationEnabled(false);
    runtime.step({}, 2);
    const auto baseline = runtime.snapshot();

    runtime.setArenaCameraRotationEnabled(true);
    runtime.step({}, 36);
    const auto rotated = runtime.snapshot();
    const bool yawApproachesTarget = rotated.arenaCameraRotationActive
        && rotated.arenaCameraTargetYawDeg <= -17.0f
        && rotated.arenaCameraYawDeg <= -15.0f;
    record(out, counts, yawApproachesTarget ? Status::Pass : Status::Fail,
        "positive_depth_drives_negative_yaw",
        "yaw=" + std::to_string(rotated.arenaCameraYawDeg)
        + " target=" + std::to_string(rotated.arenaCameraTargetYawDeg)
        + " depth=" + std::to_string(rotated.p1.depthZ));

    const bool projectionChanged =
        std::fabs(rotated.p1.screenX - baseline.p1.screenX) > 6.0f
        && std::fabs(rotated.p1.screenY - baseline.p1.screenY) > 4.0f
        && std::fabs(rotated.p1.viewDepth - baseline.p1.viewDepth) > 10.0f;
    record(out, counts, projectionChanged ? Status::Pass : Status::Fail,
        "yaw_changes_fighter_projection",
        "baseline_x=" + std::to_string(baseline.p1.screenX)
        + " rotated_x=" + std::to_string(rotated.p1.screenX)
        + " baseline_y=" + std::to_string(baseline.p1.screenY)
        + " rotated_y=" + std::to_string(rotated.p1.screenY)
        + " baseline_view_depth=" + std::to_string(baseline.p1.viewDepth)
        + " rotated_view_depth=" + std::to_string(rotated.p1.viewDepth));

    runtime.setFighterDepth(0, 0.0f);
    runtime.step({}, 54);
    const auto neutral = runtime.snapshot();
    record(out, counts,
        std::fabs(neutral.arenaCameraTargetYawDeg) <= 0.01f
            && std::fabs(neutral.arenaCameraYawDeg) < 1.0f
            ? Status::Pass
            : Status::Fail,
        "neutral_depth_eases_yaw_to_zero",
        "yaw=" + std::to_string(neutral.arenaCameraYawDeg)
        + " target=" + std::to_string(neutral.arenaCameraTargetYawDeg));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaCameraRotationDrawOrder(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-camera-rotation-draw-order")) {
        return exitCode(counts);
    }

    runtime.positionFighters(-120.0f, 120.0f);
    runtime.setFighterLife(1, 0);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, 48.0f);
    runtime.setFighterDepth(1, 48.0f);
    runtime.setArenaCameraRotationEnabled(false);
    const auto flat = runtime.snapshot();
    record(out, counts, flat.arenaDrawOrder == "0,1" ? Status::Pass : Status::Fail,
        "equal_depth_flat_order_stable",
        "draw_order=" + flat.arenaDrawOrder
        + " p1_view_depth=" + std::to_string(flat.p1.viewDepth)
        + " p2_view_depth=" + std::to_string(flat.p2.viewDepth));

    runtime.setArenaCameraRotationEnabled(true);
    runtime.step({}, 36);
    const auto rotated = runtime.snapshot();
    const bool rotatedDepthOrders = rotated.arenaDrawOrder == "1,0"
        && rotated.p1.viewDepth > rotated.p2.viewDepth + 20.0f;
    record(out, counts, rotatedDepthOrders ? Status::Pass : Status::Fail,
        "rotated_view_depth_controls_order",
        "draw_order=" + rotated.arenaDrawOrder
        + " yaw=" + std::to_string(rotated.arenaCameraYawDeg)
        + " p1_view_depth=" + std::to_string(rotated.p1.viewDepth)
        + " p2_view_depth=" + std::to_string(rotated.p2.viewDepth));

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
    const bool walkingAction = after.p2.action == 20 || after.p2.action == 21;
    record(out, counts, alignedCloser && walkingAction ? Status::Pass : Status::Fail,
        "cpu_aligns_depth_toward_target",
        "depth_distance_before=" + std::to_string(depthDistance(before))
        + " depth_distance_after=" + std::to_string(depthDistance(after))
        + " p2_depth_before=" + std::to_string(before.p2.depthZ)
        + " p2_depth_after=" + std::to_string(after.p2.depthZ)
        + " p2_action=" + std::to_string(after.p2.action)
        + " p2_anim_tick=" + std::to_string(after.p2.animTick));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaZModifierSidestep(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-z-modifier-sidestep")) {
        return exitCode(counts);
    }
    runtime.setArenaCameraRotationEnabled(true);

    runtime.positionFighters(-60.0f, 60.0f);
    runtime.setFighterControl(1, false);
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0));
    if (!idle) {
        record(out, counts, Status::Blocked, "sidestep_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setFighterDepth(0, 0.0f);
    const auto neutralBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .depthModifier = true }, 1);
    runtime.step({}, 4);
    runtime.step(SymbolicInput{ .depthModifier = true }, 1);
    runtime.step({}, 2);
    const auto neutralMid = runtime.snapshot();
    runtime.step({}, 10);
    const auto neutralAfter = runtime.snapshot();
    const bool modifierOnlySidestep = neutralAfter.p1.depthZ > neutralBefore.p1.depthZ + 16.0f
        && neutralMid.p1.depthZ > neutralBefore.p1.depthZ + 2.0f
        && neutralAfter.p1.onGround
        && std::fabs(neutralAfter.p1.y - neutralBefore.p1.y) <= 0.5f
        && neutralAfter.p1.stateType != 'A'
        && neutralAfter.p1.stateType != 'C'
        && (neutralMid.p1.action == 20 || neutralMid.p1.action == 21);
    record(out, counts, modifierOnlySidestep && neutralAfter.arenaCameraRotationActive
            ? Status::Pass
            : Status::Fail,
        "double_tap_modifier_sidestep",
        "depth_before=" + std::to_string(neutralBefore.p1.depthZ)
        + " depth_mid=" + std::to_string(neutralMid.p1.depthZ)
        + " depth_after=" + std::to_string(neutralAfter.p1.depthZ)
        + " yaw=" + std::to_string(neutralAfter.arenaCameraYawDeg)
        + " y_after=" + std::to_string(neutralAfter.p1.y)
        + " state_type=" + std::string(1, neutralAfter.p1.stateType)
        + " mid_action=" + std::to_string(neutralMid.p1.action)
        + " mid_anim_tick=" + std::to_string(neutralMid.p1.animTick));

    runtime.positionFighters(-60.0f, 60.0f);
    runtime.setFighterControl(1, false);
    waitForControllableIdle(runtime, 240);
    runtime.setFighterDepth(0, 0.0f);
    const auto upBefore = runtime.snapshot();
    runtime.step(SymbolicInput{ .depthModifier = true }, 1);
    runtime.step({}, 4);
    runtime.step(SymbolicInput{ .up = true, .depthModifier = true }, 1);
    runtime.step({}, 2);
    const auto upMid = runtime.snapshot();
    runtime.step({}, 10);
    const auto upAfter = runtime.snapshot();
    const bool upSidestep = upAfter.p1.depthZ < upBefore.p1.depthZ - 16.0f
        && upMid.p1.depthZ < upBefore.p1.depthZ - 2.0f
        && upAfter.p1.onGround
        && std::fabs(upAfter.p1.y - upBefore.p1.y) <= 0.5f
        && upAfter.p1.stateType != 'A'
        && (upMid.p1.action == 20 || upMid.p1.action == 21);
    record(out, counts, upSidestep && upAfter.arenaCameraRotationActive ? Status::Pass : Status::Fail,
        "double_tap_modifier_up_sidestep",
        "depth_before=" + std::to_string(upBefore.p1.depthZ)
        + " depth_mid=" + std::to_string(upMid.p1.depthZ)
        + " depth_after=" + std::to_string(upAfter.p1.depthZ)
        + " yaw=" + std::to_string(upAfter.arenaCameraYawDeg)
        + " y_after=" + std::to_string(upAfter.p1.y)
        + " state_type=" + std::string(1, upAfter.p1.stateType)
        + " mid_action=" + std::to_string(upMid.p1.action)
        + " mid_anim_tick=" + std::to_string(upMid.p1.animTick));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaEvilKenForwardDashBounds(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-evilken-forward-dash-bounds", "EvilKen")) {
        return exitCode(counts);
    }

    runtime.positionFighters(120.0f, 190.0f);
    runtime.setFighterControl(1, false);
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0));
    if (!idle) {
        record(out, counts, Status::Blocked, "dash_bounds_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    SymbolicInput forward;
    forward.right = true;

    bool sawDashAir = false;
    bool sawGroundedRecovery = false;
    bool allCyclesReturnedIdle = true;
    float maxPositiveY = 0.0f;
    float maxScreenX = runtime.snapshot().p1.screenX;
    float minScreenX = runtime.snapshot().p1.screenX;
    FighterSnapshot finalP1 = runtime.snapshot().p1;

    for (int cycle = 0; cycle < 7; ++cycle) {
        runtime.step(forward, 2);
        runtime.step({}, 2);
        runtime.step(forward, 2);

        bool cycleReturnedIdle = false;
        for (int frame = 0; frame < 90; ++frame) {
            const auto snap = runtime.snapshot();
            finalP1 = snap.p1;
            sawDashAir = sawDashAir || snap.p1.stateNo == 101;
            maxPositiveY = std::max(maxPositiveY, snap.p1.y);
            maxScreenX = std::max(maxScreenX, snap.p1.screenX);
            minScreenX = std::min(minScreenX, snap.p1.screenX);
            if (snap.p1.stateNo == 0 && snap.p1.ctrl && snap.p1.onGround && std::fabs(snap.p1.y) <= 0.5f) {
                sawGroundedRecovery = true;
                cycleReturnedIdle = true;
                break;
            }
            runtime.step({}, 1);
        }

        allCyclesReturnedIdle = allCyclesReturnedIdle && cycleReturnedIdle;
        if (!cycleReturnedIdle) {
            break;
        }
    }

    const bool stayedVerticallyBound = maxPositiveY <= 8.0f
        && finalP1.y <= 8.0f
        && finalP1.stateType != 'A';
    const bool stayedScreenBound = minScreenX >= -64.0f && maxScreenX <= 1024.0f;
    record(out, counts, sawDashAir ? Status::Pass : Status::Fail, "forward_dash_air_state_observed",
        "final_state=" + std::to_string(finalP1.stateNo)
        + " final_y=" + std::to_string(finalP1.y));
    record(out, counts, sawGroundedRecovery ? Status::Pass : Status::Fail, "forward_dash_grounded_recovery_observed",
        "final_state=" + std::to_string(finalP1.stateNo)
        + " final_action=" + std::to_string(finalP1.action)
        + " final_ground=" + std::to_string(finalP1.onGround ? 1 : 0));
    record(out, counts, stayedVerticallyBound ? Status::Pass : Status::Fail, "forward_dash_does_not_fall_through_stage",
        "max_y=" + std::to_string(maxPositiveY)
        + " final_y=" + std::to_string(finalP1.y)
        + " final_state_type=" + std::string(1, finalP1.stateType)
        + " final_ground=" + std::to_string(finalP1.onGround ? 1 : 0));
    record(out, counts, stayedScreenBound ? Status::Pass : Status::Fail, "forward_dash_stays_screen_bound",
        "min_screen_x=" + std::to_string(minScreenX)
        + " max_screen_x=" + std::to_string(maxScreenX));
    record(out, counts, allCyclesReturnedIdle ? Status::Pass : Status::Fail, "repeated_forward_dash_recovers_idle",
        "final_state=" + std::to_string(finalP1.stateNo)
        + " final_ctrl=" + std::to_string(finalP1.ctrl ? 1 : 0)
        + " final_y=" + std::to_string(finalP1.y));

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

int runArenaOpenBorScrollStage(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-openbor-scroll-stage", "kfm", "OpenBOR Scroll")) {
        return exitCode(counts);
    }

    runtime.positionFighters(100.0f, 40.0f);
    runtime.setFighterControl(1, false);
    runtime.forceFighterLiedown(1, 9999);
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
        + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0));
    if (!idle) {
        record(out, counts, Status::Blocked, "openbor_scroll_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    const auto start = runtime.snapshot();
    runtime.step(SymbolicInput{ .right = true }, 180);
    const auto forward = runtime.snapshot();
    const bool scrolledForward = forward.cameraX > start.cameraX + 30.0f
        && forward.p1.x > start.p1.x + 60.0f;
    record(out, counts, scrolledForward ? Status::Pass : Status::Fail, "openbor_camera_scrolls_forward",
        "camera_before=" + std::to_string(start.cameraX)
        + " camera_after=" + std::to_string(forward.cameraX)
        + " p1_x_before=" + std::to_string(start.p1.x)
        + " p1_x_after=" + std::to_string(forward.p1.x));

    runtime.step(SymbolicInput{ .left = true }, 90);
    const auto back = runtime.snapshot();
    record(out, counts, back.cameraX >= forward.cameraX - 0.5f ? Status::Pass : Status::Fail,
        "openbor_camera_does_not_scroll_backward",
        "camera_forward=" + std::to_string(forward.cameraX)
        + " camera_after_left=" + std::to_string(back.cameraX)
        + " p1_x_after_left=" + std::to_string(back.p1.x));

    runtime.step(SymbolicInput{ .right = true }, 1400);
    const auto end = runtime.snapshot();
    record(out, counts, end.cameraX >= 1750.0f && end.cameraX <= 1760.5f ? Status::Pass : Status::Fail,
        "openbor_camera_clamps_at_stage_end",
        "camera_forward=" + std::to_string(forward.cameraX)
        + " camera_end=" + std::to_string(end.cameraX));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

int runArenaEvilRyuAirSpecialContactLanding(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!setupArenaFight(runtime, out, counts, "arena-evilryu-air-special-contact-landing", "EvilRyu")) {
        return exitCode(counts);
    }

    runtime.setArenaZAxisEnabled(false);
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

    const auto final = runtime.snapshot();
    const bool leftAirSpecial = last.stateNo != 4054 && last.stateNo != 4055;
    record(out, counts, saw4055 ? Status::Pass : Status::Fail, "arena_air_chain_reaches_descent_4055",
        "final_state=" + std::to_string(last.stateNo)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_y=" + std::to_string(last.y)
        + " global_pause=" + std::to_string(final.globalPauseTicks));
    record(out, counts, sawContact ? Status::Pass : Status::Fail, "arena_air_special_contact_observed",
        "peak_helpers=" + std::to_string(peakHelpers)
        + " hit=\"" + hitText + "\"");
    record(out, counts, sawLanding ? Status::Pass : Status::Fail, "arena_air_special_lands_via_4044",
        "final_state=" + std::to_string(last.stateNo)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_y=" + std::to_string(last.y));
    record(out, counts, sawIdle ? Status::Pass : Status::Fail, "arena_air_special_recovers_idle",
        "final_state=" + std::to_string(last.stateNo)
        + " final_action=" + std::to_string(last.action)
        + " final_time=" + std::to_string(last.stateTime)
        + " final_x=" + std::to_string(last.x)
        + " final_y=" + std::to_string(last.y));
    record(out, counts, leftAirSpecial && !sawStale4055 ? Status::Pass : Status::Fail, "arena_air_special_not_stuck_in_4054_4055",
        "state=" + std::to_string(last.stateNo)
        + " time=" + std::to_string(last.stateTime)
        + " stale4055=" + std::to_string(sawStale4055 ? 1 : 0)
        + " helpers=" + std::to_string(final.activeHelpers));

    runtime.positionFighters(-36.0f, 28.0f);
    runtime.forceFighterState(0, 4050);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -36.0f, -68.0f);
    runtime.setFighterPosition(1, 28.0f, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.step({}, 1);
    runtime.setFighterVar(0, 28, 3);
    runtime.setFighterVar(0, 46, 100);
    runtime.setFighterMoveContact(0, true, false);
    runtime.step({}, 1);
    const auto forcedContact = runtime.snapshot();
    const int forcedVar28 = runtime.fighterVar(0, 28);
    const int forcedVar46 = runtime.fighterVar(0, 46);
    record(out, counts, forcedContact.p1.stateNo == 4054 ? Status::Pass : Status::Fail, "arena_parent_4050_forced_movecontact_branches",
        "state=" + std::to_string(forcedContact.p1.stateNo)
        + " time=" + std::to_string(forcedContact.p1.stateTime)
        + " var28=" + std::to_string(forcedVar28)
        + " var46=" + std::to_string(forcedVar46)
        + " move_contact=" + std::to_string(forcedContact.p1.moveContact ? 1 : 0)
        + " move_hit=" + std::to_string(forcedContact.p1.moveHit ? 1 : 0));

    runtime.positionFighters(-36.0f, 28.0f);
    runtime.forceFighterState(0, 4050);
    runtime.forceFighterState(1, 0);
    runtime.setFighterPosition(0, -36.0f, -68.0f);
    runtime.setFighterPosition(1, 28.0f, 0.0f);
    runtime.setFighterPower(0, 3000);
    runtime.setFighterVar(0, 28, 3);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);

    bool parentSaw4054 = false;
    bool parentSaw4055 = false;
    bool parentSawLanding = false;
    bool parentSawIdle = false;
    bool parentSawContact = false;
    bool parentStale = false;
    int parentContactFrame = -1;
    int parentContactState = 0;
    int parentContactTime = 0;
    int parentContactHitPause = 0;
    int parentContactVar28 = 0;
    int parentContactVar46 = 0;
    float parentContactY = 0.0f;
    int parentFreeContactFrame = -1;
    int parentFreeContactTime = 0;
    FighterSnapshot parentLast = runtime.snapshot().p1;
    std::string parentHitText;
    for (int i = 0; i < 360; ++i) {
        runtime.setFighterVar(0, 28, 3);
        runtime.step({}, 1);
        const auto snapshot = runtime.snapshot();
        parentLast = snapshot.p1;
        parentSaw4054 = parentSaw4054 || snapshot.p1.stateNo == 4054;
        parentSaw4055 = parentSaw4055 || snapshot.p1.stateNo == 4055;
        parentSawLanding = parentSawLanding || snapshot.p1.stateNo == 4044;
        parentSawIdle = parentSawIdle || (snapshot.p1.stateNo == 0 && snapshot.p1.ctrl && snapshot.p1.onGround);
        parentSawContact = parentSawContact
            || snapshot.p1.moveContact
            || snapshot.p1.moveHit
            || snapshot.p1.moveGuarded
            || snapshot.lastHitText.find("P1 hit") != std::string::npos
            || snapshot.lastHitText.find("P1 guard") != std::string::npos;
        if (parentContactFrame < 0 && (snapshot.p1.moveContact || snapshot.p1.moveHit || snapshot.p1.moveGuarded)) {
            parentContactFrame = i;
            parentContactState = snapshot.p1.stateNo;
            parentContactTime = snapshot.p1.stateTime;
            parentContactHitPause = snapshot.p1.hitPauseTicks;
            parentContactVar28 = runtime.fighterVar(0, 28);
            parentContactVar46 = runtime.fighterVar(0, 46);
            parentContactY = snapshot.p1.y;
        }
        if (parentFreeContactFrame < 0
            && snapshot.p1.stateNo == 4050
            && snapshot.p1.moveContact
            && snapshot.p1.hitPauseTicks <= 0) {
            parentFreeContactFrame = i;
            parentFreeContactTime = snapshot.p1.stateTime;
        }
        if (!snapshot.lastHitText.empty()) {
            parentHitText = snapshot.lastHitText;
        }
        parentStale = parentStale
            || (snapshot.p1.stateNo == 4050 && snapshot.p1.stateTime > 180)
            || (snapshot.p1.stateNo == 4054 && snapshot.p1.stateTime > 120)
            || (snapshot.p1.stateNo == 4055 && snapshot.p1.stateTime > 120);
        if (parentSawIdle) {
            break;
        }
    }

    if (parentSawIdle) {
        for (int i = 0; i < 45; ++i) {
            runtime.step({}, 1);
        }
        parentLast = runtime.snapshot().p1;
    }
    const auto parentFinal = runtime.snapshot();
    const int parentFinalVar28 = runtime.fighterVar(0, 28);
    const int parentFinalVar46 = runtime.fighterVar(0, 46);
    const bool parentLeftAirSpecial = parentLast.stateNo != 4050 && parentLast.stateNo != 4054 && parentLast.stateNo != 4055;
    record(out, counts, parentSawContact ? Status::Pass : Status::Fail, "arena_parent_4050_contact_observed",
        "frame=" + std::to_string(parentContactFrame)
        + " state=" + std::to_string(parentContactState)
        + " time=" + std::to_string(parentContactTime)
        + " hitpause=" + std::to_string(parentContactHitPause)
        + " var28=" + std::to_string(parentContactVar28)
        + " var46=" + std::to_string(parentContactVar46)
        + " y=" + std::to_string(parentContactY)
        + " free_contact_frame=" + std::to_string(parentFreeContactFrame)
        + " free_contact_time=" + std::to_string(parentFreeContactTime)
        + " hit=\"" + parentHitText + "\"");
    record(out, counts, parentSaw4054 && (parentSaw4055 || parentSawLanding || parentSawIdle) ? Status::Pass : Status::Fail, "arena_parent_4050_branches_to_descent",
        "saw4054=" + std::to_string(parentSaw4054 ? 1 : 0)
        + " saw4055=" + std::to_string(parentSaw4055 ? 1 : 0)
        + " final_state=" + std::to_string(parentLast.stateNo)
        + " final_time=" + std::to_string(parentLast.stateTime)
        + " final_var28=" + std::to_string(parentFinalVar28)
        + " final_var46=" + std::to_string(parentFinalVar46));
    record(out, counts, parentSawLanding && parentSawIdle ? Status::Pass : Status::Fail, "arena_parent_4050_recovers_idle",
        "landed=" + std::to_string(parentSawLanding ? 1 : 0)
        + " final_state=" + std::to_string(parentLast.stateNo)
        + " final_x=" + std::to_string(parentLast.x)
        + " final_y=" + std::to_string(parentLast.y)
        + " global_pause=" + std::to_string(parentFinal.globalPauseTicks));
    record(out, counts, parentLeftAirSpecial && !parentStale ? Status::Pass : Status::Fail, "arena_parent_4050_not_stuck",
        "state=" + std::to_string(parentLast.stateNo)
        + " time=" + std::to_string(parentLast.stateTime)
        + " stale=" + std::to_string(parentStale ? 1 : 0)
        + " helpers=" + std::to_string(parentFinal.activeHelpers));

    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
