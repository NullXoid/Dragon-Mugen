#include "VerificationScenario.h"

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

bool nearly(float actual, float expected, float tolerance) {
    return std::fabs(actual - expected) <= tolerance;
}

std::string movementDetail(const FighterSnapshot& before, const FighterSnapshot& after) {
    return "state=" + std::to_string(after.stateNo)
        + " action=" + std::to_string(after.action)
        + " x_delta=" + std::to_string(after.x - before.x)
        + " y=" + std::to_string(after.y)
        + " vx=" + std::to_string(after.vx)
        + " vy=" + std::to_string(after.vy)
        + " ground=" + std::to_string(after.onGround ? 1 : 0);
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

bool resetToIdle(RuntimeProbe& runtime) {
    if (!waitForControllableIdle(runtime, 240)) {
        return false;
    }
    runtime.positionFighters(-80.0f, 80.0f);
    runtime.forceFighterState(0, 0);
    runtime.setFighterControl(0, true);
    runtime.step({}, 3);
    return waitForControllableIdle(runtime, 120);
}

SymbolicInput direction(bool left, bool right, bool up, bool down) {
    SymbolicInput input;
    input.left = left;
    input.right = right;
    input.up = up;
    input.down = down;
    return input;
}

void recordBlockedReset(std::ostream& out, Counts& counts, std::string_view name, const RuntimeProbe& runtime) {
    const auto p1 = runtime.snapshot().p1;
    record(out, counts, Status::Blocked, name,
        "could not reset to controllable idle; state=" + std::to_string(p1.stateNo)
        + " action=" + std::to_string(p1.action)
        + " ground=" + std::to_string(p1.onGround ? 1 : 0));
}

} // namespace

int runKfmMovementDirectionAudit(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "kfm-movement-direction-audit");
    out << "reference: KFM CNS + stock MUGEN common1 movement; IKEMEN Go keeps the same 2D states\n";

    if (!waitForControllableIdle(runtime, 360)) {
        record(out, counts, Status::Blocked, "initial_idle", "fighter never reached controllable idle");
        summary(out, counts);
        return exitCode(counts);
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "neutral_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step({}, 8);
        const auto after = runtime.snapshot().p1;
        const bool ok = after.stateNo == 0 && after.onGround && nearly(after.x, before.x, 0.05f) && nearly(after.vx, 0.0f, 0.05f);
        record(out, counts, ok ? Status::Pass : Status::Fail, "neutral_idle_holds_position", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "right_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(false, true, false, false), 5);
        const auto after = runtime.snapshot().p1;
        const bool ok = after.stateNo == 20 && after.action == 20 && nearly(after.vx, 2.4f, 0.1f) && after.x > before.x + 8.0f;
        record(out, counts, ok ? Status::Pass : Status::Fail, "right_walk_matches_fwd", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "left_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(true, false, false, false), 5);
        const auto after = runtime.snapshot().p1;
        const bool ok = after.stateNo == 20 && after.action == 21 && nearly(after.vx, -2.2f, 0.1f) && after.x < before.x - 7.0f;
        record(out, counts, ok ? Status::Pass : Status::Fail, "left_walk_matches_back", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "down_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(false, false, false, true), 8);
        const auto after = runtime.snapshot().p1;
        const bool crouching = after.stateNo == 10 || after.stateNo == 11 || after.stateNo == 12;
        const bool ok = crouching && after.onGround && nearly(after.x, before.x, 0.2f) && nearly(after.vx, 0.0f, 0.05f);
        record(out, counts, ok ? Status::Pass : Status::Fail, "down_crouch_blocks_movement", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "down_right_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(false, true, false, true), 8);
        const auto after = runtime.snapshot().p1;
        const bool crouching = after.stateNo == 10 || after.stateNo == 11 || after.stateNo == 12;
        const bool ok = crouching && after.onGround && nearly(after.x, before.x, 0.2f) && nearly(after.vx, 0.0f, 0.05f);
        record(out, counts, ok ? Status::Pass : Status::Fail, "down_right_crouch_no_walk", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "down_left_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(true, false, false, true), 8);
        const auto after = runtime.snapshot().p1;
        const bool crouching = after.stateNo == 10 || after.stateNo == 11 || after.stateNo == 12;
        const bool ok = crouching && after.onGround && nearly(after.x, before.x, 0.2f) && nearly(after.vx, 0.0f, 0.05f);
        record(out, counts, ok ? Status::Pass : Status::Fail, "down_left_crouch_no_walk", movementDetail(before, after));
    }

    constexpr float kKfmJumpYAfterOnePhysicsTick = -8.4f + 0.44f;
    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "up_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(false, false, true, false), 1);
        const auto after = runtime.snapshot().p1;
        const bool ok = !after.onGround && after.action == 41 && nearly(after.vx, 0.0f, 0.05f)
            && nearly(after.vy, kKfmJumpYAfterOnePhysicsTick, 0.15f) && after.y < before.y - 7.5f;
        record(out, counts, ok ? Status::Pass : Status::Fail, "up_jump_matches_neutral", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "up_right_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(false, true, true, false), 1);
        const auto after = runtime.snapshot().p1;
        const bool ok = !after.onGround && after.action == 42 && nearly(after.vx, 2.5f, 0.1f)
            && nearly(after.vy, kKfmJumpYAfterOnePhysicsTick, 0.15f) && after.x > before.x + 2.0f;
        record(out, counts, ok ? Status::Pass : Status::Fail, "up_right_jump_matches_fwd", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "up_left_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        runtime.step(direction(true, false, true, false), 1);
        const auto after = runtime.snapshot().p1;
        const bool ok = !after.onGround && after.action == 43 && nearly(after.vx, -2.55f, 0.1f)
            && nearly(after.vy, kKfmJumpYAfterOnePhysicsTick, 0.15f) && after.x < before.x - 2.0f;
        record(out, counts, ok ? Status::Pass : Status::Fail, "up_left_jump_matches_back", movementDetail(before, after));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "buffered_jump_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        const auto up = direction(false, false, true, false);
        runtime.step(up, 1);

        bool leftGround = false;
        bool tappedBeforeLanding = false;
        bool landedAfterTap = false;
        bool sawBufferedJump = false;
        FighterSnapshot tapFrame;
        FighterSnapshot bufferedJumpFrame;
        FighterSnapshot after;
        for (int i = 0; i < 180; ++i) {
            const auto snap = runtime.snapshot().p1;
            leftGround = leftGround || !snap.onGround;
            if (leftGround
                && !tappedBeforeLanding
                && !snap.onGround
                && snap.vy > 0.0f
                && snap.y > -18.0f) {
                runtime.step(up, 1);
                tapFrame = runtime.snapshot().p1;
                runtime.step({}, 1);
                tappedBeforeLanding = true;
                continue;
            }
            if (tappedBeforeLanding && snap.onGround && snap.y >= -0.05f) {
                landedAfterTap = true;
            }
            if (landedAfterTap && !snap.onGround && snap.vy < -1.0f) {
                sawBufferedJump = true;
                bufferedJumpFrame = snap;
                break;
            }
            runtime.step({}, 1);
            after = runtime.snapshot().p1;
        }
        record(out, counts, sawBufferedJump ? Status::Pass : Status::Fail, "early_up_buffers_next_jump",
            "before_state=" + std::to_string(before.stateNo)
            + " tapped=" + std::to_string(tappedBeforeLanding ? 1 : 0)
            + " tap_y=" + std::to_string(tapFrame.y)
            + " tap_vy=" + std::to_string(tapFrame.vy)
            + " landed_after_tap=" + std::to_string(landedAfterTap ? 1 : 0)
            + " buffered_state=" + std::to_string(bufferedJumpFrame.stateNo)
            + " buffered_action=" + std::to_string(bufferedJumpFrame.action)
            + " buffered_vy=" + std::to_string(bufferedJumpFrame.vy)
            + " final_state=" + std::to_string(after.stateNo)
            + " final_ground=" + std::to_string(after.onGround ? 1 : 0));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "ff_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        const auto forward = direction(false, true, false, false);
        runtime.step(forward, 1);
        runtime.step({}, 1);
        runtime.step(forward, 1);
        bool sawRun = false;
        bool sawRunVelocity = false;
        for (int i = 0; i < 30; ++i) {
            const auto p1 = runtime.snapshot().p1;
            sawRun = sawRun || p1.stateNo == 100;
            sawRunVelocity = sawRunVelocity || (p1.stateNo == 100 && nearly(p1.vx, 4.6f, 0.1f));
            runtime.step(forward, 1);
        }
        const auto after = runtime.snapshot().p1;
        const bool ok = sawRun && sawRunVelocity && after.x > before.x + 20.0f;
        record(out, counts, ok ? Status::Pass : Status::Fail, "forward_forward_run_matches_common",
            movementDetail(before, after) + " saw_run=" + std::to_string(sawRun ? 1 : 0)
            + " saw_run_velocity=" + std::to_string(sawRunVelocity ? 1 : 0));
    }

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "bb_reset", runtime);
    } else {
        const auto before = runtime.snapshot().p1;
        const auto back = direction(true, false, false, false);
        runtime.step(back, 1);
        runtime.step({}, 1);
        runtime.step(back, 1);
        bool sawHop = false;
        bool sawHopVelocity = false;
        for (int i = 0; i < 18; ++i) {
            const auto p1 = runtime.snapshot().p1;
            sawHop = sawHop || p1.stateNo == 105;
            sawHopVelocity = sawHopVelocity || (p1.stateNo == 105 && nearly(p1.vx, -4.5f, 0.1f) && p1.vy < -2.0f);
            runtime.step(back, 1);
        }
        const auto after = runtime.snapshot().p1;
        const bool ok = sawHop && sawHopVelocity && after.x < before.x - 10.0f;
        record(out, counts, ok ? Status::Pass : Status::Fail, "back_back_hop_matches_common",
            movementDetail(before, after) + " saw_hop=" + std::to_string(sawHop ? 1 : 0)
            + " saw_hop_velocity=" + std::to_string(sawHopVelocity ? 1 : 0));
    }

    summary(out, counts);
    return exitCode(counts);
}

int runEvilRyuHighJumpMovementAudit(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "evilryu-high-jump");
    out << "reference: Evil Ryu high_jump command enters state 41 before state 50 super-jump launch\n";

    if (!resetToIdle(runtime)) {
        recordBlockedReset(out, counts, "high_jump_reset", runtime);
        summary(out, counts);
        return exitCode(counts);
    }

    const auto before = runtime.snapshot().p1;
    runtime.step(direction(false, false, false, true), 1);
    runtime.step({}, 1);

    bool sawHighJumpState = false;
    bool sawAirLaunchState = false;
    FighterSnapshot highJumpState;
    FighterSnapshot airLaunchState;

    for (int i = 0; i < 3; ++i) {
        runtime.step(direction(false, false, true, false), 1);
        const auto p1 = runtime.snapshot().p1;
        if (!sawHighJumpState && p1.stateNo == 41) {
            sawHighJumpState = true;
            highJumpState = p1;
        }
    }

    for (int i = 0; i < 80; ++i) {
        const auto p1 = runtime.snapshot().p1;
        if (!sawHighJumpState && p1.stateNo == 41) {
            sawHighJumpState = true;
            highJumpState = p1;
        }
        if (!sawAirLaunchState && p1.stateNo == 50 && !p1.onGround && p1.vy < -9.7f) {
            sawAirLaunchState = true;
            airLaunchState = p1;
        }
        runtime.step({}, 1);
    }

    const auto after = runtime.snapshot().p1;
    record(out, counts, sawHighJumpState ? Status::Pass : Status::Fail, "du_enters_high_jump_state_41",
        "before_state=" + std::to_string(before.stateNo)
        + " high_state=" + std::to_string(highJumpState.stateNo)
        + " high_action=" + std::to_string(highJumpState.action)
        + " after_state=" + std::to_string(after.stateNo)
        + " after_action=" + std::to_string(after.action));
    record(out, counts, sawAirLaunchState ? Status::Pass : Status::Fail, "high_jump_uses_super_jump_velocity",
        "launch_state=" + std::to_string(airLaunchState.stateNo)
        + " launch_y=" + std::to_string(airLaunchState.y)
        + " launch_vy=" + std::to_string(airLaunchState.vy)
        + " final_state=" + std::to_string(after.stateNo)
        + " final_y=" + std::to_string(after.y)
        + " final_vy=" + std::to_string(after.vy));

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
