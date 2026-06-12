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

bool waitForFight(RuntimeProbe& runtime, int maxFrames) {
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight)) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == static_cast<int>(MatchPhase::Fight);
}

} // namespace

int runKfmDownHitProfile(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("kfm", "Mountainside", ScenarioMode::Training, out)) {
        record(out, counts, Status::Blocked, "setup", "KFM/Mountainside training setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "kfm-down-hit-profile");

    const bool settled = waitForControllableIdle(runtime, 360);
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!settled) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-6.0f, 6.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterLiedown(1, 120);
    const auto liedown = runtime.snapshot().p2;
    const bool p2LiedownReady = liedown.stateNo == 5110 && liedown.stateType == 'L';
    record(out, counts, p2LiedownReady ? Status::Pass : Status::Fail, "p2_forced_liedown",
        "state=" + std::to_string(liedown.stateNo)
        + " action=" + std::to_string(liedown.action)
        + " type=" + std::string(1, liedown.stateType));

    const int p2LifeBefore = liedown.life;
    runtime.forceFighterState(0, 430);
    bool sawDownHit = false;
    FighterSnapshot hitTarget;
    std::string lastHitText;
    for (int i = 0; i < 45; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        const bool isKfmDownHit = snap.lastHitText.find("P1 hit 430#") != std::string::npos;
        if (isKfmDownHit) {
            lastHitText = snap.lastHitText;
        }
        if (isKfmDownHit && snap.p2.life < p2LifeBefore && snap.p2.moveType == 'H') {
            sawDownHit = true;
            hitTarget = snap.p2;
            break;
        }
    }

    record(out, counts, sawDownHit ? Status::Pass : Status::Fail, "downed_target_hit",
        "last_hit=\"" + lastHitText + "\" state=" + std::to_string(hitTarget.stateNo)
        + " action=" + std::to_string(hitTarget.action));
    const bool usedDownVelocity = sawDownHit
        && std::abs(hitTarget.hitDownVelocityX - 5.0f) <= 0.05f
        && std::abs(hitTarget.hitDownVelocityY) <= 0.05f;
    record(out, counts, usedDownVelocity ? Status::Pass : Status::Fail, "down_velocity_applied",
        "down_vx=" + std::to_string(hitTarget.hitDownVelocityX)
        + " down_vy=" + std::to_string(hitTarget.hitDownVelocityY)
        + " runtime_vx=" + std::to_string(hitTarget.vx)
        + " runtime_vy=" + std::to_string(hitTarget.vy));
    const bool usedDownHitTime = sawDownHit && hitTarget.hitStunTicks >= 20;
    record(out, counts, usedDownHitTime ? Status::Pass : Status::Fail, "down_hittime_applied",
        "hitstun=" + std::to_string(hitTarget.hitStunTicks)
        + " hitpause=" + std::to_string(hitTarget.hitPauseTicks));
    const bool stayedGroundedNoBounce = sawDownHit
        && hitTarget.onGround
        && std::abs(hitTarget.hitDownVelocityY) <= 0.05f
        && (hitTarget.stateNo == 5080 || hitTarget.action == 5080 || hitTarget.stateType == 'L');
    record(out, counts, stayedGroundedNoBounce ? Status::Pass : Status::Fail, "down_bounce_respected",
        "state=" + std::to_string(hitTarget.stateNo)
        + " action=" + std::to_string(hitTarget.action)
        + " on_ground=" + std::to_string(hitTarget.onGround ? 1 : 0));

    summary(out, counts);
    return exitCode(counts);
}

int runKfmGuardRecovery(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "Mountainside", ScenarioMode::SinglePlayer, out)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu/Mountainside single-player setup failed");
        summary(out, counts);
        return 2;
    }
    header(out, runtime, "kfm-guard-recovery");

    const bool fightStarted = waitForFight(runtime, 420);
    record(out, counts, fightStarted ? Status::Pass : Status::Fail, "fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase));
    const bool settled = fightStarted && waitForControllableIdle(runtime, 420);
    record(out, counts, settled ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo));
    if (!settled) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.positionFighters(-30.0f, 30.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 130);
    runtime.setFighterControl(1, true);
    const auto forced = runtime.snapshot().p2;
    record(out, counts, forced.stateNo == 130 ? Status::Pass : Status::Fail, "p2_forced_stand_guard",
        "state=" + std::to_string(forced.stateNo)
        + " action=" + std::to_string(forced.action)
        + " ctrl=" + std::to_string(forced.ctrl ? 1 : 0));

    bool sawGuardEnd = false;
    FighterSnapshot recovered = forced;
    for (int i = 0; i < 120; ++i) {
        runtime.step({}, 1);
        const auto p2 = runtime.snapshot().p2;
        sawGuardEnd = sawGuardEnd || p2.stateNo == 140;
        if (p2.stateNo == 0 && p2.ctrl && p2.moveType == 'I') {
            recovered = p2;
            break;
        }
        recovered = p2;
    }

    record(out, counts, sawGuardEnd ? Status::Pass : Status::Fail, "guard_end_state_seen",
        "final_state=" + std::to_string(recovered.stateNo)
        + " action=" + std::to_string(recovered.action)
        + " time=" + std::to_string(recovered.stateTime));
    record(out, counts, recovered.stateNo == 0 && recovered.ctrl && recovered.moveType == 'I' ? Status::Pass : Status::Fail,
        "guard_idle_recovers_to_neutral",
        "state=" + std::to_string(recovered.stateNo)
        + " action=" + std::to_string(recovered.action)
        + " ctrl=" + std::to_string(recovered.ctrl ? 1 : 0)
        + " move_type=" + std::string(1, recovered.moveType));

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
