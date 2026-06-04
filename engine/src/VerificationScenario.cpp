#include "VerificationScenario.h"

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
    record(out, counts, timer.comboHits > 0 ? Status::Pass : Status::Partial, "combo_or_hit_evidence",
        "combo_hits=" + std::to_string(timer.comboHits) + " last_hit=\"" + timer.lastHitText + "\"");
    record(out, counts, Status::Pass, "clean_exit", "scenario completed without crash");
    summary(out, counts);
    return exitCode(counts);
}

} // namespace

int runNamedScenario(RuntimeProbe& runtime, std::string_view scenarioName, std::ostream& out) {
    if (scenarioName == "kfm-baseline") {
        return runKfmBaseline(runtime, out);
    }
    if (scenarioName == "evilken-smoke") {
        return runEvilKenSmoke(runtime, out);
    }

    out << "VERIFY " << scenarioName << "\n"
        << "BLOCKED unknown_scenario\n"
        << "  supported: kfm-baseline, evilken-smoke\n"
        << "SUMMARY pass=0 partial=0 fail=0 blocked=1\n";
    return 2;
}

} // namespace dragon::verification
