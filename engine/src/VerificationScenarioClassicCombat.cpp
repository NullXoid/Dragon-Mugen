#include "VerificationScenario.h"

#include "AppTypes.h"

#include <array>
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

struct CombatCase {
    std::string_view characterId;
    std::string_view label;
    int guardState = 0;
    int fallState = 0;
    int koState = 0;
    bool crouchGuard = false;
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

bool waitForMatchPhase(RuntimeProbe& runtime, MatchPhase phase, int maxFrames) {
    const int expected = static_cast<int>(phase);
    for (int i = 0; i < maxFrames; ++i) {
        if (runtime.snapshot().matchPhase == expected) {
            return true;
        }
        runtime.step({}, 1);
    }
    return runtime.snapshot().matchPhase == expected;
}

bool waitForActiveFight(RuntimeProbe& runtime, int maxFrames) {
    return waitForMatchPhase(runtime, MatchPhase::Fight, maxFrames);
}

bool waitForP1Idle(RuntimeProbe& runtime, int maxFrames) {
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

std::string fighterDetail(const FighterSnapshot& fighter) {
    return "state=" + std::to_string(fighter.stateNo)
        + " action=" + std::to_string(fighter.action)
        + " time=" + std::to_string(fighter.stateTime)
        + " life=" + std::to_string(fighter.life)
        + " move=" + std::string(1, fighter.moveType)
        + " type=" + std::string(1, fighter.stateType)
        + " phys=" + std::string(1, fighter.physics)
        + " ctrl=" + std::to_string(fighter.ctrl ? 1 : 0)
        + " ground=" + std::to_string(fighter.onGround ? 1 : 0)
        + " hpause=" + std::to_string(fighter.hitPauseTicks)
        + " hstun=" + std::to_string(fighter.hitStunTicks);
}

std::string snapshotDetail(const RuntimeSnapshot& snap) {
    return "phase=" + std::to_string(snap.matchPhase)
        + " winner=" + std::to_string(snap.roundWinner)
        + " reason=" + std::to_string(snap.roundEndReason)
        + " effects=" + std::to_string(snap.activeEffects)
        + " sounds=" + std::to_string(snap.activeSounds)
        + " p1{" + fighterDetail(snap.p1) + "}"
        + " p2{" + fighterDetail(snap.p2) + "}"
        + " text=\"" + snap.lastHitText + "\"";
}

bool setupCase(RuntimeProbe& runtime, std::ostream& out, ScenarioMode mode, const CombatCase& testCase) {
    if (!runtime.setup(testCase.characterId, "Mountainside", mode, out)) {
        return false;
    }
    return waitForActiveFight(runtime, 480) && waitForP1Idle(runtime, 480);
}

bool guardStateLooksValid(const FighterSnapshot& p2) {
    return p2.stateNo == 130
        || p2.stateNo == 131
        || p2.stateNo == 140
        || p2.stateNo == 150
        || p2.stateNo == 151
        || p2.stateNo == 152
        || p2.stateNo == 153
        || p2.moveType == 'H';
}

void runGuardCase(RuntimeProbe& runtime, Counts& counts, std::ostream& out, const CombatCase& testCase) {
    const std::string prefix = std::string(testCase.label) + "_guard";
    if (!setupCase(runtime, out, ScenarioMode::Versus, testCase)) {
        record(out, counts, Status::Blocked, prefix + "_setup", "Versus setup did not reach controllable fight");
        return;
    }

    SymbolicInput p2Guard;
    p2Guard.right = true;
    p2Guard.down = testCase.crouchGuard;
    bool sawGuard = false;
    bool sawDefenderGuardState = false;
    bool sawGuardSpark = false;
    bool sawGuardSound = false;
    bool recovered = false;
    RuntimeSnapshot guardSnap;
    RuntimeSnapshot finalSnap;
    float matchedHalfDistance = 0.0f;
    constexpr std::array<float, 10> halfDistances{4.0f, 8.0f, 12.0f, 16.0f, 20.0f, 24.0f, 30.0f, 36.0f, 44.0f, 56.0f};
    for (const float halfDistance : halfDistances) {
        runtime.positionFighters(-halfDistance, halfDistance);
        runtime.forceFighterState(0, 0);
        runtime.forceFighterState(1, 0);
        runtime.setFighterControl(1, true);
        runtime.step({}, p2Guard, 4);

        const auto before = runtime.snapshot();
        runtime.forceFighterState(0, testCase.guardState);
        for (int i = 0; i < 240; ++i) {
            runtime.step({}, p2Guard, 1);
            const auto snap = runtime.snapshot();
            if (snap.lastHitText.find("P1 guard ") != std::string::npos) {
                sawGuard = true;
                matchedHalfDistance = halfDistance;
                guardSnap = snap;
                sawDefenderGuardState = sawDefenderGuardState || guardStateLooksValid(snap.p2);
                sawGuardSpark = sawGuardSpark || snap.activeEffects > before.activeEffects
                    || snap.lastHitText.find("spark ") != std::string::npos;
                sawGuardSound = sawGuardSound || snap.activeSounds > before.activeSounds
                    || snap.lastHitText.find("snd ") != std::string::npos;
            }
            if (sawGuard && guardStateLooksValid(snap.p2)) {
                sawDefenderGuardState = true;
            }
            if (sawGuard && snap.p2.ctrl && snap.p2.onGround && (snap.p2.moveType == 'I' || guardStateLooksValid(snap.p2))) {
                recovered = true;
                finalSnap = snap;
                break;
            }
            finalSnap = snap;
        }
        if (sawGuard) {
            break;
        }
    }

    record(out, counts, sawGuard ? Status::Pass : Status::Fail,
        prefix + "_contact",
        "half_distance=" + std::to_string(matchedHalfDistance) + " " + snapshotDetail(sawGuard ? guardSnap : finalSnap));
    record(out, counts, sawGuard && sawDefenderGuardState ? Status::Pass : Status::Fail,
        prefix + "_defender_state",
        snapshotDetail(sawGuard ? guardSnap : finalSnap));
    record(out, counts, sawGuard && sawGuardSpark && sawGuardSound ? Status::Pass : Status::Fail,
        prefix + "_presentation",
        snapshotDetail(sawGuard ? guardSnap : finalSnap));
    record(out, counts, recovered ? Status::Pass : Status::Fail,
        prefix + "_recovers",
        snapshotDetail(finalSnap));
}

bool fallStateLooksValid(const FighterSnapshot& p2) {
    return p2.stateNo == 5050
        || p2.stateNo == 5070
        || p2.stateNo == 5071
        || p2.stateNo == 5080
        || p2.stateNo == 5100
        || p2.stateNo == 5101
        || p2.stateNo == 5110
        || p2.stateNo == 5120
        || p2.stateNo == 5160
        || p2.stateType == 'L';
}

void runFallCase(RuntimeProbe& runtime, Counts& counts, std::ostream& out, const CombatCase& testCase) {
    const std::string prefix = std::string(testCase.label) + "_fall";
    if (!setupCase(runtime, out, ScenarioMode::Training, testCase)) {
        record(out, counts, Status::Blocked, prefix + "_setup", "Training setup did not reach controllable fight");
        return;
    }

    runtime.setTrainingDummyGuardMode("off");
    runtime.positionFighters(-12.0f, 14.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(1, false);
    runtime.step({}, 2);
    runtime.forceFighterState(0, testCase.fallState);

    bool sawHit = false;
    bool sawFall = false;
    bool sawGrounded = false;
    bool recovered = false;
    RuntimeSnapshot hitSnap;
    RuntimeSnapshot finalSnap;
    for (int i = 0; i < 540; ++i) {
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        if (snap.lastHitText.find("P1 hit ") != std::string::npos && !sawHit) {
            sawHit = true;
            hitSnap = snap;
        }
        if (sawHit && fallStateLooksValid(snap.p2)) {
            sawFall = true;
        }
        if (sawFall && snap.p2.onGround && (snap.p2.stateType == 'L' || snap.p2.stateNo == 5110 || snap.p2.stateNo == 5120)) {
            sawGrounded = true;
        }
        if (sawGrounded && snap.p2.stateNo == 0 && snap.p2.ctrl && snap.p2.onGround && snap.p2.moveType == 'I') {
            recovered = true;
            finalSnap = snap;
            break;
        }
        finalSnap = snap;
    }

    record(out, counts, sawHit ? Status::Pass : Status::Fail,
        prefix + "_contact",
        snapshotDetail(sawHit ? hitSnap : finalSnap));
    record(out, counts, sawHit && sawFall ? Status::Pass : Status::Fail,
        prefix + "_knockdown_path",
        snapshotDetail(finalSnap));
    record(out, counts, sawHit && sawGrounded ? Status::Pass : Status::Fail,
        prefix + "_grounded",
        snapshotDetail(finalSnap));
    record(out, counts, recovered ? Status::Pass : Status::Fail,
        prefix + "_recovers",
        snapshotDetail(finalSnap));
}

void runKoCase(RuntimeProbe& runtime, Counts& counts, std::ostream& out, const CombatCase& testCase) {
    const std::string prefix = std::string(testCase.label) + "_ko";
    if (!setupCase(runtime, out, ScenarioMode::SinglePlayer, testCase)) {
        record(out, counts, Status::Blocked, prefix + "_setup", "Single-player setup did not reach controllable fight");
        return;
    }

    runtime.positionFighters(-16.0f, 16.0f);
    runtime.forceFighterState(0, 0);
    runtime.forceFighterState(1, 0);
    runtime.setFighterControl(1, false);
    runtime.setFighterLife(1, 5);
    runtime.step({}, 2);
    const int beforeLife = runtime.snapshot().p2.life;
    runtime.forceFighterState(0, testCase.koState);

    bool sawHit = false;
    bool sawLifeZero = false;
    bool sawRoundFinish = false;
    RuntimeSnapshot hitSnap;
    RuntimeSnapshot finalSnap;
    for (int i = 0; i < 360; ++i) {
        runtime.setFighterControl(1, false);
        runtime.step({}, 1);
        const auto snap = runtime.snapshot();
        const bool p1ActualHit = snap.lastHitText.find("P1 hit ") != std::string::npos
            || snap.p1.moveHit
            || snap.p2.life < beforeLife;
        if (p1ActualHit && !sawHit) {
            sawHit = true;
            hitSnap = snap;
        }
        sawLifeZero = sawLifeZero || snap.p2.life <= 0;
        if (snap.matchPhase == static_cast<int>(MatchPhase::RoundFinish)) {
            sawRoundFinish = true;
            finalSnap = snap;
            break;
        }
        finalSnap = snap;
    }

    const bool finishMatches = sawRoundFinish
        && finalSnap.roundWinner == 1
        && finalSnap.roundEndReason == static_cast<int>(RoundEndReason::Ko)
        && finalSnap.p2.life <= 0;
    record(out, counts, sawHit ? Status::Pass : Status::Fail,
        prefix + "_actual_hit",
        snapshotDetail(sawHit ? hitSnap : finalSnap));
    record(out, counts, sawHit && sawLifeZero ? Status::Pass : Status::Fail,
        prefix + "_life_zero",
        snapshotDetail(finalSnap));
    record(out, counts, finishMatches ? Status::Pass : Status::Fail,
        prefix + "_round_finish",
        snapshotDetail(finalSnap));
}

} // namespace

int runClassicFightCombat(RuntimeProbe& runtime, std::ostream& out) {
    Counts counts;
    header(out, runtime, "classic-fight-combat");

    constexpr std::array<CombatCase, 3> cases{{
        {"kfm", "kfm", 200, 440, 200, false},
        {"EvilKen", "evilken", 420, 420, 210, true},
        {"EvilRyu", "evilryu", 420, 420, 210, true},
    }};

    for (const auto& testCase : cases) {
        runGuardCase(runtime, counts, out, testCase);
        runFallCase(runtime, counts, out, testCase);
        runKoCase(runtime, counts, out, testCase);
    }

    summary(out, counts);
    return exitCode(counts);
}

} // namespace dragon::verification
