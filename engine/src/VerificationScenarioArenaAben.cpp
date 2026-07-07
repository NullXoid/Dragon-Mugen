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

} // namespace

int runArenaZAbenWalkAnimation(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("A.Ben", "TMNT OpenBOR Street", ScenarioMode::Arena, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Arena setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    header(out, runtime, "arena-z-aben-walk-animation");

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail, "arena_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase)
        + " timer_ticks=" + std::to_string(runtime.snapshot().matchTimerTicks));
    if (!activeFight) {
        record(out, counts, Status::Blocked, "arena_checks", "Arena fight phase was not active");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setArenaCameraRotationEnabled(true);
    runtime.positionFighters(-120.0f, 160.0f);
    runtime.setFighterControl(1, false);
    const bool idle = waitForControllableIdle(runtime, 240);
    record(out, counts, idle ? Status::Pass : Status::Fail, "controllable_idle_ready",
        "state=" + std::to_string(runtime.snapshot().p1.stateNo)
            + " ctrl=" + std::to_string(runtime.snapshot().p1.ctrl ? 1 : 0)
            + " p1_id=" + runtime.snapshot().p1CharacterId
            + " p1_name=" + runtime.snapshot().p1CharacterName);
    if (!idle) {
        record(out, counts, Status::Blocked, "aben_depth_walk_checks", "controllable idle gate failed");
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    const auto before = runtime.snapshot();
    runtime.step(SymbolicInput{ .down = true, .depthModifier = true }, 36);
    const auto after = runtime.snapshot();
    const bool loadedAben = after.p1CharacterId == "A.Ben" || after.p1CharacterName.find("A.Ben") != std::string::npos;
    const bool movedDepth = after.p1.depthZ > before.p1.depthZ + 8.0f;
    const bool stayedGrounded = after.p1.onGround
        && std::fabs(after.p1.y - before.p1.y) <= 0.5f
        && after.p1.stateType != 'C';
    const bool walkAction = after.p1.action == 20 || after.p1.action == 21;
    const bool walkTickAdvanced = after.p1.animTick > before.p1.animTick;
    record(out, counts, loadedAben ? Status::Pass : Status::Fail,
        "aben_loaded_for_depth_walk",
        "p1_id=" + after.p1CharacterId + " p1_name=" + after.p1CharacterName);
    record(out, counts, movedDepth && stayedGrounded && walkAction && walkTickAdvanced ? Status::Pass : Status::Fail,
        "aben_depth_walk_animates",
        "depth_before=" + std::to_string(before.p1.depthZ)
            + " depth_after=" + std::to_string(after.p1.depthZ)
            + " state=" + std::to_string(after.p1.stateNo)
            + " action=" + std::to_string(after.p1.action)
            + " anim_tick_before=" + std::to_string(before.p1.animTick)
            + " anim_tick_after=" + std::to_string(after.p1.animTick)
            + " state_type=" + std::string(1, after.p1.stateType));

    runtime.step(SymbolicInput{ .up = true, .depthModifier = true }, 36);
    const auto reverse = runtime.snapshot();
    const bool reversedDepth = reverse.p1.depthZ < after.p1.depthZ - 8.0f;
    const bool reverseWalkTickAdvanced = reverse.p1.animTick > after.p1.animTick;
    record(out, counts, reversedDepth && reverseWalkTickAdvanced ? Status::Pass : Status::Fail,
        "aben_depth_walk_animates_reverse",
        "depth_after_down=" + std::to_string(after.p1.depthZ)
            + " depth_after_up=" + std::to_string(reverse.p1.depthZ)
            + " anim_tick_down=" + std::to_string(after.p1.animTick)
            + " anim_tick_up=" + std::to_string(reverse.p1.animTick)
            + " action=" + std::to_string(reverse.p1.action));

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
