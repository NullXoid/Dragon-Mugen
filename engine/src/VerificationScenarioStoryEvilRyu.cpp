#include "VerificationScenario.h"

#include "AppTypes.h"

#include <algorithm>
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

} // namespace

int runStoryEvilRyuSuperRecovery(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    if (!runtime.setup("EvilRyu", "TMNT OpenBOR Street", ScenarioMode::Story, out, 1)) {
        record(out, counts, Status::Blocked, "setup", "Evil Ryu Story setup failed");
        summary(out, counts);
        return exitCode(counts);
    }
    header(out, runtime, "story-evilryu-super-recovery");

    const bool activeFight = waitForActiveFight(runtime, 420);
    record(out, counts, activeFight ? Status::Pass : Status::Fail,
        "story_fight_phase_ready",
        "match_phase=" + std::to_string(runtime.snapshot().matchPhase));
    if (!activeFight) {
        summary(out, counts);
        return exitCode(counts);
    }

    runtime.setArenaCpuFrozen(true);
    runtime.setFighterPosition(0, 220.0f, 0.0f);
    runtime.setFighterPosition(1, 315.0f, 0.0f);
    runtime.setFighterDepth(0, 0.0f);
    runtime.setFighterDepth(1, 0.0f);
    runtime.setFighterControl(0, false);
    runtime.setFighterControl(1, false);
    runtime.setFighterPower(0, 3000);
    runtime.setFighterVar(0, 28, 0);
    runtime.setFighterLife(1, 1000);
    runtime.forceFighterState(0, 3885);

    bool sawSuperPause = false;
    bool sawPauseClear = false;
    bool sawHelper = false;
    bool sawHit = false;
    bool recovered = false;
    int maxPause = 0;
    int maxHelpers = 0;
    int longestPoseStall = 0;
    int poseStall = 0;
    int previousState = -1;
    int previousAnimTick = -1;
    int previousStateTime = -1;
    FighterSnapshot finalP1;
    FighterSnapshot finalP2;
    std::string lastHitText;

    for (int frame = 0; frame < 520; ++frame) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        finalP1 = snap.p1;
        finalP2 = snap.p2;
        maxPause = std::max(maxPause, snap.globalPauseTicks);
        maxHelpers = std::max(maxHelpers, snap.activeHelpers);
        sawSuperPause = sawSuperPause || (snap.globalPauseIsSuper && snap.globalPauseTicks > 0);
        sawPauseClear = sawPauseClear || (sawSuperPause && snap.globalPauseTicks == 0);
        sawHelper = sawHelper || snap.activeHelpers > 0;
        sawHit = sawHit || snap.p1.moveHit || snap.comboHits > 0 || snap.lastHitText.find("P1 hit") != std::string::npos;
        if (!snap.lastHitText.empty()) {
            lastHitText = snap.lastHitText;
        }

        if (snap.p1.stateNo == previousState
            && snap.p1.animTick == previousAnimTick
            && snap.p1.stateTime == previousStateTime
            && snap.globalPauseTicks == 0
            && snap.p1.hitPauseTicks == 0) {
            ++poseStall;
        } else {
            poseStall = 0;
            previousState = snap.p1.stateNo;
            previousAnimTick = snap.p1.animTick;
            previousStateTime = snap.p1.stateTime;
        }
        longestPoseStall = std::max(longestPoseStall, poseStall);

        recovered = snap.globalPauseTicks == 0
            && snap.p1.stateNo == 0
            && snap.p1.moveType == 'I'
            && snap.p1.ctrl
            && snap.activeHelpers <= 1;
        if (recovered) {
            break;
        }
    }

    record(out, counts, sawSuperPause ? Status::Pass : Status::Fail,
        "story_superpause_observed",
        "max_pause=" + std::to_string(maxPause));
    record(out, counts, sawPauseClear ? Status::Pass : Status::Fail,
        "story_superpause_clears",
        "final_pause=" + std::to_string(runtime.snapshot().globalPauseTicks));
    record(out, counts, sawHelper ? Status::Pass : Status::Fail,
        "story_super_helper_spawns",
        "max_helpers=" + std::to_string(maxHelpers));
    record(out, counts, sawHit ? Status::Pass : Status::Fail,
        "story_super_can_hit_enemy",
        "last_hit=\"" + lastHitText + "\"");
    record(out, counts, longestPoseStall < 90 ? Status::Pass : Status::Fail,
        "story_super_no_post_pause_pose_stall",
        "longest_stall=" + std::to_string(longestPoseStall)
            + " final_state=" + std::to_string(finalP1.stateNo)
            + " final_time=" + std::to_string(finalP1.stateTime));
    record(out, counts, recovered ? Status::Pass : Status::Fail,
        "story_super_recovers_gameplay",
        "final_p1_state=" + std::to_string(finalP1.stateNo)
            + " final_p1_time=" + std::to_string(finalP1.stateTime)
            + " final_p2_state=" + std::to_string(finalP2.stateNo));

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
